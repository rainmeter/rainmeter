/* Copyright (C) 2013 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

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
