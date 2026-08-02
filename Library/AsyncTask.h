// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

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
