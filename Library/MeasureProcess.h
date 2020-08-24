/* Copyright (C) 2020 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_LIBRARY_MEASUREPROCESS_H_
#define RM_LIBRARY_MEASUREPROCESS_H_

#include "Measure.h"
#include <thread>
#include <mutex>
#include <future>

class MeasureProcess : public Measure
{
public:
	MeasureProcess(Skin* skin, const WCHAR* name);
	virtual ~MeasureProcess();

	MeasureProcess(const MeasureProcess& other) = delete;
	MeasureProcess& operator=(MeasureProcess other) = delete;

	UINT GetTypeID() override { return TypeID<MeasureProcess>(); }

protected:
	void ReadOptions(ConfigParser& parser, const WCHAR* section) override;
	void UpdateValue() override;

private:
	static void MonitorProcesses();

	std::wstring m_ProcessName;

	static std::vector<std::wstring> c_Processes;
	static std::thread c_ProcessThread;
	static std::promise<void> c_ProcessExitSignal;
	static std::future<void> c_ProcessFuture;
	static std::mutex c_ProcessMutex;

	static UINT c_References;
	static int c_UpdateInterval;
};

#endif
