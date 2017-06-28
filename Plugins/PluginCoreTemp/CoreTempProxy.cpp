#include <windows.h>
#include "CoreTempProxy.h"

CoreTempProxy::CoreTempProxy(void)
{
	memset(&this->m_pCoreTempData, 0, sizeof(CoreTempSharedDataEx));
}

CoreTempProxy::~CoreTempProxy(void)
{
}

UINT CoreTempProxy::GetCoreLoad(int _index) const
{
	return this->m_pCoreTempData.uiLoad[_index];
}

UINT CoreTempProxy::GetTjMax(int _index) const
{
	return this->m_pCoreTempData.uiTjMax[_index];
}

UINT CoreTempProxy::GetCoreCount() const
{
	return this->m_pCoreTempData.uiCoreCnt;
}

UINT CoreTempProxy::GetCPUCount() const
{
	return this->m_pCoreTempData.uiCPUCnt;
}

float CoreTempProxy::GetTemp(int _index) const
{
	return this->m_pCoreTempData.fTemp[_index];
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
	return this->m_pCoreTempData.fMultiplier;
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

bool CoreTempProxy::IsTdpSupported() const
{
	return this->m_pCoreTempData.ucTdpSupported != 0;
}

bool CoreTempProxy::IsPowerSupported() const
{
	return this->m_pCoreTempData.ucPowerSupported != 0;
}

UINT CoreTempProxy::GetStructureVersion() const
{
	return this->m_pCoreTempData.uiStructVersion;
}

UINT CoreTempProxy::GetTdp(int _index) const
{
	return this->m_pCoreTempData.uiTdp[_index];
}

float CoreTempProxy::GetPower(int _index) const
{
	return this->m_pCoreTempData.fPower[_index];
}

float CoreTempProxy::GetMultiplier(int _index) const
{
	return this->m_pCoreTempData.fMultipliers[_index];
}

const CoreTempSharedDataEx &CoreTempProxy::GetDataStruct() const
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
		::FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, lastError, 0, this->m_ErrorMessage, 99, NULL);
	}

	return this->m_ErrorMessage;
}