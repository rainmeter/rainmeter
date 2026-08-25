// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

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
