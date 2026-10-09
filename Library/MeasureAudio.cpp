// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "MeasureAudio.h"
#include "CommandHandler.h"
#include "Logger.h"

#include <cerrno>
#include <cwchar>
#include <cwctype>
#include <Endpointvolume.h>
#include <Functiondiscoverykeys_devpkey.h>
#include <Mmdeviceapi.h>

// Undocumented COM interface used to set the default audio render endpoint.
class DECLSPEC_UUID("294935CE-F637-4E7C-A41B-AB255460B862") CPolicyConfigClient;

interface IPolicyConfig : public IUnknown
{
public:
  virtual HRESULT GetMixFormat(PCWSTR, WAVEFORMATEX**);
  virtual HRESULT STDMETHODCALLTYPE GetDeviceFormat(PCWSTR, INT, WAVEFORMATEX**);
  virtual HRESULT STDMETHODCALLTYPE SetDeviceFormat(PCWSTR, WAVEFORMATEX*, WAVEFORMATEX*);
  virtual HRESULT STDMETHODCALLTYPE GetProcessingPeriod(PCWSTR, INT, PINT64, PINT64);
  virtual HRESULT STDMETHODCALLTYPE SetProcessingPeriod(PCWSTR, PINT64);
  virtual HRESULT STDMETHODCALLTYPE GetShareMode(PCWSTR, struct DeviceShareMode*);
  virtual HRESULT STDMETHODCALLTYPE SetShareMode(PCWSTR, struct DeviceShareMode*);
  virtual HRESULT STDMETHODCALLTYPE GetPropertyValue(PCWSTR, const PROPERTYKEY&, PROPVARIANT*);
  virtual HRESULT STDMETHODCALLTYPE SetPropertyValue(PCWSTR, const PROPERTYKEY&, PROPVARIANT*);
  virtual HRESULT STDMETHODCALLTYPE SetDefaultEndpoint(PCWSTR, ERole);
  virtual HRESULT STDMETHODCALLTYPE SetEndpointVisibility(PCWSTR, INT);
};

interface DECLSPEC_UUID("568b9108-44bf-40b4-9006-86afe5b5a620") IPolicyConfig;

namespace {

template <class T>
void SafeRelease(T*& object)
{
	if (object)
	{
		object->Release();
		object = nullptr;
	}
}

bool CreateEnumerator(MeasureAudio* measure, IMMDeviceEnumerator** enumerator)
{
	HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)enumerator);
	if (hr == S_OK && *enumerator)
	{
		return true;
	}

	if (hr == REGDB_E_CLASSNOTREG)
	{
		LogErrorF(measure, L"Audio: COM creation failed REGDB_E_CLASSNOTREG");
	}
	else if (hr == CLASS_E_NOAGGREGATION)
	{
		LogErrorF(measure, L"Audio: COM creation failed CLASS_E_NOAGGREGATION");
	}
	else if (hr == E_NOINTERFACE)
	{
		LogErrorF(measure, L"Audio: COM creation failed E_NOINTERFACE");
	}
	else
	{
		LogErrorF(measure, L"Audio: COM creation failed %li", (long)hr);
	}

	return false;
}

std::wstring GetDefaultEndpointID(IMMDeviceEnumerator* enumerator)
{
	std::wstring id;
	IMMDevice* endpoint = nullptr;
	if (enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &endpoint) == S_OK)
	{
		LPWSTR endpointID = nullptr;
		if (endpoint->GetId(&endpointID) == S_OK)
		{
			id = endpointID;
		}
		CoTaskMemFree(endpointID);
	}

	SafeRelease(endpoint);
	return id;
}

bool ReadCommandArgument(const std::wstring& command, std::wstring* bang, std::wstring* argument)
{
	const size_t pos = command.find(' ');
	if (pos == std::wstring::npos)
	{
		return false;
	}

	*bang = command.substr(0, pos);
	*argument = command.substr(pos + 1);
	return true;
}

bool ReadAudioArgument(const WCHAR* arg, long& value, bool& relative)
{
	while (iswspace(*arg)) ++arg;
	relative = *arg == L'+' || *arg == L'-';
	errno = 0;
	WCHAR* end = nullptr;
	value = wcstol(arg, &end, 10);
	if (end == arg || errno == ERANGE) return false;

	while (iswspace(*end)) ++end;
	return *end == L'\0';
}

}  // namespace

