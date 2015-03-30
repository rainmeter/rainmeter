/*
Copyright (C) 2015 Brian Ferguson

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
*/

#ifndef __MEASURELOOP_H__
#define __MEASURELOOP_H__

#include "Measure.h"

class MeasureLoop : public Measure
{
public:
	MeasureLoop(Skin* skin, const WCHAR* name);
	virtual ~MeasureLoop();

	MeasureLoop(const MeasureLoop& other) = delete;
	MeasureLoop& operator=(MeasureLoop other) = delete;

	virtual void Command(const std::wstring& command);

	virtual UINT GetTypeID() { return TypeID<MeasureLoop>(); }

protected:
	virtual void ReadOptions(ConfigParser& parser, const WCHAR* section);
	virtual void UpdateValue();

private:
	void Reset();

	int m_StartValue;
	int m_EndValue;
	int m_Increment;

	int m_LoopCount;
	int m_LoopCounter;

	bool m_SkipFirst;
	bool m_HasOverRun;
};

#endif
