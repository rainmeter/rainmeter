#include <windows.h>
#include "SharedMem.h"

CSharedMemClient::CSharedMemClient(void)
{
}

CSharedMemClient::~CSharedMemClient(void)
{
}

bool CSharedMemClient::ReadSharedMem(PCORE_TEMP_SHARED_DATA i_SharedData)
{
	bool bRet = false;
	PCORE_TEMP_SHARED_DATA pSharedData;
	HANDLE hdlMemory;
	HANDLE hdlMutex;

	hdlMutex = CreateMutex(NULL,FALSE,CORE_TEMP_MUTEX_OBJECT);
	if (hdlMutex == NULL) 
	{		
		return false;
	} 
	
	WaitForSingleObject(hdlMutex, INFINITE);   
	
	hdlMemory = OpenFileMapping(
		FILE_MAP_READ,						// Read only permission. 
		TRUE,								
		CORE_TEMP_MAPPING_OBJECT);			// "CoreTempMappingObject"
	
	if (hdlMemory == NULL) 
	{	
		ReleaseMutex(hdlMutex); 
		return false;
	} 
	
	pSharedData = (PCORE_TEMP_SHARED_DATA)MapViewOfFile(hdlMemory, FILE_MAP_READ, 0, 0, 0);
	if (pSharedData == NULL) 
	{
		ReleaseMutex(hdlMutex); 
		CloseHandle(hdlMemory);
		hdlMemory = NULL;
		return false;
	} 

	__try
	{
		memcpy_s(i_SharedData, sizeof(core_temp_shared_data), pSharedData, sizeof(core_temp_shared_data));
		bRet = true;
	}
	__except(1)
	{
		bRet = false;
		SetLastError(0x20000000); //Unknown error
	}

	UnmapViewOfFile(pSharedData);
	CloseHandle(hdlMemory);
	ReleaseMutex(hdlMutex); 

	return bRet;
}