/*
Copyright (C) 2009 Shaivya Mahajan

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#pragma warning(disable: 4786)
#pragma warning(disable: 4996)

#include <windows.h>
#include <map>
#include <string>
#include <math.h>
#include <wlanapi.h>
#pragma comment( lib, "wlanapi.lib")
#include "..\..\Library\Export.h"	// Rainmeter's exported functions

/* The exported functions */
extern "C"
{
__declspec( dllexport ) UINT Initialize(HMODULE instance, LPCTSTR iniFile, LPCTSTR section, UINT id);
__declspec( dllexport ) void Finalize(HMODULE instance, UINT id);
__declspec( dllexport ) UINT Update(UINT id);
__declspec( dllexport ) LPCTSTR GetString(UINT id, UINT flags);
__declspec( dllexport ) UINT GetPluginVersion();
__declspec( dllexport ) LPCTSTR GetPluginAuthor();

}
//Function that translates DOT11 ENUMs to output strings
LPCTSTR getDot11str(int,int);
//Wrapper function for writing to log file
void Log(const WCHAR* string);

enum MEASURETYPE
{
	UNKNOWN,
	SSID,
	QUALITY,
	ENCRYPTION,
	AUTH,
	LIST,
	PHY,
};

//Struct for storing current meter's settings
typedef struct meas_data {
	MEASURETYPE type;
	UINT listStyle;
	UINT listMax;
	WCHAR * netlist;
	bool listInit;
} meas_data_t;


std::map<UINT, meas_data_t> g_meas_data;
int g_Instances = 0;
/* Globals that store system's wifi interface/adapter structs */
/* These are initialized in Initialize(), used during each update*/
HANDLE hClient = NULL;
PWLAN_INTERFACE_INFO pInterface = NULL;
PWLAN_INTERFACE_INFO_LIST pIntfList = NULL;

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
	/* initialize interface/adapter structs */
	DWORD dwNegotiatedVersion = 0;
	DWORD dwErr;
	g_Instances++;
	//Create WINLAN API Handle
	if(hClient == NULL){
		dwErr = WlanOpenHandle( WLAN_API_VERSION, NULL, &dwNegotiatedVersion, &hClient );
		if( ERROR_SUCCESS != dwErr ){
			WCHAR buffer[256];
			wsprintf(buffer, L"WifiStatus.dll: Unable to open WLAN API Handle. Error code (%d): %s",(int)dwErr,getDot11str(dwErr,5));
			Log(buffer);
			return 0;
		}
	}
	//Query list of WLAN interfaces
	if(pIntfList == NULL){
		dwErr= WlanEnumInterfaces(hClient, NULL, &pIntfList);
		if(( ERROR_SUCCESS != dwErr) || (&pIntfList->dwNumberOfItems <= 0)){
			WCHAR buffer[256];
			wsprintf(buffer, L"WifiStatus.dll: Unable to find any WLAN interfaces/adapters. Error code %d",(int) dwErr);
			Log(buffer);
			return 0;
		}
	}
	//Select a WLAN interface, default 0.
	LPCTSTR data = ReadConfigString(section, L"WifiIntfID", L"");
	
	if ((data != NULL) && (_wcsicmp(L"", data) != 0)){
		if(_wtoi(data) < (int)pIntfList->dwNumberOfItems){
			pInterface = &pIntfList->InterfaceInfo[_wtoi(data)];
		} else {
			WCHAR buffer[256];
			wsprintf(buffer, L"WifiStatus.dll: Adapter is disabled or invalid (WifiIntfID=%s), fix INTF ID(default=0) or enable the adapter.",data);
			Log(buffer);
			pInterface = &pIntfList->InterfaceInfo[0]; 
		}		
	} else {
		pInterface = &pIntfList->InterfaceInfo[0]; 
	}
	//Select LIST style
	data = ReadConfigString(section, L"WifiListStyle", L"");
	
	if ((data != NULL) && (_wcsicmp(L"", data) != 0)){
		if ( (_wtoi(data) >= 0) && (_wtoi(data) <= 3)){
			g_meas_data[id].listStyle = _wtoi(data);
		} else {
			WCHAR buffer[256];
			wsprintf(buffer, L"WifiStatus.dll: Invalid List Style given (WifiListStyle=%s), defaulting to 0.",data);
			Log(buffer);
			g_meas_data[id].listStyle = 0;
		}		
	} else {
		g_meas_data[id].listStyle = 0;
	}

	//Set maxmimum number of list items
	data = ReadConfigString(section, L"WifiListLimit", L"");
	g_meas_data[id].listInit = false;
	if ((data != NULL) && (_wcsicmp(L"", data) != 0)){
		if (_wtoi(data) > 0){
			g_meas_data[id].listMax = _wtoi(data);
		} else {
			WCHAR buffer[256];
			wsprintf(buffer, L"WifiStatus.dll: Invalid List Limit given (WifiListLimit=%s), defaulting to 5.",data);
			Log(buffer);
			g_meas_data[id].listMax = 5;
		}		
	} else {
		g_meas_data[id].listMax = 5;
	}
	//Select type of measure
	MEASURETYPE infoType = UNKNOWN;
	LPCTSTR type = ReadConfigString(section, L"WifiInfoType", L"");
	if(type){
		if (_wcsicmp(L"SSID", type) == 0){
			infoType=SSID;
		} 
		else if (_wcsicmp(L"QUALITY", type) == 0){
			infoType=QUALITY;
		} 
		else if (_wcsicmp(L"ENCRYPTION", type) == 0){
			infoType=ENCRYPTION;
		} 
		else if (_wcsicmp(L"AUTH", type) == 0){
			infoType=AUTH;
		} 
		else if (_wcsicmp(L"LIST", type) == 0){
			infoType=LIST;
		} 
		else if (_wcsicmp(L"PHY", type) == 0){
			infoType=PHY;
		} else {
			WCHAR buffer[256];
			wsprintf(buffer, L"WifiStatus.dll: Invalid type given, WifiInfoType=%d.",type);
			Log(buffer);
		}
		g_meas_data[id].type = infoType;
	}

	switch(infoType){
		case SSID:
		case ENCRYPTION:
		case AUTH:
			return 0;
		case QUALITY:
			return 100;
	}

	return 0;
}

