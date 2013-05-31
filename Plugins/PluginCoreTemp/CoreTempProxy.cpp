#include <windows.h>
#include "CoreTempProxy.h"

CoreTempProxy::CoreTempProxy(void)
{
	memset(&this->m_pCoreTempData, 0, sizeof(CORE_TEMP_SHARED_DATA));
}

CoreTempProxy::~CoreTempProxy(void)
{
}

UINT CoreTempProxy::GetCoreLoad(int i_Index) const
{
	return this->m_pCoreTempData.uiLoad[i_Index];
}

UINT CoreTempProxy::GetTjMax(int i_Index) const
{
	return this->m_pCoreTempData.uiTjMax[i_Index];
}

UINT CoreTempProxy::GetCoreCount() const
{
	return this->m_pCoreTempData.uiCoreCnt;
}

UINT CoreTempProxy::GetCPUCount() const
{
	return this->m_pCoreTempData.uiCPUCnt;
}

float CoreTempProxy::GetTemp(int i_Index) const
{
	return this->m_pCoreTempData.fTemp[i_Index];
}

float CoreTempProxy::GetVID() const
{
	return this->m_pCoreTempData.fVID;
}

float CoreTempProxy::GetCPUSpeed() const
{
	return this->m_pCoreTempData.fCPUSpeed;
}

float CoreTempProxy::GetFSBSpeed() const
{
	return this->m_pCoreTempData.fFSBSpeed;
}

float CoreTempProxy::GetMultiplier() const
{
	return this->m_pCoreTempData.fMultipier;
}

LPCSTR CoreTempProxy::GetCPUName() const
{
	return this->m_pCoreTempData.sCPUName;
}

bool CoreTempProxy::IsFahrenheit() const
{
	return this->m_pCoreTempData.ucFahrenheit != 0;
}

bool CoreTempProxy::IsDistanceToTjMax() const
{
	return this->m_pCoreTempData.ucDeltaToTjMax != 0;
}

const CORE_TEMP_SHARED_DATA &CoreTempProxy::GetDataStruct() const
{
	return this->m_pCoreTempData;
}

bool CoreTempProxy::GetData()
{
	return this->m_SharedMem.ReadSharedMem(&this->m_pCoreTempData);
}

LPCWSTR CoreTempProxy::GetErrorMessage()
{
	DWORD lastError;

	lastError = ::GetLastError();
	if ((lastError & UNKNOWN_EXCEPTION) > 0)
	{
		wcscpy_s(this->m_ErrorMessage, L"Unknown error occured while copying shared memory.");
	}
	else
	{
		::FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, lastError, 0, this->m_ErrorMessage, 99, nullptr);
	}

	return this->m_ErrorMessage;
}