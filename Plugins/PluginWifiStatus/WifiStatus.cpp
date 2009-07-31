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


std::map<UINT, MEASURETYPE> g_Types;
std::map<UINT, UINT> g_ListStyle;
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
	PWLAN_INTERFACE_CAPABILITY pCapability = NULL;
	PWLAN_INTERFACE_INFO_LIST pIntfList = NULL;
	DWORD dwErr;
	//Create WINLAN API Handle
	if(hClient == NULL){
		dwErr = WlanOpenHandle( WLAN_API_VERSION, NULL, &dwNegotiatedVersion, &hClient );
		if( ERROR_SUCCESS != dwErr ){
			std::wstring error = L"Unable to open WLAN API Handle! (err code: "; 
			error+= getDot11str(dwErr,5);
			MessageBox(NULL, error.c_str(), L"Rainmeter", MB_OK);
			//return 0;
		}
	}
	//Query list of WLAN interfaces
	if(pIntfList == NULL){
		dwErr= WlanEnumInterfaces(hClient, NULL, &pIntfList);
		if(( ERROR_SUCCESS != dwErr) || (&pIntfList->dwNumberOfItems <= 0)){
			std::wstring error = L"Unable to find a valid WLAN interface/adapter! (err code: ";
			error+=(int) dwErr + L")";
			MessageBox(NULL, error.c_str(), L"Rainmeter", MB_OK);
			return 0;
		}
	}
	//Select a WLAN interface, default 0.
	LPCTSTR data = ReadConfigString(section, L"WifiIntfID", L"");
	
	if ((data != NULL) && (wcsicmp(L"", data) != 0)){
		if(_wtoi(data) < (int)pIntfList->dwNumberOfItems){
			pInterface = &pIntfList->InterfaceInfo[_wtoi(data)];
		} else {
			std::wstring error = L"Invalid WifiIntfID given (defaulting to 0). WifiIntfID=";
			error+=data;
			MessageBox(NULL, error.c_str(), L"Rainmeter", MB_OK);
			pInterface = &pIntfList->InterfaceInfo[0]; 
		}		
	} else {
		pInterface = &pIntfList->InterfaceInfo[0]; 
	}
	//Select LIST style
	data = ReadConfigString(section, L"WifiListStyle", L"");
	
	if ((data != NULL) && (wcsicmp(L"", data) != 0)){
		if ( (_wtoi(data) >= 0) && (_wtoi(data) <= 3)){
			g_ListStyle[id] = _wtoi(data);
		} else {
			std::wstring error = L"Invalid WifiListStyle given (defaulting to 0). WifiListStyle=";
			error+=data;
			MessageBox(NULL, error.c_str(), L"Rainmeter", MB_OK);
			g_ListStyle[id] = 0;
		}		
	} else {
		g_ListStyle[id] = 0;
	}
	//Select type of measure
	MEASURETYPE infoType = UNKNOWN;
	LPCTSTR type = ReadConfigString(section, L"WifiInfoType", L"");
	if(type){
		if (wcsicmp(L"SSID", type) == 0){
			infoType=SSID;
		} 
		else if (wcsicmp(L"QUALITY", type) == 0){
			infoType=QUALITY;
		} 
		else if (wcsicmp(L"ENCRYPTION", type) == 0){
			infoType=ENCRYPTION;
		} 
		else if (wcsicmp(L"AUTH", type) == 0){
			infoType=AUTH;
		} 
		else if (wcsicmp(L"LIST", type) == 0){
			infoType=LIST;
		} 
		else if (wcsicmp(L"PHY", type) == 0){
			infoType=PHY;
		} else {
			std::wstring error = L"No such WifiInfoType: ";
			error += type;
			MessageBox(NULL, error.c_str(), L"Rainmeter", MB_OK);
		}
		g_Types[id] = infoType;
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
	std::map<UINT, MEASURETYPE>::iterator typeIter = g_Types.find(id);
	if(typeIter == g_Types.end()) return NULL;
	switch((*typeIter).second)
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
	
	//@TODO - Netlist needs to be dynamically allocated + renewed
	static WCHAR buffer[256];
	static WCHAR netlist[1024];//stores current list of available networks
	bool bNetList = false; //whether to return buffer or netlist
	bool invalidType= false;
	unsigned int listStyle = 0;
	memset(buffer,'\0',256);
	
	std::map<UINT, MEASURETYPE>::iterator typeIter = g_Types.find(id);
	if(typeIter == g_Types.end()) return NULL;
	
	std::map<UINT, UINT>::iterator verboseIter = g_ListStyle.find(id);
	if(verboseIter == g_ListStyle.end()) return NULL;
	listStyle = (*verboseIter).second;
	//Set up variables for WLAN queries
	ULONG outsize = 0;
	PWLAN_CONNECTION_ATTRIBUTES wlan_cattr=NULL;
	PWLAN_AVAILABLE_NETWORK_LIST pwnl=NULL;
	DWORD dwCErr, dwLErr;
	GUID& intfGUID = pInterface->InterfaceGuid;

	//Initialize WLAN structs with queries
	dwCErr= WlanQueryInterface( hClient, &intfGUID, wlan_intf_opcode_current_connection, NULL, &outsize, (PVOID*)&wlan_cattr, NULL );
	dwLErr= WlanGetAvailableNetworkList(hClient,&intfGUID,NULL,NULL,&pwnl);

	switch((*typeIter).second)
	{   
		case LIST:
			if(ERROR_SUCCESS != dwLErr){return NULL;}
			memset(netlist,'\0',1024);
			memset(buffer,'\0',256);
			//Check all items in WLAN NETWORK LIST
			for(int i=0; i < (int)pwnl->dwNumberOfItems ; i++){
				//SSID is in UCHAR, convert to WCHAR
				mbstowcs(buffer,(char*)pwnl->Network[i].dot11Ssid.ucSSID,pwnl->Network[i].dot11Ssid.uSSIDLength);
				//Prevent duplicats that result from profiles
				if((wcsstr(netlist,buffer)== NULL)&&(wcsicmp(L"", buffer) != 0)){
					//Add an SSID to list
					wcscat(netlist,buffer);
					//Add PHY type
					memset(buffer,'\0',256);
					if(listStyle > 0){
						if(listStyle == 1 || listStyle == 3){
							wcscat(netlist,L" @ ");
							wcscpy(buffer,getDot11str(pwnl->Network[i].dot11PhyTypes[0],4));
							wcscat(netlist,buffer);
							//Add Cipher type
							memset(buffer,'\0',256);
						}
						if(listStyle == 2 || listStyle == 3){
							wcscat(netlist,L" (");
							wcscpy(buffer,getDot11str(pwnl->Network[i].dot11DefaultCipherAlgorithm,1));
							wcscat(netlist,buffer);
							memset(buffer,'\0',256);
							//Add Auth type
							wcscat(netlist,L":");
							wcscpy(buffer,getDot11str(pwnl->Network[i].dot11DefaultAuthAlgorithm,2));
							wcscat(netlist,buffer);
							wcscat(netlist,L")");
						}
					}
					wcscat(netlist,L"\n");
				}
				memset(buffer,'\0',256);

			}//end for
			bNetList=TRUE;
			break;
		
		case SSID:
			if(ERROR_SUCCESS != dwCErr){return NULL;}
			//Need to convert ucSSID to wchar from uchar
			mbstowcs(buffer,(char *)wlan_cattr->wlanAssociationAttributes.dot11Ssid.ucSSID,wlan_cattr->wlanAssociationAttributes.dot11Ssid.uSSIDLength);
			//If not connected yet add current status
			wcscat(buffer,getDot11str(wlan_cattr->isState,3));
			break;
		
		case PHY:
			if(ERROR_SUCCESS != dwCErr){return NULL;}
			wcscpy(buffer,getDot11str(wlan_cattr->wlanAssociationAttributes.dot11PhyType,4));
			break;

		case ENCRYPTION:
			if(ERROR_SUCCESS != dwCErr){return NULL;}
			wcscpy(buffer,getDot11str(wlan_cattr->wlanSecurityAttributes.dot11CipherAlgorithm,1));
			break;

		case AUTH:
			if(ERROR_SUCCESS != dwCErr){return NULL;}
			wcscpy(buffer,getDot11str(wlan_cattr->wlanSecurityAttributes.dot11AuthAlgorithm,2));					
			break;

		default: //InfoType does not refer to a string measure
			invalidType= true;
			break;
			
	}
	if(wlan_cattr!=NULL)WlanFreeMemory(wlan_cattr);
	if(pwnl!=NULL)WlanFreeMemory(pwnl);
	if(bNetList)return netlist;
	if(invalidType)
		return NULL;
	else
		return buffer;
}