MeasureAudio::MeasureAudio(Skin* skin, const WCHAR* name) : Measure(skin, name),
	m_IsMute(FALSE),
	m_MasterVolume(0.5f)
{
	m_MaxValue = 100.0;

	static const bool s_BangsRegistered = []()
	{
		const UINT typeId = TypeID<MeasureAudio>();
		CommandHandler::RegisterMeasureBang<MeasureAudio, &MeasureAudio::SetVolumeBang>(typeId, L"Audio:SetVolume");
		CommandHandler::RegisterMeasureBang<MeasureAudio, &MeasureAudio::SetOutputIndexBang>(typeId, L"Audio:SetOutputIndex");
		CommandHandler::RegisterMeasureBang<MeasureAudio, &MeasureAudio::Mute>(typeId, L"Audio:Mute");
		CommandHandler::RegisterMeasureBang<MeasureAudio, &MeasureAudio::Unmute>(typeId, L"Audio:Unmute");
		CommandHandler::RegisterMeasureBang<MeasureAudio, &MeasureAudio::ToggleMute>(typeId, L"Audio:ToggleMute");
		return true;
	} ();
}

MeasureAudio::~MeasureAudio()
{
}

void MeasureAudio::Initialize()
{
	Measure::Initialize();

	EnumerateEndpoints();
	GetAudioState(VolumeAction::Initialize);
}

void MeasureAudio::UpdateValue()
{
	GetAudioState(VolumeAction::GetVolume);
	m_Value = m_IsMute ? -1.0 : floor(m_MasterVolume * 100.0 + 0.5);
	if (m_Value > 100.0)
	{
		m_Value = 100.0;
	}
}

std::optional<std::wstring_view> MeasureAudio::GetStringValue()
{
	m_StringValue = L"ERROR";

	IMMDeviceEnumerator* enumerator = nullptr;
	if (!CreateEnumerator(this, &enumerator))
	{
		m_StringValue = L"ERROR - Initializing COM";
		return CheckSubstitute(m_StringValue);
	}

	IMMDevice* endpoint = nullptr;
	if (enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &endpoint) == S_OK)
	{
		IPropertyStore* props = nullptr;
		if (endpoint->OpenPropertyStore(STGM_READ, &props) == S_OK)
		{
			PROPVARIANT varName;
			PropVariantInit(&varName);
			if (props->GetValue(PKEY_Device_DeviceDesc, &varName) == S_OK)
			{
				m_StringValue = varName.pwszVal;
			}
			else
			{
				m_StringValue = L"ERROR - Getting Device Description";
			}
			PropVariantClear(&varName);
		}
		else
		{
			m_StringValue = L"ERROR - Getting Property";
		}

		SafeRelease(props);
	}
	else
	{
		m_StringValue = L"ERROR - Getting Default Device";
	}

	SafeRelease(endpoint);
	SafeRelease(enumerator);
	return CheckSubstitute(m_StringValue);
}

void MeasureAudio::Command(const std::wstring& command)
{
	std::wstring bang;
	std::wstring argument;
	if (ReadCommandArgument(command, &bang, &argument))
	{
		const bool output = _wcsicmp(bang.c_str(), L"SetOutputIndex") == 0;
		const bool volume = _wcsicmp(bang.c_str(), L"SetVolume") == 0;
		const bool relative = _wcsicmp(bang.c_str(), L"ChangeVolume") == 0;
		if (!output && !volume && !relative)
		{
			LogWarningF(this, L"Audio: Unknown bang");
			return;
		}

		int value = 0;
		if (swscanf_s(argument.c_str(), L"%d", &value) != 1 || (relative && value == 0))
		{
			LogWarningF(this, L"Audio: Incorrect number of arguments for bang");
			return;
		}

		if (output)
		{
			SetOutputIndex(value, false, true);
		}
		else
		{
			SetVolume(value, relative, true);
		}

	}
	else if (_wcsicmp(command.c_str(), L"ToggleNext") == 0)
	{
		SetOutputIndex(1, true);
	}
	else if (_wcsicmp(command.c_str(), L"TogglePrevious") == 0)
	{
		SetOutputIndex(-1, true);
	}
	else if (_wcsicmp(command.c_str(), L"ToggleMute") == 0)
	{
		ToggleMute();
	}
	else if (_wcsicmp(command.c_str(), L"Mute") == 0)
	{
		Mute();
	}
	else if (_wcsicmp(command.c_str(), L"Unmute") == 0)
	{
		Unmute();
	}
	else
	{
		LogWarningF(this, L"Audio: Unknown bang");
	}
}

