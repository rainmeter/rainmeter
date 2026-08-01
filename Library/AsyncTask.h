/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_LIBRARY_ASYNCTASK_H_
#define RM_LIBRARY_ASYNCTASK_H_

#include <Windows.h>
#include <atomic>

class __declspec(novtable) AsyncTask
{
public:
	static void HandleResultMessage(WPARAM wParam, LPARAM lParam);

	bool Start();
	void AbortWhenPossible();

protected:
	AsyncTask(void* requestor);
	virtual ~AsyncTask() {}

	virtual void StartWorkOnWorkerThread() = 0;
	virtual void FinishWorkOnMainThread() = 0;

	void* m_Requestor;
	std::atomic<bool> m_AbortRequested;

};

#endif