/*
	switches from winlanapi.h + SDK 
	in: -DOT11 ENUM (converted to int)
	    -type of ENUM (cipher=1, auth=2, status=3, phy=4)
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
					return L"n";
				default:
					return L"???";
		}
	}
	else{
		switch(dot11enum){
				case ERROR_INVALID_PARAMETER:
					return L"Invalid parameters";
				case ERROR_NOT_ENOUGH_MEMORY:
					return L"Not enough memory";
				case ERROR_REMOTE_SESSION_LIMIT_EXCEEDED:
					return L"Too many handles already issued";
				default:
					return L"Windows just hates you";
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
	std::map<UINT, MEASURETYPE>::iterator i1 = g_Types.find(id);
	if (i1 != g_Types.end())
	{
		g_Types.erase(i1);
	}
	if(hClient != NULL){
		WlanCloseHandle(hClient, NULL);
		hClient = NULL;
	}
	if(pIntfList != NULL){
		WlanFreeMemory(pIntfList);
		hClient = NULL;
	}
}

/*
  Returns the version number of the plugin. The value
  can be calculated like this: Major * 1000 + Minor.
  So, e.g. 2.31 would be 2031.
*/
UINT GetPluginVersion()
{
	return 1008;
}

/*
  Returns the author of the plugin for the about dialog.
*/
LPCTSTR GetPluginAuthor()
{
	return L"nvme (shaivya.m@gmail.com)";
}
