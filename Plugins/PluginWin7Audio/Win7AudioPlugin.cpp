/*
  Copyright (C) 2010 Stefan Hiller

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#pragma warning(disable: 4786)
#pragma warning(disable: 4996)

#include <windows.h>
#include <math.h>
#include <string>
#include <map>
#include <vector>
#include <time.h>

#include <Mmdeviceapi.h>
#include <Endpointvolume.h>
#include <Functiondiscoverykeys_devpkey.h>
#include "PolicyConfig.h"

#include "..\..\Library\Export.h"	// Rainmeter's exported functions

#define SAFE_RELEASE(punk)  \
              if ((punk) != NULL) { (punk)->Release(); (punk) = NULL; }

/* The exported functions */
extern "C"
{
__declspec( dllexport ) UINT Initialize(HMODULE instance, LPCTSTR iniFile, LPCTSTR section, UINT id);
__declspec( dllexport ) void Finalize(HMODULE instance, UINT id);
//__declspec( dllexport ) UINT Update(UINT id);
__declspec( dllexport ) double Update2(UINT id);
__declspec( dllexport ) LPCTSTR GetString(UINT id, UINT flags);
__declspec( dllexport ) UINT GetPluginVersion();
__declspec( dllexport ) LPCTSTR GetPluginAuthor();
__declspec( dllexport ) void ExecuteBang(LPCTSTR args, UINT id);
}

static BOOL com_initialized = FALSE;
static BOOL instance_created = FALSE;
static BOOL is_mute = FALSE;
static float master_volume = 0.5f;

static enum VolumeAction
{
	INIT,
	TOGGLE_MUTE,
	GET_VOLUME
};

const static CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const static IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const static IID IID_CPolicyConfigClient = __uuidof(CPolicyConfigClient);
const static IID IID_IPolicyConfig = __uuidof(IPolicyConfig);
const static IID IID_IAudioEndpointVolume = __uuidof(IAudioEndpointVolume);

static IMMDeviceEnumerator *pEnumerator = 0;
static IMMDeviceCollection *pCollection = 0;

static std::vector<std::wstring> endpointIDs;

UINT CleanUp()
{
	SAFE_RELEASE(pCollection);
	SAFE_RELEASE(pEnumerator);
	instance_created = false;
	return 0;
}

bool InitCom()
{
	if (!com_initialized) com_initialized = SUCCEEDED(CoInitialize(0));
	if (!com_initialized)
	{
		LSLog(LOG_DEBUG, L"Rainmeter", L"Win7AudioPlugin: COM initialization failed!");
		return false;
	}
	HRESULT hr = CoCreateInstance(CLSID_MMDeviceEnumerator, 0, CLSCTX_ALL,
								  IID_IMMDeviceEnumerator, (void**)&pEnumerator);
	instance_created = (S_OK == hr && pEnumerator);
	if (!instance_created)
	{
		std::wstring dbg_str = L"Win7AudioPlugin: COM instance creation failed!";
		if (hr == REGDB_E_CLASSNOTREG) dbg_str += L" REGDB_E_CLASSNOTREG";
		else if (hr == CLASS_E_NOAGGREGATION) dbg_str += L" CLASS_E_NOAGGREGATION";
		else if (hr == E_NOINTERFACE) dbg_str += L" E_NOINTERFACE";
		else
		{
			static WCHAR e_code[256];
			wsprintf(e_code, L" %li", (long)hr);

			dbg_str += e_code;
		}
		LSLog(LOG_DEBUG, L"Rainmeter", dbg_str.c_str());
		return CleanUp() != 0;
	}
	if (S_OK != pEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pCollection) || !pCollection)
	{
		LSLog(LOG_DEBUG, L"Rainmeter", L"Win7AudioPlugin: Could not enumerate AudioEndpoints!");
		return CleanUp() != 0;
	}
	return true;
}

void UnInitCom()
{
	CleanUp();
	if (com_initialized) CoUninitialize();
	com_initialized = FALSE;
}