/*
  This function is called when new value should be measured.
  The function returns the new value.
*/
UINT Update(UINT id)
{
	if(pInterface == NULL) return NULL;

	//Get measure id, and identify type
	//std::map<UINT, MEASURETYPE>::iterator typeIter = g_Types.find(id);
	//if(typeIter == g_Types.end()) return NULL;
	MEASURETYPE current_type = g_meas_data[id].type;
	switch(current_type)
	{
		case QUALITY:
			//Set up variables for WLAN query
			ULONG outsize = 0;
			PWLAN_CONNECTION_ATTRIBUTES wlan_cattr=NULL;
			DWORD dwErr;
			GUID& intfGUID = pInterface->InterfaceGuid;
			dwErr = WlanQueryInterface( hClient, &intfGUID, wlan_intf_opcode_current_connection, NULL, &outsize, (PVOID*)&wlan_cattr, NULL );
			if( ERROR_SUCCESS != dwErr){
				return 0;
			}
			int retval = (int)wlan_cattr->wlanAssociationAttributes.wlanSignalQuality;
			if(wlan_cattr!=NULL)WlanFreeMemory(wlan_cattr);
			return retval;
		
		//Transfer rates will go here
	}
	return NULL;
}


LPCTSTR GetString(UINT id, UINT flags) 
{
	if(pInterface == NULL) return NULL;
	
	//Some variables for data manipulation in this function
	static WCHAR buffer[128];
	bool bNetList = false; //whether to return buffer or netlist
	bool bInvalidType = false;
	bool bIntfError = false;
	unsigned int listStyle = 0;
	memset(buffer,'\0',128);
	listStyle = g_meas_data[id].listStyle;
	int printed = 0; //count of how many networks have been printed already

	//Set up variables for WLAN queries
	ULONG outsize = 0;
	PWLAN_CONNECTION_ATTRIBUTES wlan_cattr=NULL;
	PWLAN_AVAILABLE_NETWORK_LIST pwnl=NULL;
	DWORD dwCErr, dwLErr;
	GUID& intfGUID = pInterface->InterfaceGuid;

	//Initialize WLAN structs with queries, break if no interface found
	dwCErr= WlanQueryInterface( hClient, &intfGUID, wlan_intf_opcode_current_connection, NULL, &outsize, (PVOID*)&wlan_cattr, NULL );
	dwLErr= WlanGetAvailableNetworkList(hClient,&intfGUID,NULL,NULL,&pwnl);
	MEASURETYPE current_type = g_meas_data[id].type;
	UINT listMax = g_meas_data[id].listMax;

	switch(current_type)
	{   
		case LIST:
			if(ERROR_SUCCESS != dwLErr){return L"Error";}
			
			if (!g_meas_data[id].listInit){//Check if netlist has memory allocated already
				//Size of network name can be up to 64 chars, set to 80 to add room for  delimiters
				g_meas_data[id].netlist = (WCHAR*)malloc( 80 * sizeof(WCHAR) * g_meas_data[id].listMax);
				if(g_meas_data[id].netlist == NULL){
					WCHAR debug[256];
					wsprintf(debug, L"WifiStatus.dll: Unable to allocate memory for network list.");
					Log(buffer);
					g_meas_data[id].listInit = false;
					free(g_meas_data[id].netlist);
					return NULL;
				}
				g_meas_data[id].listInit = true;
			}
			
			memset(g_meas_data[id].netlist,'\0', (80 * sizeof(WCHAR) * g_meas_data[id].listMax));
			memset(buffer,'\0',128);
			
			//Check all items in WLAN NETWORK LIST
			for(int i=0; i < (int)pwnl->dwNumberOfItems ; i++){
				if(printed == g_meas_data[id].listMax)
					break;
				
				//SSID is in UCHAR, convert to WCHAR
				mbstowcs(buffer,(char*)pwnl->Network[i].dot11Ssid.ucSSID,pwnl->Network[i].dot11Ssid.uSSIDLength);
				
				//Prevent duplicates that result from profiles, check using SSID
				if((wcsstr(g_meas_data[id].netlist,buffer)== NULL)&&(_wcsicmp(L"", buffer) != 0)){
					printed++;
					if(listStyle > 0){
						wsprintf(g_meas_data[id].netlist,L"%s%s",g_meas_data[id].netlist,buffer);
						memset(buffer,'\0',128);
						if(listStyle == 1 || listStyle == 3){
							//ADD PHY type
							wsprintf(buffer,L" @%s", getDot11str(pwnl->Network[i].dot11PhyTypes[0],4)); 
						}
						if(listStyle == 2 || listStyle == 3){
							//ADD cipher and authentication
							wsprintf(buffer,L"%s (%s:%s)",buffer,getDot11str(pwnl->Network[i].dot11DefaultCipherAlgorithm,1)
																,getDot11str(pwnl->Network[i].dot11DefaultAuthAlgorithm,2)); 							
						}
					}
					wsprintf(g_meas_data[id].netlist,L"%s%s\n",g_meas_data[id].netlist,buffer);
				}
				memset(buffer,'\0',128);

			}//end for
			bNetList=true;
			break;
		
		case SSID:
			if(ERROR_SUCCESS != dwCErr){
				bIntfError = true;
				break;
			}
			//Need to convert ucSSID to wchar from uchar
			mbstowcs(buffer,(char *)wlan_cattr->wlanAssociationAttributes.dot11Ssid.ucSSID,wlan_cattr->wlanAssociationAttributes.dot11Ssid.uSSIDLength);
			//If not connected yet add current status
			wcscat(buffer,getDot11str(wlan_cattr->isState,3));
			break;
		
		case PHY:
			if(ERROR_SUCCESS != dwCErr){
				bIntfError = true;
				break;
			}
			wcscpy(buffer,getDot11str(wlan_cattr->wlanAssociationAttributes.dot11PhyType,4));
			break;

		case ENCRYPTION:
			if(ERROR_SUCCESS != dwCErr){
				bIntfError = true;
				break;
			}
			wcscpy(buffer,getDot11str(wlan_cattr->wlanSecurityAttributes.dot11CipherAlgorithm,1));
			break;

		case AUTH:
			if(ERROR_SUCCESS != dwCErr){
				bIntfError = true;
				break;
			}
			wcscpy(buffer,getDot11str(wlan_cattr->wlanSecurityAttributes.dot11AuthAlgorithm,2));					
			break;

		default: //InfoType does not refer to a string measure
			bInvalidType= true;
			break;
			
	}
	if(wlan_cattr!=NULL)WlanFreeMemory(wlan_cattr);
	if(pwnl!=NULL)WlanFreeMemory(pwnl);
	
	if(bNetList)
		return g_meas_data[id].netlist;
	if(bIntfError)
		return L"-1";
	else {
		if(bInvalidType)
			return NULL;
		else
			return buffer;
	}
}

