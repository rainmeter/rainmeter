/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_COMMON_CRITICALSECTION_H_
#define RM_COMMON_CRITICALSECTION_H_

#include <Windows.h>

class CriticalSection
{
public:
	CriticalSection()
	{
		// See http://stackoverflow.com/questions/804848/critical-sections-leaking-memory-on-vista-win2008/
		if (InitializeCriticalSectionEx(&m_CriticalSection, 0, CRITICAL_SECTION_NO_DEBUG_INFO) == FALSE)
		{
			InitializeCriticalSectionAndSpinCount(&m_CriticalSection, 0);
		}
	}

	~CriticalSection()
	{
		DeleteCriticalSection(&m_CriticalSection);
	}

	void Enter()
	{
		EnterCriticalSection(&m_CriticalSection);
	}

	void Leave()
	{
		LeaveCriticalSection(&m_CriticalSection);
	}

	bool TryEnter()
	{
		return TryEnterCriticalSection(&m_CriticalSection) != FALSE;
	}

	CriticalSection(const CriticalSection&) = delete;
	CriticalSection& operator=(const CriticalSection&) = delete;

private:
	CRITICAL_SECTION m_CriticalSection;
};

class CriticalSectionLock
{
public:
	CriticalSectionLock(CriticalSection& criticalSection) : m_CriticalSection(criticalSection)
	{
		m_CriticalSection.Enter();
	}

	~CriticalSectionLock()
	{
		m_CriticalSection.Leave();
	}

	CriticalSectionLock(const CriticalSectionLock&) = delete;
	CriticalSectionLock& operator=(const CriticalSectionLock&) = delete;

private:
	CriticalSection& m_CriticalSection;
};

#endif