HRESULT RegisterDevice(PCWSTR devID)
{
	HRESULT hr = S_FALSE;
	try
	{
		InitCom();
		IPolicyConfig *pPolicyConfig;

		hr = CoCreateInstance(IID_CPolicyConfigClient, NULL,
							CLSCTX_ALL, IID_IPolicyConfig,
							(LPVOID *)&pPolicyConfig);
		if (hr == S_OK)
		{
			hr = pPolicyConfig->SetDefaultEndpoint(devID, eConsole);
			if (hr == S_OK)
			{
				hr = pPolicyConfig->SetDefaultEndpoint(devID, eCommunications);
			}
			SAFE_RELEASE(pPolicyConfig);
		}
	}
	catch (...)
	{
		LSLog(LOG_DEBUG, L"Rainmeter", L"Win7AudioPlugin: RegisterDevice - Exception!");
		hr = S_FALSE;
	}
	UnInitCom();
    return hr;
}

std::wstring GetDefaultID()
{
	std::wstring id_default = L"";
	IMMDevice * pEndpoint = 0;
	try
	{
		if (pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pEndpoint) == S_OK)
		{
			LPWSTR pwszID = 0;
			if (pEndpoint->GetId(&pwszID) == S_OK)
			{
				id_default = std::wstring(pwszID);
			}
			CoTaskMemFree(pwszID);
		}
	}
	catch (...)
	{
		LSLog(LOG_DEBUG, L"Rainmeter", L"Win7AudioPlugin: GetDefaultID - Exception!");
		id_default = L"Exception";
	}
	SAFE_RELEASE(pEndpoint)
	return id_default;
}

bool GetWin7AudioState(const VolumeAction action)
{
	IMMDevice * pEndpoint = 0;
	IAudioEndpointVolume * pEndptVol = 0;
	bool success = false;

	try
	{
		if (InitCom())
		{
			if (pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pEndpoint) == S_OK)
			{
				if (pEndpoint->Activate(IID_IAudioEndpointVolume, CLSCTX_ALL, 0, (void**)&pEndptVol) == S_OK)
				{
					if (pEndptVol->GetMute(&is_mute) == S_OK && action == TOGGLE_MUTE)
					{
						success = pEndptVol->SetMute(is_mute == TRUE ? FALSE : TRUE, 0) == S_OK;
					}
					// get current volume
					float vol = 0.0f;
					if (action != TOGGLE_MUTE && pEndptVol->GetMasterVolumeLevelScalar(&vol) == S_OK)
					{
						master_volume = vol;
					}
				}
			}
		}
	}
	catch (...)
	{
		LSLog(LOG_DEBUG, L"Rainmeter", L"Win7AudioPlugin: Win7ToggleMute - Exception!");
	}
	SAFE_RELEASE(pEndptVol)
	SAFE_RELEASE(pEndpoint)
	UnInitCom();
	return success;
}

UINT GetIndex()
{
	
	std::wstring id_default = L"";
	if (InitCom()) id_default = GetDefaultID();
	UnInitCom();

	for (UINT i = 0; i < endpointIDs.size(); i++)
	{
		if (endpointIDs[i].compare(id_default) == 0) return i + 1;
	}
	return 0;
}

bool SetWin7Volume(UINT volume, int offset = 0)
{
	IMMDevice * pEndpoint = 0;
	IAudioEndpointVolume * pEndptVol = 0;
	bool success = false;

	try
	{
		if (InitCom())
		{
			if (pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pEndpoint) == S_OK)
			{
				if (pEndpoint->Activate(IID_IAudioEndpointVolume, CLSCTX_ALL, 0, (void**)&pEndptVol) == S_OK)
				{
					pEndptVol->SetMute(FALSE, 0);
					float vol = 0.0f;
					if (offset != 0) // change master volume + offset
					{
						float off = static_cast<float>(offset) / 100.0f;
						vol = master_volume + off;
						vol = (vol < 0.0f) ? 0.0f : ((vol > 1.0f) ? 1.0f : vol);
					}
					else
					{
						vol = (float)volume / 100.0f;
					}
					// set to volume
					success = pEndptVol->SetMasterVolumeLevelScalar(vol, 0) == S_OK;
					if (success) success = pEndptVol->GetMasterVolumeLevelScalar(&vol) == S_OK;
					if (success) master_volume = vol;
				}
			}
		}

	}
	catch (...)
	{
		LSLog(LOG_DEBUG, L"Rainmeter", L"Win7AudioPlugin: SetWin7Volume - Exception!");
	}
	SAFE_RELEASE(pEndptVol)
	SAFE_RELEASE(pEndpoint)
	UnInitCom();
	return success;
}

