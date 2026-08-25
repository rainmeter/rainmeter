// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "Measure.h"
#include <optional>

class MeasureUsageMonitor : public Measure
{
public:
	MeasureUsageMonitor(Skin* skin, const WCHAR* name);
	virtual ~MeasureUsageMonitor();

	MeasureUsageMonitor(const MeasureUsageMonitor& other) = delete;
	MeasureUsageMonitor& operator=(MeasureUsageMonitor other) = delete;

	UINT GetTypeID() override { return TypeID<MeasureUsageMonitor>(); }
	const WCHAR* GetStringValue() override;

	// Stops the background collectors. Must be called before the library is unloaded.
	static void FinalizeStatic();

	// What Index= asks the measure for. Everything other than Instance is about the instance list
	// as a whole rather than any one instance in it.
	enum class IndexType
	{
		Instance,
		Sum,
		Average,
		Count
	};

	enum class BlockType
	{
		None,
		Blacklist,
		Whitelist
	};

	// The part of the measure options that the background collectors need. A copy is handed to the
	// collector so that the worker threads never touch the measure itself.
	struct CounterOptions
	{
		size_t id = 0;                       // Identifies the option set, and nothing else
		std::wstring category;
		std::wstring counter;
		std::wstring blockKey;               // Identifies option sets that can share an instance list
		std::vector<std::wstring> blockList;
		BlockType blockType = BlockType::Blacklist;
		bool rollup = true;
		bool pidToName = false;
	};

	// One instance of a counter, with its values already made human readable.
	struct CounterInstance
	{
		std::wstring name;
		double value = 0.0;
		LONGLONG rawValue = 0;

		// The value of the "_Total" instance of the same counter, which Percent=1 scales against.
		// Nothing when the counter has been collected and genuinely has no such instance, which is
		// the only case worth warning about. Counters that have not been collected yet report 0.0
		// so that measures stay quiet until there is something to say.
		std::optional<double> total = 0.0;
	};

protected:
	void ReadOptions(ConfigParser& parser, std::wstring_view section) override;
	void UpdateValue() override;

private:
	// A collector has nothing to report until it has collected twice, which a measure with a high
	// UpdateDivider would otherwise sit out
	enum class State
	{
		Starting,
		WaitingForInitialValue,
		Normal
	};

	void RegisterMeasure();
	void UnregisterMeasure();
	void UpdatePidReference(bool needed);
	bool HasCollectedData();

	CounterOptions m_Options;
	CounterInstance m_CurrentInstance;

	std::wstring m_InstanceName;   // Name=
	IndexType m_IndexType;
	int m_Index;
	bool m_RawValue;
	bool m_Percent;

	bool m_Registered;

	State m_State;
	int m_OriginalUpdateDivider;

	// Keep a measure that reads its options every update from spamming the log
	bool m_RegisterFailed;
	bool m_LoggedNoTotal;
	bool m_PidReferenced;
};
