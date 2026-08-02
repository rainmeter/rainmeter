// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>
#include <Iphlpapi.h>
#include "Measure.h"

class MeasureNet : public Measure
{
public:
	virtual UINT GetTypeID() { return TypeID<MeasureNet>(); }

	static void UpdateIFTable();

	static void UpdateStats();
	static void ResetStats();
	static void ReadStats(const std::wstring& iniFile, std::wstring& statsDate);
	static void WriteStats(const WCHAR* iniFile, const std::wstring& statsDate);

	static void FinalizeStatic();

protected:
	enum NET
	{
		NET_IN,
		NET_OUT,
		NET_TOTAL
	};

	MeasureNet(Skin* skin, const WCHAR* name, NET type);
	virtual ~MeasureNet();

	MeasureNet(const MeasureNet& other) = delete;
	MeasureNet& operator=(MeasureNet other) = delete;

	virtual void ReadOptions(ConfigParser& parser, const WCHAR* section);
	virtual void UpdateValue();

private:
	ULONG64 GetNetOctets(NET net);
	ULONG64 GetNetStatsValue(NET net);

	NET m_Net;
	ULONG m_Interface;

	ULONG64 m_Octets;
	bool m_FirstTime;
	bool m_Cumulative;
	bool m_UseBits;

	static std::vector<ULONG64> c_OldStatValues;
	static std::vector<ULONG64> c_StatValues;
};
