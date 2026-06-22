/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "MeasureSpeedFan.h"
#include "ConfigParser.h"
#include "Logger.h"

#pragma pack(push, 1)
struct SpeedFanData
{
	WORD version;
	WORD flags;
	INT memSize;
	DWORD handle;
	WORD numTemps;
	WORD numFans;
	WORD numVolts;
	INT temps[32];
	INT fans[32];
	INT volts[32];
};
#pragma pack(pop)

MeasureSpeedFan::MeasureSpeedFan(Skin* skin, const WCHAR* name) : Measure(skin, name),
	m_Type(SensorType::Temperature),
	m_Scale(ScaleType::Source),
	m_Number()
{
}

MeasureSpeedFan::~MeasureSpeedFan()
{
}

void MeasureSpeedFan::ReadOptions(ConfigParser& parser, const WCHAR* section)
{
	Measure::ReadOptions(parser, section);

	const std::wstring type = parser.ReadString(section, L"SpeedFanType", L"TEMPERATURE");
	if (_wcsicmp(L"TEMPERATURE", type.c_str()) == 0)
	{
		m_Type = SensorType::Temperature;

		const std::wstring scale = parser.ReadString(section, L"SpeedFanScale", L"C");
		if (_wcsicmp(L"C", scale.c_str()) == 0)
		{
			m_Scale = ScaleType::Celsius;
		}
		else if (_wcsicmp(L"F", scale.c_str()) == 0)
		{
			m_Scale = ScaleType::Fahrenheit;
		}
		else if (_wcsicmp(L"K", scale.c_str()) == 0)
		{
			m_Scale = ScaleType::Kelvin;
		}
		else
		{
			LogErrorF(this, L"SpeedFan: SpeedFanScale=%s is not valid", scale.c_str());
		}
	}
	else if (_wcsicmp(L"FAN", type.c_str()) == 0)
	{
		m_Type = SensorType::Fan;
	}
	else if (_wcsicmp(L"VOLTAGE", type.c_str()) == 0)
	{
		m_Type = SensorType::Voltage;
	}
	else
	{
		LogErrorF(this, L"SpeedFan: SpeedFanType=%s is not valid", type.c_str());
	}

	m_Number = parser.ReadUInt(section, L"SpeedFanNumber", 0U);
}

void MeasureSpeedFan::UpdateValue()
{
	double value = 0.0;
	ReadSharedData(&value);
	m_Value = value;
}

void MeasureSpeedFan::ReadSharedData(double* value)
{
	HANDLE data = OpenFileMapping(FILE_MAP_READ, FALSE, L"SFSharedMemory_ALM");
	if (!data)
	{
		return;
	}

	SpeedFanData* speedFanData = (SpeedFanData*)MapViewOfFile(data, FILE_MAP_READ, 0, 0, 0);
	if (!speedFanData)
	{
		CloseHandle(data);
		return;
	}

	if (speedFanData->version == 1)
	{
		switch (m_Type)
		{
		case SensorType::Temperature:
			if (m_Number < speedFanData->numTemps)
			{
				*value = speedFanData->temps[m_Number] / 100.0;

				if (m_Scale == ScaleType::Fahrenheit)
				{
					*value *= 1.8;
					*value += 32.0;
				}
				else if (m_Scale == ScaleType::Kelvin)
				{
					*value += 273.15;
				}
			}
			break;

		case SensorType::Fan:
			if (m_Number < speedFanData->numFans)
			{
				*value = speedFanData->fans[m_Number];
			}
			break;

		case SensorType::Voltage:
			if (m_Number < speedFanData->numVolts)
			{
				*value = speedFanData->volts[m_Number] / 100.0;
			}
			break;
		}
	}
	else
	{
		LogErrorF(this, L"SpeedFan: Incorrect shared memory version");
	}

	UnmapViewOfFile(speedFanData);
	CloseHandle(data);
}
