#pragma once
#include "SharedMem.h"

#define UNKNOWN_EXCEPTION 0x20000000

class CoreTempProxy
{
public:
	CoreTempProxy(void);
	virtual ~CoreTempProxy(void);
	
	UINT GetCoreLoad(int i_Index) const;
    UINT GetTjMax(int i_Index) const;
    UINT GetCoreCount() const;
    UINT GetCPUCount() const;
    float GetTemp(int i_Index) const;
    float GetVID() const;
    float GetCPUSpeed() const;
    float GetFSBSpeed() const;
    float GetMultiplier() const;
    LPCSTR GetCPUName() const;
    bool IsFahrenheit() const;
    bool IsDistanceToTjMax() const;
    const CORE_TEMP_SHARED_DATA &GetDataStruct() const;

	bool GetData();
	DWORD GetDllError() const { return GetLastError(); }
	LPCWSTR GetErrorMessage();
private:

	CSharedMemClient m_SharedMem;
	CORE_TEMP_SHARED_DATA m_pCoreTempData;
	WCHAR m_ErrorMessage[100];
};
