/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "AsyncTask.h"
#include "Rainmeter.h"

AsyncTask::AsyncTask(void* requestor) :
	m_Requestor(requestor),
	m_AbortRequested(false)
{
}

bool AsyncTask::Start()
{
	return QueueUserWorkItem(
		[](void* param) -> DWORD
		{
			auto task = (AsyncTask*)param;
			task->StartWorkOnWorkerThread();

			PostMessage(GetRainmeter().GetWindow(), WM_RAINMETER_HANDLE_ASYNC_TASK_RESULT, (WPARAM)task, 0);

			return 0;
		},
		this,
		0) != FALSE;
}

void AsyncTask::AbortWhenPossible()
{
	m_AbortRequested = true;
}

void AsyncTask::HandleResultMessage(WPARAM wParam, LPARAM lParam)
{
	auto task = (AsyncTask*)wParam;
	task->FinishWorkOnMainThread();

	delete task;
	task = nullptr;
}
