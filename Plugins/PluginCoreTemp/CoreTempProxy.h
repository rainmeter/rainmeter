#pragma once
#include "SharedMem.h"

#define UNKNOWN_EXCEPTION 0x20000000

class CoreTempProxy
{
public:
	CoreTempProxy(void);
	virtual ~CoreTempProxy(void);
	
	UINT GetCoreLoad(int Index) const;
	UINT GetTjMax(int Index) const;
	UINT GetCoreCount() const;
	UINT GetCPUCount() const;
	float GetTemp(int Index) const;
	float GetVID() const;
	float GetCPUSpeed() const;
	float GetFSBSpeed() const;
	float GetMultiplier() const;
	LPCSTR GetCPUName() const;
	bool IsFahrenheit() const;
	bool IsDistanceToTjMax() const;
	bool IsTdpSupported() const;
	bool IsPowerSupported() const;
	UINT GetStructureVersion() const;
	UINT GetTdp(int Index) const;
	float GetPower(int Index) const;
	float GetMultiplier(int Index) const;

	const CoreTempSharedDataEx &GetDataStruct() const;

	bool GetData();
	DWORD GetDllError() const { return GetLastError(); }
	LPCWSTR GetErrorMessage();

private:
	CSharedMemClient m_SharedMem;
	CoreTempSharedDataEx m_pCoreTempData;
	WCHAR m_ErrorMessage[100];
};
