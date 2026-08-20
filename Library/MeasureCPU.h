// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "Measure.h"

typedef LONG (WINAPI *FPNTQSI)(UINT, PVOID, ULONG, PULONG);

class MeasureCPU : public Measure
{
public:
	MeasureCPU(Skin* skin, const WCHAR* name);
	virtual ~MeasureCPU();

	MeasureCPU(const MeasureCPU& other) = delete;
	MeasureCPU& operator=(MeasureCPU other) = delete;

	virtual UINT GetTypeID() { return TypeID<MeasureCPU>(); }

	static void InitializeStatic();
	static void FinalizeStatic();

protected:
	void ReadOptions(ConfigParser& parser, std::wstring_view section) override;
	virtual void UpdateValue();

private:
	void CalcUsage(double idleTime, double systemTime);

	int m_Processor;

	double m_OldTime[2];

	static FPNTQSI c_NtQuerySystemInformation;

	static int c_NumOfProcessors;
	static ULONG c_BufferSize;
};