/*
  This function is called when the measure is initialized.
  The function must return the maximum value that can be measured. 
  The return value can also be 0, which means that Rainmeter will
  track the maximum value automatically. The parameters for this
  function are:

  instance  The instance of this DLL
  iniFile   The name of the ini-file (usually Rainmeter.ini)
  section   The name of the section in the ini-file for this measure
  id        The identifier for the measure. This is used to identify the measures that use the same plugin.
*/
UINT Initialize(HMODULE instance, LPCTSTR iniFile, LPCTSTR section, UINT id)
{
	if (!InitCom())
	{
		UnInitCom();
		return 0;
	}

	UINT count;
    if (!pCollection || (S_OK != pCollection->GetCount(&count)))
	{
		UnInitCom();
		return 0;
	}
	endpointIDs = std::vector<std::wstring>(count);

	for (UINT i = 0; i < count; i++)
    {
	    IMMDevice *pEndpoint = 0;

        // Get pointer to endpoint number i.
		if (pCollection->Item(i, &pEndpoint) == S_OK)
		{
			// Get the endpoint ID string.
			LPWSTR pwszID = 0;
			if (pEndpoint->GetId(&pwszID) == S_OK)
			{
				endpointIDs[i] = std::wstring(pwszID);
			}
			CoTaskMemFree(pwszID);
		}
		SAFE_RELEASE(pEndpoint)
	}
	UnInitCom();
	GetWin7AudioState(INIT);
	return 100;
}

/*
  This function is called when new value should be measured.
  The function returns the new value.
*/
//UINT Update(UINT id)
//{
//	GetWin7AudioState(GET_VOLUME);
//	UINT volume = is_mute == TRUE ? 0 : static_cast<UINT>(master_volume * 100.0f + 0.5f);	// rounding up at 0.5
//	return volume > 100 ? 100 : volume;
//}

/*
This function is called when new value should be measured.
The function returns the new value.
*/
double Update2(UINT id)
{	
	GetWin7AudioState(GET_VOLUME);
	double volume = is_mute == TRUE ? -1.0 : floor(master_volume * 100.0 + 0.5);	// rounding up at 0.5
	return volume > 100.0 ? 100.0 : volume;
}

LPCTSTR GetString(UINT id, UINT flags) 
{
	static WCHAR result[256];
	wsprintf(result, L"ERROR");
	try {
		if (!InitCom() || !pEnumerator)
		{
			UnInitCom();
			wsprintf(result, L"ERROR - Initializing COM");
			return result;
		}

		IMMDevice * pEndpoint = 0;
		if (pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pEndpoint) == S_OK)
		{
			IPropertyStore * pProps = 0;
			if (pEndpoint->OpenPropertyStore(STGM_READ, &pProps) == S_OK)
			{
				PROPVARIANT varName;
				PropVariantInit(&varName);
				if (pProps->GetValue(PKEY_Device_DeviceDesc, &varName) == S_OK)
				{
					wcsncpy(result, varName.pwszVal, 255);
					PropVariantClear(&varName);
					SAFE_RELEASE(pProps)
					SAFE_RELEASE(pEndpoint)
					UnInitCom();
					return result;
				}
				else
				{
					PropVariantClear(&varName);
					SAFE_RELEASE(pProps)
					SAFE_RELEASE(pEndpoint)
					wsprintf(result, L"ERROR - Getting Device Description");
				}
			}
			else
			{
				SAFE_RELEASE(pProps)
				SAFE_RELEASE(pEndpoint)
				wsprintf(result, L"ERROR - Getting Property");
			}
		}
		else
		{
			SAFE_RELEASE(pEndpoint)
			wsprintf(result, L"ERROR - Getting Default Device");
		}
	} catch (...) {
		LSLog(LOG_DEBUG, L"Rainmeter", L"Win7AudioPlugin: GetString - Exception!");
		wsprintf(result, L"Exception");
	}
	UnInitCom();
	return result;
}

/*
  If the measure needs to free resources before quitting.
  The plugin can export Finalize function, which is called
  when Rainmeter quits (or refreshes).
*/
void Finalize(HMODULE instance, UINT id)
{
	UnInitCom();
}

UINT GetPluginVersion()
{
	return 1006;
}

LPCTSTR GetPluginAuthor()
{
	return L"reiswaffel.deviantart.com";
}