void MeasureAudio::SetVolumeBang(const WCHAR* arg)
{
	long value = 0;
	bool relative = false;
	if (!ReadAudioArgument(arg, value, relative))
	{
		LogErrorF(this, L"!Audio:SetVolume: Invalid volume: %s", arg);
		return;
	}

	SetVolume(value, relative);
}

void MeasureAudio::SetOutputIndexBang(const WCHAR* arg)
{
	long value = 0;
	bool relative = false;
	if (!ReadAudioArgument(arg, value, relative))
	{
		LogErrorF(this, L"!Audio:SetOutputIndex: Invalid output index: %s", arg);
		return;
	}

	SetOutputIndex(value, relative);
}

void MeasureAudio::SetOutputIndex(long value, bool relative, bool clamp)
{
	if (relative && value == 0) return;

	EnumerateEndpoints();
	const long long count = (long long)m_EndpointIDs.size();
	if (count == 0)
	{
		LogErrorF(this, L"Audio: No output device found");
		return;
	}

	long long index = value;
	if (relative)
	{
		const UINT currentIndex = GetDefaultEndpointIndex();
		if (!currentIndex)
		{
			LogErrorF(this, L"Audio: Could not find default output");
			return;
		}

		index = (((long long)currentIndex - 1 + value) % count + count) % count + 1;
	}
	else if (clamp)
	{
		index = (index < 1) ? 1 : ((index > count) ? count : index);
	}
	else if (index < 1 || index > count)
	{
		LogErrorF(this, L"Audio: Output index out of range: %li", value);
		return;
	}

	const HRESULT hr = RegisterDevice(m_EndpointIDs[(size_t)(index - 1)].c_str());
	if (FAILED(hr))
	{
		LogErrorF(this, L"Audio: Error setting output (%li)", (long)hr);
	}
}

void MeasureAudio::Mute()
{
	SetMute(VolumeAction::Mute);
}

void MeasureAudio::Unmute()
{
	SetMute(VolumeAction::Unmute);
}

void MeasureAudio::ToggleMute()
{
	SetMute(VolumeAction::ToggleMute);
}

void MeasureAudio::SetMute(VolumeAction action)
{
	if (!GetAudioState(action))
	{
		LogErrorF(this, L"Audio: Error setting mute state");
	}
}

void MeasureAudio::EnumerateEndpoints()
{
	m_EndpointIDs.clear();

	IMMDeviceEnumerator* enumerator = nullptr;
	if (!CreateEnumerator(this, &enumerator))
	{
		return;
	}

	IMMDeviceCollection* collection = nullptr;
	if (enumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &collection) != S_OK || !collection)
	{
		LogWarningF(this, L"Audio: Could not enumerate AudioEndpoints");
		SafeRelease(enumerator);
		return;
	}

	UINT count = 0;
	if (collection->GetCount(&count) == S_OK)
	{
		m_EndpointIDs.resize(count);
		for (UINT i = 0; i < count; ++i)
		{
			IMMDevice* endpoint = nullptr;
			if (collection->Item(i, &endpoint) == S_OK)
			{
				LPWSTR endpointID = nullptr;
				if (endpoint->GetId(&endpointID) == S_OK)
				{
					m_EndpointIDs[i] = endpointID;
				}
				CoTaskMemFree(endpointID);
			}
			SafeRelease(endpoint);
		}
	}

	SafeRelease(collection);
	SafeRelease(enumerator);
}

