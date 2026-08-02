// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

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
