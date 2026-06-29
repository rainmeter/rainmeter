/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "MeasureCoreTemp.h"
#include "CoreTemp/CoreTempProxy.h"
#include "ConfigParser.h"
#include "Logger.h"

class CoreTempProxy;

CoreTempProxy& GetCoreTempProxy()
{
	static CoreTempProxy s_Proxy;
	return s_Proxy;
}

MeasureCoreTemp::MeasureCoreTemp(Skin* skin, const WCHAR* name) : Measure(skin, name),
	m_Type(Type::Temperature),
	m_Index(0),
	m_StringValue()
{
}

MeasureCoreTemp::~MeasureCoreTemp()
{
}

void MeasureCoreTemp::ReadOptions(ConfigParser& parser, const WCHAR* section)
{
	Measure::ReadOptions(parser, section);

	m_Type = ConvertType(parser.ReadString(section, L"CoreTempType", L"Temperature").c_str());
	m_Index = parser.ReadInt(section, L"CoreTempIndex", 0);
}

void MeasureCoreTemp::UpdateValue()
{
	m_Value = 0.0;

	auto& proxy = GetCoreTempProxy();
	if (!proxy.GetData())
	{
		return;
	}

	switch (m_Type)
	{
	case Type::Temperature:
		m_Value = proxy.GetTemp(m_Index);
		break;

	case Type::MaxTemperature:
		m_Value = GetHighestTemp();
		break;

	case Type::TjMax:
		m_Value = proxy.GetTjMax(m_Index);
		break;

	case Type::Load:
		m_Value = proxy.GetCoreLoad(m_Index);
		break;

	case Type::Vid:
		m_Value = proxy.GetVID();
		break;

	case Type::CpuSpeed:
		m_Value = proxy.GetCPUSpeed();
		break;

	case Type::BusSpeed:
		m_Value = proxy.GetFSBSpeed();
		break;

	case Type::BusMultiplier:
		m_Value = proxy.GetMultiplier();
		break;

	case Type::CpuName:
		break;

	case Type::CoreSpeed:
		m_Value = proxy.GetMultiplier(m_Index) * proxy.GetFSBSpeed();
		break;

	case Type::CoreBusMultiplier:
		m_Value = proxy.GetMultiplier(m_Index);
		break;

	case Type::Tdp:
		m_Value = proxy.GetTdp(m_Index);
		break;

	case Type::Power:
		m_Value = proxy.GetPower(m_Index);
		break;
	}
}

const WCHAR* MeasureCoreTemp::GetStringValue()
{
	auto& proxy = GetCoreTempProxy();
	switch (m_Type)
	{
	case Type::Vid:
		_snwprintf_s(m_StringValue, _TRUNCATE, L"%.4f", proxy.GetVID());
		return CheckSubstitute(m_StringValue);

	case Type::CpuName:
		_snwprintf_s(m_StringValue, _TRUNCATE, L"%S", proxy.GetCPUName());
		return CheckSubstitute(m_StringValue);
	}

	return nullptr;
}

MeasureCoreTemp::Type MeasureCoreTemp::ConvertType(const WCHAR* type)
{
	if (_wcsicmp(type, L"Temperature") == 0)
	{
		return Type::Temperature;
	}
	else if (_wcsicmp(type, L"MaxTemperature") == 0)
	{
		return Type::MaxTemperature;
	}
	else if (_wcsicmp(type, L"TjMax") == 0)
	{
		return Type::TjMax;
	}
	else if (_wcsicmp(type, L"Load") == 0)
	{
		return Type::Load;
	}
	else if (_wcsicmp(type, L"Vid") == 0)
	{
		return Type::Vid;
	}
	else if (_wcsicmp(type, L"CpuSpeed") == 0)
	{
		return Type::CpuSpeed;
	}
	else if (_wcsicmp(type, L"BusSpeed") == 0)
	{
		return Type::BusSpeed;
	}
	else if (_wcsicmp(type, L"BusMultiplier") == 0)
	{
		return Type::BusMultiplier;
	}
	else if (_wcsicmp(type, L"CpuName") == 0)
	{
		return Type::CpuName;
	}
	else if (_wcsicmp(type, L"CoreSpeed") == 0)
	{
		return Type::CoreSpeed;
	}
	else if (_wcsicmp(type, L"CoreBusMultiplier") == 0)
	{
		return Type::CoreBusMultiplier;
	}
	else if (_wcsicmp(type, L"Tdp") == 0)
	{
		return Type::Tdp;
	}
	else if (_wcsicmp(type, L"Power") == 0)
	{
		return Type::Power;
	}

	LogWarningF(this, L"CoreTemp: CoreTempType=%s is not valid", type);
	return Type::Temperature;
}

float MeasureCoreTemp::GetHighestTemp() const
{
	auto& proxy = GetCoreTempProxy();
	float temp = -255.0f;
	const UINT coreCount = proxy.GetCoreCount();
	for (UINT i = 0; i < coreCount; ++i)
	{
		const float coreTemp = proxy.GetTemp(i);
		if (temp < coreTemp)
		{
			temp = coreTemp;
		}
	}

	return temp;
}