/*
	switches from winlanapi.h + SDK 
	in: -DOT11 ENUM (converted to int)
	    -type of ENUM (cipher=1, auth=2, status=3, phy=4, otherwise=error strings)
	out: String to be returned by measure
*/
LPCTSTR getDot11str(int dot11enum,int type){
	if(type ==1){
		switch(dot11enum){
				case DOT11_CIPHER_ALGO_NONE:
					return L"NONE";
				case DOT11_CIPHER_ALGO_WEP40:
					return L"WEP40";
				case DOT11_CIPHER_ALGO_TKIP:
					return L"TKIP";
				case DOT11_CIPHER_ALGO_CCMP:
					return L"AES";
				case DOT11_CIPHER_ALGO_WEP104:
					return L"WEP104";
				case DOT11_CIPHER_ALGO_WPA_USE_GROUP:
					return L"WPA-GROUP";
				case DOT11_CIPHER_ALGO_WEP:
					return L"WEP";
				default:
					return L"???";	
		}
	}
	else if (type == 2){
		switch(dot11enum){
				case DOT11_AUTH_ALGO_80211_OPEN:
					return L"Open";
				case DOT11_AUTH_ALGO_80211_SHARED_KEY:
					return L"Shared";
				 case DOT11_AUTH_ALGO_WPA_NONE:
					return L"WPA-NONE";
				case DOT11_AUTH_ALGO_WPA:
					return L"WPA-Enterprise";
				case DOT11_AUTH_ALGO_WPA_PSK:
					return L"WPA-Personal";
				case DOT11_AUTH_ALGO_RSNA:
					return L"WPA2-Enterprise";
				case DOT11_AUTH_ALGO_RSNA_PSK:
					return L"WPA2-Personal";
				default:
					return L"???";				
		}
	}
	else if(type==3){
		switch(dot11enum){
				case wlan_interface_state_connected:
					return L"";
				case wlan_interface_state_authenticating:
					return L"(authorizing...)";
				default:
					return L"(connecting...)";
		}
	}
	else if(type==4){
		switch(dot11enum){
				case dot11_phy_type_unknown:
					return L"???";
				case dot11_phy_type_dsss:
					return L"DSSS";
				case dot11_phy_type_erp:
					return L"802.11g";					
				case dot11_phy_type_fhss:
					return L"FHSS";
				case dot11_phy_type_hrdsss:
					return L"802.11b";
				case dot11_phy_type_irbaseband:
					return L"IR-Band";
				case dot11_phy_type_ofdm:
					return L"802.11a";
				//Case below appears as dot11_phy_type_ht on MSDN
				//However its not supported in winlanapi.h ???
				case 7:
					return L"802.11n";
				default:
					return L"???";
		}
	}
	else{
		switch(dot11enum){
				case ERROR_INVALID_PARAMETER:
					return L"Invalid parameters.";
				case ERROR_NOT_ENOUGH_MEMORY:
					return L"Not enough memory.";
				case ERROR_REMOTE_SESSION_LIMIT_EXCEEDED:
					return L"Too many handles already issued.";
				default:
					return L"Unknown error code.";
		}
	}
}

