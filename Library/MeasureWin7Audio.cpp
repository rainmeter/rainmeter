/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "MeasureWin7Audio.h"
#include "Logger.h"

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

bool CreateEnumerator(MeasureWin7Audio* measure, IMMDeviceEnumerator** enumerator)
{
	HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)enumerator);
	if (hr == S_OK && *enumerator)
	{
		return true;
	}

	if (hr == REGDB_E_CLASSNOTREG)
	{
		LogErrorF(measure, L"Win7Audio: COM creation failed REGDB_E_CLASSNOTREG");
	}
	else if (hr == CLASS_E_NOAGGREGATION)
	{
		LogErrorF(measure, L"Win7Audio: COM creation failed CLASS_E_NOAGGREGATION");
	}
	else if (hr == E_NOINTERFACE)
	{
		LogErrorF(measure, L"Win7Audio: COM creation failed E_NOINTERFACE");
	}
	else
	{
		LogErrorF(measure, L"Win7Audio: COM creation failed %li", (long)hr);
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

}  // namespace

MeasureWin7Audio::MeasureWin7Audio(Skin* skin, const WCHAR* name) : Measure(skin, name),
	m_IsMute(FALSE),
	m_MasterVolume(0.5f)
{
	m_MaxValue = 100.0;
}

MeasureWin7Audio::~MeasureWin7Audio()
{
}

void MeasureWin7Audio::Initialize()
{
	Measure::Initialize();

	EnumerateEndpoints();
	GetAudioState(VolumeAction::Initialize);
}

void MeasureWin7Audio::UpdateValue()
{
	GetAudioState(VolumeAction::GetVolume);
	m_Value = m_IsMute ? -1.0 : floor(m_MasterVolume * 100.0 + 0.5);
	if (m_Value > 100.0)
	{
		m_Value = 100.0;
	}
}

const WCHAR* MeasureWin7Audio::GetStringValue()
{
	m_StringValue = L"ERROR";

	IMMDeviceEnumerator* enumerator = nullptr;
	if (!CreateEnumerator(this, &enumerator))
	{
		m_StringValue = L"ERROR - Initializing COM";
		return CheckSubstitute(m_StringValue.c_str());
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
	return CheckSubstitute(m_StringValue.c_str());
}

void MeasureWin7Audio::Command(const std::wstring& command)
{
	std::wstring bang;
	std::wstring argument;
	if (ReadCommandArgument(command, &bang, &argument))
	{
		if (_wcsicmp(bang.c_str(), L"SetOutputIndex") == 0)
		{
			int index = 0;
			if (swscanf_s(argument.c_str(), L"%d", &index) == 1)
			{
				EnumerateEndpoints();
				if (m_EndpointIDs.empty())
				{
					LogWarningF(this, L"Win7Audio: No device found");
					return;
				}

				if (index <= 0)
				{
					index = 1;
				}
				else if (index > (int)m_EndpointIDs.size())
				{
					index = (int)m_EndpointIDs.size();
				}

				RegisterDevice(m_EndpointIDs[index - 1].c_str());
			}
			else
			{
				LogWarningF(this, L"Win7Audio: Incorrect number of arguments for bang");
			}
		}
		else if (_wcsicmp(bang.c_str(), L"SetVolume") == 0)
		{
			int volume = 0;
			if (swscanf_s(argument.c_str(), L"%d", &volume) == 1)
			{
				if (!SetVolume(volume < 0 ? 0 : (volume > 100 ? 100 : (UINT)volume)))
				{
					LogErrorF(this, L"Win7Audio: Error setting volume");
				}
			}
			else
			{
				LogWarningF(this, L"Win7Audio: Incorrect number of arguments for bang");
			}
		}
		else if (_wcsicmp(bang.c_str(), L"ChangeVolume") == 0)
		{
			int offset = 0;
			if (swscanf_s(argument.c_str(), L"%d", &offset) == 1 && offset)
			{
				if (!SetVolume(0, offset))
				{
					LogErrorF(this, L"Win7Audio: Error changing volume");
				}
			}
			else
			{
				LogWarningF(this, L"Win7Audio: Incorrect number of arguments for bang");
			}
		}
		else
		{
			LogWarningF(this, L"Win7Audio: Unknown bang");
		}
	}
	else if (_wcsicmp(command.c_str(), L"ToggleNext") == 0)
	{
		EnumerateEndpoints();
		const UINT index = GetDefaultEndpointIndex();
		if (index)
		{
			RegisterDevice(m_EndpointIDs[(index == m_EndpointIDs.size()) ? 0 : index].c_str());
		}
		else
		{
			LogErrorF(this, L"Win7Audio: Update error");
		}
	}
	else if (_wcsicmp(command.c_str(), L"TogglePrevious") == 0)
	{
		EnumerateEndpoints();
		const UINT index = GetDefaultEndpointIndex();
		if (index)
		{
			RegisterDevice(m_EndpointIDs[(index == 1) ? m_EndpointIDs.size() - 1 : index - 2].c_str());
		}
		else
		{
			LogErrorF(this, L"Win7Audio: Update error");
		}
	}
	else if (_wcsicmp(command.c_str(), L"ToggleMute") == 0)
	{
		GetAudioState(VolumeAction::ToggleMute);
	}
	else if (_wcsicmp(command.c_str(), L"Mute") == 0)
	{
		if (!m_IsMute)
		{
			GetAudioState(VolumeAction::ToggleMute);
		}
	}
	else if (_wcsicmp(command.c_str(), L"Unmute") == 0)
	{
		if (m_IsMute)
		{
			GetAudioState(VolumeAction::ToggleMute);
		}
	}
	else
	{
		LogWarningF(this, L"Win7Audio: Unknown bang");
	}
}

void MeasureWin7Audio::EnumerateEndpoints()
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
		LogWarningF(this, L"Win7Audio: Could not enumerate AudioEndpoints");
		SafeRelease(enumerator);
		return;
	}

	UINT count = 0U;
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

bool MeasureWin7Audio::GetAudioState(VolumeAction action)
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
				if (endpointVolume->GetMute(&m_IsMute) == S_OK && action == VolumeAction::ToggleMute)
				{
					success = endpointVolume->SetMute(m_IsMute ? FALSE : TRUE, nullptr) == S_OK;
				}

				float volume = 0.0f;
				if (action != VolumeAction::ToggleMute && endpointVolume->GetMasterVolumeLevelScalar(&volume) == S_OK)
				{
					m_MasterVolume = volume;
					success = true;
				}
			}
			SafeRelease(endpointVolume);
		}
		SafeRelease(endpoint);
	}

	SafeRelease(enumerator);
	return success;
}

bool MeasureWin7Audio::SetVolume(UINT volume, int offset)
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
				endpointVolume->SetMute(FALSE, nullptr);

				float newVolume = 0.0f;
				if (offset != 0)
				{
					newVolume = m_MasterVolume + (float)offset / 100.0f;
					newVolume = (newVolume < 0.0f) ? 0.0f : ((newVolume > 1.0f) ? 1.0f : newVolume);
				}
				else
				{
					newVolume = (float)volume / 100.0f;
				}

				success = endpointVolume->SetMasterVolumeLevelScalar(newVolume, nullptr) == S_OK;
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
	return success;
}

UINT MeasureWin7Audio::GetDefaultEndpointIndex()
{
	UINT index = 0U;
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

HRESULT MeasureWin7Audio::RegisterDevice(const WCHAR* deviceID)
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
