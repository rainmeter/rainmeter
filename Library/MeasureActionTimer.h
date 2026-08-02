// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "../Common/CriticalSection.h"
#include "Measure.h"

class MeasureActionTimer : public Measure
{
public:
	MeasureActionTimer(Skin* skin, const WCHAR* name);
	virtual ~MeasureActionTimer();

	MeasureActionTimer(const MeasureActionTimer& other) = delete;
	MeasureActionTimer& operator=(MeasureActionTimer other) = delete;

	UINT GetTypeID() override { return TypeID<MeasureActionTimer>(); }

	static void HandleExecuteMessage(WPARAM wParam, LPARAM lParam);

protected:
	void ReadOptions(ConfigParser& parser, const WCHAR* section) override;
	void UpdateValue() override {};
	void Command(const std::wstring& command) override;

private:
	class ActionTimerTask;

	struct Action
	{
		std::vector<std::wstring> commands;
		ActionTimerTask* task = nullptr;
		size_t generation = 0;
	};

	struct SharedData
	{
		SharedData(Skin* skin) : skin(skin) {}

		Skin* skin;
		std::vector<Action> actions;
		std::shared_ptr<CriticalSection> criticalSection = std::make_shared<CriticalSection>();
		bool active = true;
	};

	struct ExecuteMessage
	{
		std::weak_ptr<SharedData> data;
		size_t actionIndex;
		size_t commandIndex;
		size_t generation;
	};

	std::shared_ptr<SharedData> m_Data;

	bool m_IgnoreWarnings;
};