bool MeasureAudio::GetAudioState(VolumeAction action)
{
	bool success = false;
	IMMDeviceEnumerator* enumerator = nullptr;
	if (CreateEnumerator(this, &enumerator))
	{
		IMMDevice* endpoint = nullptr;
		if (enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &endpoint) == S_OK)
		{
			IAudioEndpointVolume* endpointVolume = nullptr;
			if (endpoint->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr, (void**)&endpointVolume) == S_OK)
			{
				if (action == VolumeAction::Mute || action == VolumeAction::Unmute || action == VolumeAction::ToggleMute)
				{
					BOOL mute = action == VolumeAction::Mute;
					success = action != VolumeAction::ToggleMute || endpointVolume->GetMute(&mute) == S_OK;
					if (success)
					{
						if (action == VolumeAction::ToggleMute) mute = !mute;
						success = endpointVolume->SetMute(mute, nullptr) == S_OK;
						if (success) m_IsMute = mute;
					}
				}
				else
				{
					float volume = 0.0f;
					endpointVolume->GetMute(&m_IsMute);
					if (endpointVolume->GetMasterVolumeLevelScalar(&volume) == S_OK)
					{
						m_MasterVolume = volume;
						success = true;
					}
				}
			}
			SafeRelease(endpointVolume);
		}
		SafeRelease(endpoint);
	}

	SafeRelease(enumerator);
	return success;
}

void MeasureAudio::SetVolume(long value, bool relative, bool unmute)
{
	if (relative && value == 0) return;

	bool success = false;
	IMMDeviceEnumerator* enumerator = nullptr;
	if (CreateEnumerator(this, &enumerator))
	{
		IMMDevice* endpoint = nullptr;
		if (enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &endpoint) == S_OK)
		{
			IAudioEndpointVolume* endpointVolume = nullptr;
			if (endpoint->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr, (void**)&endpointVolume) == S_OK)
			{
				float newVolume = 0.0f;
				success = !relative || endpointVolume->GetMasterVolumeLevelScalar(&newVolume) == S_OK;
				if (success && unmute)
				{
					success = endpointVolume->SetMute(FALSE, nullptr) == S_OK;
					if (success) m_IsMute = FALSE;
				}

				if (success)
				{
					newVolume += (float)value / 100.0f;
					newVolume = (newVolume < 0.0f) ? 0.0f : ((newVolume > 1.0f) ? 1.0f : newVolume);
					success = endpointVolume->SetMasterVolumeLevelScalar(newVolume, nullptr) == S_OK;
				}

				if (success)
				{
					success = endpointVolume->GetMasterVolumeLevelScalar(&newVolume) == S_OK;
				}
				if (success)
				{
					m_MasterVolume = newVolume;
				}
			}
			SafeRelease(endpointVolume);
		}
		SafeRelease(endpoint);
	}

	SafeRelease(enumerator);
	if (!success)
	{
		LogErrorF(this, L"Audio: Error setting volume");
	}
}

UINT MeasureAudio::GetDefaultEndpointIndex()
{
	UINT index = 0;
	IMMDeviceEnumerator* enumerator = nullptr;
	if (CreateEnumerator(this, &enumerator))
	{
		const std::wstring defaultID = GetDefaultEndpointID(enumerator);
		for (UINT i = 0; i < m_EndpointIDs.size(); ++i)
		{
			if (_wcsicmp(m_EndpointIDs[i].c_str(), defaultID.c_str()) == 0)
			{
				index = i + 1;
				break;
			}
		}
	}

	SafeRelease(enumerator);
	return index;
}

HRESULT MeasureAudio::RegisterDevice(const WCHAR* deviceID)
{
	HRESULT hr = S_FALSE;
	IPolicyConfig* policyConfig = nullptr;
	hr = CoCreateInstance(__uuidof(CPolicyConfigClient), nullptr, CLSCTX_ALL, __uuidof(IPolicyConfig), (void**)&policyConfig);
	if (hr == S_OK)
	{
		hr = policyConfig->SetDefaultEndpoint(deviceID, eConsole);
		if (hr == S_OK)
		{
			hr = policyConfig->SetDefaultEndpoint(deviceID, eCommunications);
		}
	}

	SafeRelease(policyConfig);
	return hr;
}
