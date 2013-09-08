/*
  Copyright (C) 2013 NAME

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

  Open Hardware Monitor code is subject to the Mozilla Public
  License:

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0.  If a copy of the MPL was not distributed with
  this file, You can obtain one at http://mozilla.org/MPL/2.0/.

  The original source code for this program is the project Open
  Hardware Monitor (http://openhardwaremonitor.org/).

  The intent of this inclusion is to allow Rainmeter to retrieve
  hardware related information without requiring another program
  to run in the background.
*/

#ifndef __MEASUREGPU_H__
#define __MEASUREGPU_H__

#include "Measure.h"

class MeasureGPU : public Measure
{
public:
	MeasureGPU(MeterWindow* meterWindow, const WCHAR* name);
	virtual ~MeasureGPU();

	virtual UINT GetTypeID() { return TypeID<MeasureGPU>(); }

	static void InitializeStatic();
	static void FinalizeStatic();

protected:
	virtual void ReadOptions(ConfigParser& parser, const WCHAR* section);
	virtual void UpdateValue();

private:
	void CalcUsage(double idleTime, double systemTime);

	int m_Processor;

	double m_OldTime[2];

	bool m_bOHM_Plugin_Available;

	static int c_NumOfProcessors;
	static ULONG c_BufferSize;
};

#endif