void ExecuteBang(LPCTSTR args, UINT id)
{
	std::wstring wholeBang = args;

	size_t pos = wholeBang.find(' ');
	if (pos != -1)
	{
		std::wstring bang = wholeBang.substr(0, pos);
		wholeBang.erase(0, pos + 1);

		if (_wcsicmp(bang.c_str(), L"SetOutputIndex") == 0)
		{
			// Parse parameters
			int index = 0;
			if (1 == swscanf_s(wholeBang.c_str(), L"%d", &index))
			{
				if (endpointIDs.size() <= 0)
				{
					LSLog(LOG_DEBUG, L"Rainmeter", L"Win7AudioPlugin: No device found!");
					return;
				}
				// set to endpoint [index-1]
				if (index <= 0) index = 1;
				else if (index > (int)endpointIDs.size()) index = (int)endpointIDs.size();
				RegisterDevice(endpointIDs[index - 1].c_str());
			}
			else
			{
				LSLog(LOG_DEBUG, L"Rainmeter", L"Win7AudioPlugin: Incorrect number of arguments for the bang!");
			}
		}
		else if (_wcsicmp(bang.c_str(), L"SetVolume") == 0)
		{
			// Parse parameters
			int volume = 0;
			if (1 == swscanf_s(wholeBang.c_str(), L"%d", &volume))
			{
				if (!SetWin7Volume(volume < 0 ? 0 : (volume > 100 ? 100 : (UINT)volume)))
				{
					LSLog(LOG_DEBUG, L"Rainmeter", L"Win7AudioPlugin: Error setting volume!");
				}
			}
			else
			{
				LSLog(LOG_DEBUG, L"Rainmeter", L"Win7AudioPlugin: Incorrect number of arguments for the bang!");
			}
		}
		else if (_wcsicmp(bang.c_str(), L"ChangeVolume") == 0)
		{
			// Parse parameters
			int offset = 0;
			if (1 == swscanf_s(wholeBang.c_str(), L"%d", &offset) && offset)
			{
				if (!SetWin7Volume(0, offset))
				{
					LSLog(LOG_DEBUG, L"Rainmeter", L"Win7AudioPlugin: Error changing volume!");
				}
			}
			else
			{
				LSLog(LOG_DEBUG, L"Rainmeter", L"Win7AudioPlugin: Incorrect number of arguments for the bang!");
			}
		}
		else
		{
			LSLog(LOG_DEBUG, L"Rainmeter", L"Win7AudioPlugin: Unknown bang!");
		}

	}
	else if (_wcsicmp(wholeBang.c_str(), L"ToggleNext") == 0)
	{
		//LSLog(LOG_NOTICE, L"Rainmeter", L"Win7AudioPlugin: Next device.");
		const UINT i = GetIndex();
		if (i) RegisterDevice(endpointIDs[(i == endpointIDs.size()) ? 0 : i].c_str());
		else LSLog(LOG_DEBUG, L"Rainmeter", L"Win7AudioPlugin: Update error - Try refresh!");
	}
	else if (_wcsicmp(wholeBang.c_str(), L"TogglePrevious") == 0)
	{
		const UINT i = GetIndex();
		if (i) RegisterDevice(endpointIDs[(i == 1) ? endpointIDs.size() - 1 : i - 2].c_str());
		else LSLog(LOG_DEBUG, L"Rainmeter", L"Win7AudioPlugin: Update error - Try refresh!");
	}
	else if (_wcsicmp(wholeBang.c_str(), L"ToggleMute") == 0)
	{
		if (!GetWin7AudioState(TOGGLE_MUTE)) LSLog(LOG_DEBUG, L"Rainmeter", L"Win7AudioPlugin: Mute Toggle Error!");
	}
	else if (_wcsicmp(wholeBang.c_str(), L"Mute") == 0)
	{
		if (!is_mute)
		{
			if (!GetWin7AudioState(TOGGLE_MUTE))
			{
				LSLog(LOG_DEBUG, L"Rainmeter", L"Win7AudioPlugin: Mute Error!");
			}
		}
	}
	else if (_wcsicmp(wholeBang.c_str(), L"Unmute") == 0)
	{
		if (is_mute)
		{
			if (!GetWin7AudioState(TOGGLE_MUTE))
			{
				LSLog(LOG_DEBUG, L"Rainmeter", L"Win7AudioPlugin: Unmute Error!");
			}
		}
	}
	else
	{
		LSLog(LOG_DEBUG, L"Rainmeter", L"Win7AudioPlugin: Unknown bang!");
	}
}