/*
  If the measure needs to free resources before quitting.
  The plugin can export Finalize function, which is called
  when Rainmeter quits (or refreshes).
*/
void Finalize(HMODULE instance, UINT id)
{
	std::map<UINT, meas_data_t>::iterator i1 = g_meas_data.find(id);
	if (i1 != g_meas_data.end())
	{
		free(g_meas_data[id].netlist);
		g_meas_data[id].listInit = false;
		g_meas_data.erase(i1);
	}
	g_Instances--;
	if(hClient != NULL && g_Instances == 0){
		WlanCloseHandle(hClient, NULL);
		hClient = NULL;
	}
	if(pIntfList != NULL && g_Instances == 0){
		WlanFreeMemory(pIntfList);
		pIntfList = NULL;
	}
}

/*
  Wrapper function grabbed from the WebParser plugin
*/
void Log(const WCHAR* string)
{
	// @TODO: put logging into critical section
	LSLog(LOG_DEBUG, L"Rainmeter", string);
}

/*
  Returns the version number of the plugin. The value
  can be calculated like this: Major * 1000 + Minor.
  So, e.g. 2.31 would be 2031.
*/
UINT GetPluginVersion()
{
	return 1009;
}

/*
  Returns the author of the plugin for the about dialog.
*/
LPCTSTR GetPluginAuthor()
{
	return L"nvme (shaivya.m@gmail.com)";
}
