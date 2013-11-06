/*
  Copyright (C) 2013 Rainmeter Team

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

#ifndef RM_COMMON_TIMER_H_
#define RM_COMMON_TIMER_H_

#include <Windows.h>

// Performs timing using the high-resolution performance counter.
class Timer
{
public:
	void Start()
	{
		QueryPerformanceCounter(&m_Start);
	}

	void Stop()
	{
		QueryPerformanceCounter(&m_Stop);
	}

	// Returns the elapsed time in milliseconds.
	double GetElapsed() const
	{
		static LARGE_INTEGER s_Frequency = []()
		{
			LARGE_INTEGER frequency;
			QueryPerformanceFrequency(&frequency);
			return frequency;
		} ();

		return (m_Stop.QuadPart - m_Start.QuadPart) * 1000.0 / s_Frequency.QuadPart;
	}

private:
	LARGE_INTEGER m_Start;
	LARGE_INTEGER m_Stop;
};

#endif
