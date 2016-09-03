/* Copyright (C) 2001 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __MEASURECPU_H__
#define __MEASURECPU_H__

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
	virtual void ReadOptions(ConfigParser& parser, const WCHAR* section);
	virtual void UpdateValue();

private:
	void CalcUsage(double idleTime, double systemTime);

	int m_Processor;

	double m_OldTime[2];

	static FPNTQSI c_NtQuerySystemInformation;

	static int c_NumOfProcessors;
	static ULONG c_BufferSize;
};

#endif
