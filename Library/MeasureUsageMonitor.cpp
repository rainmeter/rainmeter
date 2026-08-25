// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "MeasureUsageMonitor.h"
#include "ConfigParser.h"
#include "Logger.h"
#include "../Common/CriticalSection.h"
#include "../Common/PdhUtil.h"
#include <pdh.h>

namespace {

using BlockType = MeasureUsageMonitor::BlockType;
using CounterInstance = MeasureUsageMonitor::CounterInstance;
using CounterOptions = MeasureUsageMonitor::CounterOptions;

// Performance data is not refreshed any faster than this, so there is nothing to be gained from
// collecting more often than PerfMon itself does.
const DWORD c_UpdateRate = 1000;

// Translating the instance names of the GPU counters needs a process ID to name mapping, which is
// itself read from the "Process" category. It is shared by every measure that asks for it.
CriticalSection g_PidLock;         // Guards the mapping
CriticalSection g_PidUpdateLock;   // Keeps collections from overlapping
ankerl::unordered_dense::map<DWORD, std::wstring> g_Pids;
PDH_HQUERY g_PidQuery = nullptr;
PDH_HCOUNTER g_PidCounter = nullptr;
HANDLE g_PidTimer = nullptr;
UINT g_PidReferences = 0;

void UpdatePids()
{
	CriticalSectionTryLock lock(g_PidUpdateLock);
	if (!lock) return;

	// The query is only ever touched here, and the timer is always torn down before it is closed
	if (PdhCollectQueryData(g_PidQuery) != ERROR_SUCCESS) return;

	// Kept between collections for the same reason the collectors keep theirs
	static std::vector<BYTE> s_Buffer;

	DWORD count = 0;
	if (!PdhUtil::GetRawArray(g_PidCounter, s_Buffer, count)) return;

	const auto items = (const PDH_RAW_COUNTER_ITEM*)s_Buffer.data();

	ankerl::unordered_dense::map<DWORD, std::wstring> pids;
	pids.reserve(count);

	for (DWORD i = 0; i < count; ++i)
	{
		// Both "Idle" and "_Total" report a process ID of zero
		if (!PdhUtil::IsValidStatus(items[i].RawValue.CStatus) || items[i].RawValue.FirstValue == 0) continue;

		// The name is stored as PerfMon reports it, since rolling up similar names happens after
		// the translation
		pids.insert_or_assign((DWORD)items[i].RawValue.FirstValue, items[i].szName);
	}

	CriticalSectionLock dataLock(g_PidLock);
	g_Pids = std::move(pids);
}

void CALLBACK PidTimerProc(void* context, BOOLEAN timerOrWaitFired)
{
	UpdatePids();
}

void StopPidCollector()
{
	g_PidReferences = 0;

	if (g_PidTimer)
	{
		// Waits for a collection that is already running to finish
		DeleteTimerQueueTimer(nullptr, g_PidTimer, INVALID_HANDLE_VALUE);
		g_PidTimer = nullptr;
	}

	if (g_PidQuery)
	{
		PdhCloseQuery(g_PidQuery);   // Also closes every counter that was added to the query
		g_PidQuery = nullptr;
		g_PidCounter = nullptr;
	}

	CriticalSectionLock lock(g_PidLock);
	g_Pids.clear();
}

void AddPidReference()
{
	if (++g_PidReferences > 1) return;

	if (PdhOpenQuery(nullptr, 0, &g_PidQuery) != ERROR_SUCCESS)
	{
		g_PidQuery = nullptr;
		return;
	}

	if (!PdhUtil::AddCounter(g_PidQuery, L"Process", L"ID Process", L"*", &g_PidCounter))
	{
		LogDebugF(L"UsageMonitor: Could not read the process IDs, so PIDToName will be ignored");
		PdhCloseQuery(g_PidQuery);
		g_PidQuery = nullptr;
		return;
	}

	CreateTimerQueueTimer(&g_PidTimer, nullptr, PidTimerProc, nullptr, 0, c_UpdateRate, WT_EXECUTEDEFAULT);
}

void ReleasePidReference()
{
	if (g_PidReferences == 0 || --g_PidReferences > 0) return;

	StopPidCollector();
}

// Replaces a "pid_1234_..." instance name with the name of the process that owns it. Returns false
// when the instance should be dropped because the process is not one we know about.
bool TranslatePid(std::wstring& name)
{
	const size_t position = name.find(L"pid_");
	if (position == std::wstring::npos) return true;

	const size_t start = position + 4;
	const size_t end = name.find(L'_', start);
	if (end == std::wstring::npos || end == start) return true;

	const std::wstring number = name.substr(start, end - start);
	WCHAR* stop = nullptr;
	const DWORD pid = wcstoul(number.c_str(), &stop, 10);
	if (stop == nullptr || *stop != L'\0') return true;

	CriticalSectionLock lock(g_PidLock);

	// Leave the name alone until there is something to translate it with
	if (g_Pids.empty()) return true;

	const auto found = g_Pids.find(pid);
	if (found == g_Pids.end()) return false;

	name = found->second;
	return true;
}

// Keeps one performance counter category up to date on a background thread and hands out the
// collected instances to the measures using it. As much as possible is shared between those
// measures, since reading a category is by far the most expensive part of the whole exercise.
class Collector
{
public:
	explicit Collector(const std::wstring& category);
	~Collector();

	Collector(const Collector&) = delete;
	Collector& operator=(const Collector&) = delete;

	bool IsValid() const { return m_Query != nullptr; }

	bool AddMeasure(const CounterOptions& options);

	// Returns true when the collector no longer has anything to collect, in which case the caller
	// is expected to destroy it
	bool RemoveMeasure(const CounterOptions& options);

	CounterInstance GetInstance(const CounterOptions& options, int index);
	CounterInstance GetInstance(const CounterOptions& options, const std::wstring& name);
	CounterInstance GetSum(const CounterOptions& options);
	CounterInstance GetAverage(const CounterOptions& options);

private:
	// Everything that changes the name an instance ends up under, and therefore which instances get
	// merged together, has to pick a different set of instances
	static int NameKey(const CounterOptions& options) { return (options.rollup ? 1 : 0) | (options.pidToName ? 2 : 0); }

	struct CounterInfo
	{
		// Every instance of the counter by name, including the ones that the block list leaves out
		ankerl::unordered_dense::map<std::wstring, CounterInstance> byName[4];
		bool byNameValid[4] = { false, false, false, false };

		// The instances that the block list allows, sorted by value, keyed by block key
		ankerl::unordered_dense::map<std::wstring, std::vector<CounterInstance>> byUsage;
		ankerl::unordered_dense::map<std::wstring, double> sum;
		ankerl::unordered_dense::map<std::wstring, double> average;
	};

	struct CounterQuery
	{
		PDH_HCOUNTER handle = nullptr;
		ankerl::unordered_dense::map<size_t, CounterOptions> options;   // Keyed by measure
	};

	static void CALLBACK TimerProc(void* parameter, BOOLEAN timerOrWaitFired);

	void Collect();
	bool BuildCounterInfo(const CounterQuery& counter, CounterInfo& info);

	static void BuildByName(const CounterOptions& options, const PDH_RAW_COUNTER_ITEM* items, DWORD count,
		const std::vector<double>& values,
		ankerl::unordered_dense::map<std::wstring, CounterInstance>& byName);
	static void BuildByUsage(const CounterOptions& options,
		const ankerl::unordered_dense::map<std::wstring, CounterInstance>& byName, CounterInfo& info);

	// Must be called with the data lock held
	static std::optional<double> GetTotal(const CounterInfo& info, const CounterOptions& options);

	std::wstring m_Category;
	PDH_HQUERY m_Query;
	HANDLE m_Timer;
	bool m_CollectFailed;

	// Guards the query and the registered measures, and keeps collections from overlapping
	CriticalSection m_UpdateLock;
	ankerl::unordered_dense::map<std::wstring, CounterQuery> m_Counters;

	// Only ever touched by a collection, and kept between them so that reading a category does not
	// have to go back to the heap for buffers that settle at a stable size
	std::vector<BYTE> m_RawBuffer;
	std::vector<BYTE> m_FormattedBuffer;
	std::vector<double> m_Values;

	// Guards the collected data, so that measures never have to wait for a collection
	CriticalSection m_DataLock;
	ankerl::unordered_dense::map<std::wstring, CounterInfo> m_Data;
};

Collector::Collector(const std::wstring& category) :
	m_Category(category),
	m_Query(nullptr),
	m_Timer(nullptr),
	m_CollectFailed(false)
{
	if (PdhOpenQuery(nullptr, 0, &m_Query) != ERROR_SUCCESS)
	{
		m_Query = nullptr;
	}
}

Collector::~Collector()
{
	if (m_Timer)
	{
		// Waits for a collection that is already running to finish, so this must never be reached
		// while the update lock is held
		DeleteTimerQueueTimer(nullptr, m_Timer, INVALID_HANDLE_VALUE);
		m_Timer = nullptr;
	}

	if (m_Query)
	{
		PdhCloseQuery(m_Query);   // Also closes every counter that was added to the query
		m_Query = nullptr;
	}
}

bool Collector::AddMeasure(const CounterOptions& options)
{
	if (!m_Query) return false;

	CriticalSectionLock lock(m_UpdateLock);

	auto counter = m_Counters.find(options.counter);
	if (counter == m_Counters.end())
	{
		CounterQuery added;
		if (!PdhUtil::AddCounter(m_Query, m_Category, options.counter, L"*", &added.handle)) return false;

		counter = m_Counters.emplace(options.counter, std::move(added)).first;
	}

	counter->second.options.insert_or_assign(options.id, options);

	if (!m_Timer)
	{
		CreateTimerQueueTimer(&m_Timer, nullptr, TimerProc, this, 0, c_UpdateRate, WT_EXECUTEDEFAULT);
	}

	return true;
}

bool Collector::RemoveMeasure(const CounterOptions& options)
{
	CriticalSectionLock lock(m_UpdateLock);

	const auto counter = m_Counters.find(options.counter);
	if (counter != m_Counters.end())
	{
		counter->second.options.erase(options.id);

		if (counter->second.options.empty())
		{
			if (counter->second.handle)
			{
				PdhRemoveCounter(counter->second.handle);
			}
			m_Counters.erase(counter);

			CriticalSectionLock dataLock(m_DataLock);
			m_Data.erase(options.counter);
		}
	}

	return m_Counters.empty();
}

void CALLBACK Collector::TimerProc(void* context, BOOLEAN timerOrWaitFired)
{
	((Collector*)context)->Collect();
}

void Collector::Collect()
{
	// Skip this round entirely when a collection is still running or a measure is being registered,
	// rather than letting collections stack up
	CriticalSectionTryLock lock(m_UpdateLock);
	if (!lock) return;

	if (m_Counters.empty()) return;

	const PDH_STATUS status = PdhCollectQueryData(m_Query);
	if (status != ERROR_SUCCESS)
	{
		if (!m_CollectFailed)
		{
			m_CollectFailed = true;
			LogDebugF(L"UsageMonitor: Could not read the category \"%s\" (0x%08x)", m_Category.c_str(), status);
		}
		return;
	}
	m_CollectFailed = false;

	// Everything is built up on the side and only then swapped in, so that measures reading the
	// previous collection are never held up for longer than the swap itself
	ankerl::unordered_dense::map<std::wstring, CounterInfo> data;
	data.reserve(m_Counters.size());

	for (const auto& counter : m_Counters)
	{
		CounterInfo info;
		if (BuildCounterInfo(counter.second, info))
		{
			data.emplace(counter.first, std::move(info));
		}
	}

	CriticalSectionLock dataLock(m_DataLock);
	m_Data = std::move(data);
}

bool Collector::BuildCounterInfo(const CounterQuery& counter, CounterInfo& info)
{
	DWORD rawCount = 0;
	if (!PdhUtil::GetRawArray(counter.handle, m_RawBuffer, rawCount)) return false;

	const auto rawItems = (const PDH_RAW_COUNTER_ITEM*)m_RawBuffer.data();

	// Counters that measure a rate have nothing to report until they have been collected twice, in
	// which case every instance is simply left at zero
	DWORD formattedCount = 0;
	if (PdhUtil::GetFormattedArray(counter.handle, m_FormattedBuffer, formattedCount))
	{
		PdhUtil::MatchValues(rawItems, rawCount, (const PDH_FMT_COUNTERVALUE_ITEM*)m_FormattedBuffer.data(),
			formattedCount, m_Values);
	}
	else
	{
		m_Values.assign(rawCount, 0.0);
	}

	for (const auto& entry : counter.options)
	{
		const CounterOptions& options = entry.second;
		const int nameKey = NameKey(options);

		if (!info.byNameValid[nameKey])
		{
			BuildByName(options, rawItems, rawCount, m_Values, info.byName[nameKey]);
			info.byNameValid[nameKey] = true;
		}

		if (info.byUsage.find(options.blockKey) == info.byUsage.end())
		{
			BuildByUsage(options, info.byName[nameKey], info);
		}
	}

	return true;
}

void Collector::BuildByName(const CounterOptions& options, const PDH_RAW_COUNTER_ITEM* items, DWORD count,
	const std::vector<double>& values,
	ankerl::unordered_dense::map<std::wstring, CounterInstance>& byName)
{
	byName.reserve(count);

	for (DWORD i = 0; i < count; ++i)
	{
		std::wstring name = items[i].szName;

		if (options.pidToName && !TranslatePid(name)) continue;

		// Roll up similar names by taking everything before the last number in the name
		if (options.rollup)
		{
			const size_t position = name.rfind(L'#');
			if (position != std::wstring::npos && position > 0)
			{
				name.resize(position);
			}
		}

		const double value = values[i];
		const LONGLONG rawValue = PdhUtil::IsValidStatus(items[i].RawValue.CStatus) ? items[i].RawValue.FirstValue : 0;

		// Instances that ended up under the same name are combined into one
		const auto existing = byName.find(name);
		if (existing == byName.end())
		{
			CounterInstance instance;
			instance.name = name;
			instance.value = value;
			instance.rawValue = rawValue;
			byName.emplace(std::move(name), std::move(instance));
		}
		else
		{
			existing->second.value += value;
			existing->second.rawValue += rawValue;
		}
	}
}

void Collector::BuildByUsage(const CounterOptions& options,
	const ankerl::unordered_dense::map<std::wstring, CounterInstance>& byName, CounterInfo& info)
{
	std::vector<CounterInstance> byUsage;
	byUsage.reserve(byName.size());
	double sum = 0.0;

	for (const auto& entry : byName)
	{
		const bool listed = std::find(options.blockList.begin(), options.blockList.end(), entry.first) !=
			options.blockList.end();

		const bool allowed =
			options.blockType == BlockType::None ||
			(options.blockType == BlockType::Blacklist && !listed) ||
			(options.blockType == BlockType::Whitelist && listed);

		if (allowed)
		{
			byUsage.push_back(entry.second);
			sum += entry.second.value;
		}
	}

	std::sort(byUsage.begin(), byUsage.end(), [](const CounterInstance& lhs, const CounterInstance& rhs)
	{
		// Raw values break the ties that the formatted values leave behind
		if (lhs.value != rhs.value) return lhs.value > rhs.value;
		return lhs.rawValue > rhs.rawValue;
	});

	const double average = sum > 0.0 ? sum / (double)byUsage.size() : 0.0;

	info.byUsage.insert_or_assign(options.blockKey, std::move(byUsage));
	info.sum.insert_or_assign(options.blockKey, sum);
	info.average.insert_or_assign(options.blockKey, average);
}

std::optional<double> Collector::GetTotal(const CounterInfo& info, const CounterOptions& options)
{
	const int nameKey = NameKey(options);
	if (!info.byNameValid[nameKey]) return 0.0;

	const auto& byName = info.byName[nameKey];
	const auto found = byName.find(L"_Total");
	if (found == byName.end()) return std::nullopt;

	return found->second.value;
}

CounterInstance Collector::GetInstance(const CounterOptions& options, int index)
{
	CriticalSectionLock lock(m_DataLock);

	const auto counter = m_Data.find(options.counter);
	if (counter != m_Data.end())
	{
		const auto byUsage = counter->second.byUsage.find(options.blockKey);
		if (byUsage != counter->second.byUsage.end() && index > 0 && (size_t)index <= byUsage->second.size())
		{
			// Instances are not zero indexed in skins
			CounterInstance instance = byUsage->second[index - 1];
			instance.total = GetTotal(counter->second, options);
			return instance;
		}
	}

	return CounterInstance();
}

CounterInstance Collector::GetInstance(const CounterOptions& options, const std::wstring& name)
{
	CriticalSectionLock lock(m_DataLock);

	const auto counter = m_Data.find(options.counter);
	if (counter != m_Data.end())
	{
		const int nameKey = NameKey(options);
		if (counter->second.byNameValid[nameKey])
		{
			const auto& byName = counter->second.byName[nameKey];
			const auto found = byName.find(name);
			if (found != byName.end())
			{
				CounterInstance instance = found->second;

				// Looking up the "_Total" instance itself must not go looking for it again
				instance.total = (name == L"_Total") ? std::optional<double>(instance.value) : GetTotal(counter->second, options);
				return instance;
			}
		}
	}

	CounterInstance instance;
	instance.name = name;
	return instance;
}

CounterInstance Collector::GetSum(const CounterOptions& options)
{
	CriticalSectionLock lock(m_DataLock);

	CounterInstance instance;
	instance.name = L"Total";

	const auto counter = m_Data.find(options.counter);
	if (counter != m_Data.end())
	{
		const auto sum = counter->second.sum.find(options.blockKey);
		if (sum != counter->second.sum.end())
		{
			instance.value = sum->second;
			instance.total = GetTotal(counter->second, options);
		}
	}

	return instance;
}

CounterInstance Collector::GetAverage(const CounterOptions& options)
{
	CriticalSectionLock lock(m_DataLock);

	CounterInstance instance;
	instance.name = L"Average";

	const auto counter = m_Data.find(options.counter);
	if (counter != m_Data.end())
	{
		const auto average = counter->second.average.find(options.blockKey);
		if (average != counter->second.average.end())
		{
			instance.value = average->second;
			instance.total = GetTotal(counter->second, options);
		}
	}

	return instance;
}

// Every category being monitored, which is only ever touched on the main thread
ankerl::unordered_dense::map<std::wstring, std::unique_ptr<Collector>>& GetCollectors()
{
	static ankerl::unordered_dense::map<std::wstring, std::unique_ptr<Collector>> s_Collectors;
	return s_Collectors;
}

Collector* FindCollector(const std::wstring& category)
{
	auto& collectors = GetCollectors();
	const auto found = collectors.find(category);
	return found == collectors.end() ? nullptr : found->second.get();
}

// Splits a "foo|bar|baz" list, trimming the whitespace around each entry
std::vector<std::wstring> ParseBlockList(const std::wstring& value)
{
	std::vector<std::wstring> list;

	size_t start = 0;
	while (true)
	{
		const size_t end = value.find(L'|', start);
		std::wstring item = value.substr(start, (end == std::wstring::npos) ? std::wstring::npos : end - start);

		const size_t first = item.find_first_not_of(L" \t\r\n");
		if (first == std::wstring::npos)
		{
			item.clear();
		}
		else
		{
			item = item.substr(first, item.find_last_not_of(L" \t\r\n") - first + 1);
		}
		list.push_back(std::move(item));

		if (end == std::wstring::npos) break;
		start = end + 1;
	}

	return list;
}

// Identifies option sets whose filtered instance lists are interchangeable. Everything that changes
// which instances end up in the list, or under which name, has to be part of the key.
std::wstring BuildBlockKey(const CounterOptions& options)
{
	std::wstring key;
	key += options.rollup ? L'1' : L'0';
	key += options.pidToName ? L'1' : L'0';
	key += options.blockType == BlockType::None ? L'N' : (options.blockType == BlockType::Blacklist ? L'B' : L'W');

	for (const auto& item : options.blockList)
	{
		key += L'|';
		key += item;
	}

	return key;
}

enum class Alias
{
	CPU,
	Ram,
	RamShared,
	IO,
	IORead,
	IOWrite,
	GPU,
	VRam,
	VRamShared,
	NetDown,
	NetUp,
	Custom
};

}  // namespace

MeasureUsageMonitor::MeasureUsageMonitor(Skin* skin, const WCHAR* name) : Measure(skin, name),
	m_Options(),
	m_CurrentInstance(),
	m_InstanceName(),
	m_Index(0),
	m_RawValue(false),
	m_Percent(false),
	m_Registered(false),
	m_RegisterFailed(false),
	m_LoggedNoTotal(false),
	m_PidReferenced(false)
{
}

MeasureUsageMonitor::~MeasureUsageMonitor()
{
	UnregisterMeasure();
	UpdatePidReference(false);
}

void MeasureUsageMonitor::FinalizeStatic()
{
	GetCollectors().clear();
	StopPidCollector();
}

void MeasureUsageMonitor::ReadOptions(ConfigParser& parser, std::wstring_view section)
{
	Measure::ReadOptions(parser, section);

	static constexpr ConfigParser::EnumOption<Alias> s_Aliases[] =
	{
		{ L"CPU", Alias::CPU },
		{ L"RAM", Alias::Ram },
		{ L"RAMSHARED", Alias::RamShared },
		{ L"IO", Alias::IO },
		{ L"IOREAD", Alias::IORead },
		{ L"IOWRITE", Alias::IOWrite },
		{ L"GPU", Alias::GPU },
		{ L"VRAM", Alias::VRam },
		{ L"VRAMSHARED", Alias::VRamShared },
		{ L"NETDOWN", Alias::NetDown },
		{ L"NETUP", Alias::NetUp },
		{ L"CUSTOM", Alias::Custom }
	};
	const Alias alias = parser.ReadEnum(section, L"Alias", Alias::Custom, s_Aliases);

	CounterOptions options;
	options.id = (size_t)this;

	double maxValue = 0.0;
	bool percent = false;
	bool pidToName = false;

	// NETDOWN and NETUP have never been implemented
	switch (alias)
	{
	case Alias::CPU:
		options.category = L"Process";
		options.counter = L"% Processor Time";
		percent = true;
		break;

	case Alias::Ram:
		options.category = L"Process";
		options.counter = L"Working Set - Private";
		break;

	case Alias::RamShared:
		options.category = L"Process";
		options.counter = L"Working Set";
		break;

	case Alias::IO:
		options.category = L"Process";
		options.counter = L"IO Data Bytes/sec";
		break;

	case Alias::IORead:
		options.category = L"Process";
		options.counter = L"IO Read Bytes/sec";
		break;

	case Alias::IOWrite:
		options.category = L"Process";
		options.counter = L"IO Write Bytes/sec";
		break;

	case Alias::GPU:
		options.category = L"GPU Engine";
		options.counter = L"Utilization Percentage";
		pidToName = true;

		// The GPU counters have no "_Total" instance to scale against, so the range is set here
		// instead of through Percent
		maxValue = 100.0;
		break;

	case Alias::VRam:
		options.category = L"GPU Process Memory";
		options.counter = L"Dedicated Usage";
		pidToName = true;
		break;

	case Alias::VRamShared:
		options.category = L"GPU Process Memory";
		options.counter = L"Shared Usage";
		pidToName = true;
		break;

	default:
		break;
	}

	// An alias that has already filled these in is only overridden by an option that says something
	std::wstring category;
	parser.ReadString(category, section, L"Category", L"");
	if (!category.empty())
	{
		options.category = std::move(category);
	}

	std::wstring counter;
	parser.ReadString(counter, section, L"Counter", L"");
	if (!counter.empty())
	{
		options.counter = std::move(counter);
	}

	m_RawValue = parser.ReadBool(section, L"RawValue", false);
	options.rollup = parser.ReadBool(section, L"Rollup", true);

	const std::wstring& whitelist = parser.ReadString(section, L"Whitelist", L"");
	if (!whitelist.empty())
	{
		options.blockType = BlockType::Whitelist;
		options.blockList = ParseBlockList(whitelist);
	}
	else
	{
		// NOTE: This reuses the buffer that |whitelist| points at
		const std::wstring& blacklist = parser.ReadString(section, L"Blacklist", L"_Total|Idle");
		if (!blacklist.empty())
		{
			options.blockType = BlockType::Blacklist;
			options.blockList = ParseBlockList(blacklist);
		}
		else
		{
			options.blockType = BlockType::None;
			options.blockList.clear();
		}
	}

	m_Percent = parser.ReadBool(section, L"Percent", percent);
	if (m_Percent)
	{
		maxValue = 100.0;
	}

	options.pidToName = parser.ReadBool(section, L"PIDToName", pidToName);
	options.blockKey = BuildBlockKey(options);

	// Setting the options of a measure that uses dynamic variables can move it to another counter,
	// in which case the old one has to let go of it first
	if (m_Options.category != options.category || m_Options.counter != options.counter)
	{
		UnregisterMeasure();
		m_RegisterFailed = false;
		m_LoggedNoTotal = false;
	}

	m_Options = std::move(options);

	if (!m_Options.category.empty() && !m_Options.counter.empty())
	{
		RegisterMeasure();
	}

	UpdatePidReference(m_Registered && m_Options.pidToName);

	// One of these is normally left alone
	m_Index = parser.ReadInt(section, L"Index", 0);
	parser.ReadString(m_InstanceName, section, L"Name", L"");

	if (!parser.IsValueDefined(section, L"MaxValue"))
	{
		if (maxValue == 0.0)
		{
			// These counters have no natural maximum, so the measure has to find its own range.
			// Starting over is only right when it was not already doing so, since a measure that
			// uses dynamic variables reads its options on every update.
			if (!m_LogMaxValue)
			{
				m_MaxValue = 1.0;
				m_LogMaxValue = true;
				m_MedianValues.clear();
			}
		}
		else
		{
			m_MaxValue = maxValue;
			m_LogMaxValue = false;
		}
	}
}

void MeasureUsageMonitor::RegisterMeasure()
{
	auto& collectors = GetCollectors();

	auto found = collectors.find(m_Options.category);
	if (found == collectors.end())
	{
		auto collector = std::make_unique<Collector>(m_Options.category);
		if (!collector->IsValid())
		{
			if (!m_RegisterFailed)
			{
				m_RegisterFailed = true;
				LogErrorF(this, L"UsageMonitor: Could not read the category \"%s\"", m_Options.category.c_str());
			}
			return;
		}

		found = collectors.emplace(m_Options.category, std::move(collector)).first;
	}

	if (!found->second->AddMeasure(m_Options))
	{
		if (!m_RegisterFailed)
		{
			m_RegisterFailed = true;
			LogErrorF(this, L"UsageMonitor: Could not find a counter named \"%s\" in the category \"%s\"",
				m_Options.counter.c_str(), m_Options.category.c_str());
		}

		// A collector that was created just for this measure has nothing left to do
		if (found->second->RemoveMeasure(m_Options))
		{
			collectors.erase(found);
		}
		return;
	}

	m_Registered = true;
}

void MeasureUsageMonitor::UnregisterMeasure()
{
	if (!m_Registered) return;
	m_Registered = false;

	auto& collectors = GetCollectors();
	const auto found = collectors.find(m_Options.category);
	if (found == collectors.end()) return;

	// The collector waits for a running collection while it is being destroyed, so this must happen
	// with no lock of its own held
	if (found->second->RemoveMeasure(m_Options))
	{
		collectors.erase(found);
	}
}

void MeasureUsageMonitor::UpdatePidReference(bool needed)
{
	if (needed == m_PidReferenced) return;

	m_PidReferenced = needed;
	needed ? AddPidReference() : ReleasePidReference();
}

void MeasureUsageMonitor::UpdateValue()
{
	m_CurrentInstance = CounterInstance();

	if (m_Options.category.empty() || m_Options.counter.empty())
	{
		m_Value = 0.0;
		return;
	}

	Collector* collector = FindCollector(m_Options.category);
	if (collector == nullptr)
	{
		m_CurrentInstance.name = m_InstanceName;
	}
	else if (!m_InstanceName.empty())
	{
		m_CurrentInstance = collector->GetInstance(m_Options, m_InstanceName);
	}
	else if (m_Index == -1)
	{
		m_CurrentInstance = collector->GetAverage(m_Options);
	}
	else if (m_Index == 0)
	{
		m_CurrentInstance = collector->GetSum(m_Options);
	}
	else if (m_Index > 0)
	{
		m_CurrentInstance = collector->GetInstance(m_Options, m_Index);
	}

	double value = m_CurrentInstance.value;

	if (m_RawValue)
	{
		// Raw values override Percent
		value = (double)m_CurrentInstance.rawValue;
	}
	else if (m_Percent)
	{
		if (m_CurrentInstance.total.has_value())
		{
			const double total = m_CurrentInstance.total.value();
			if (total != 0.0)
			{
				value = value / total * 100.0;
			}

			// This measure is more accurate than the "_Total" instance it is scaled against, so the
			// result can creep just above 100
			if (value > 100.0)
			{
				value = 100.0;
			}
		}
		else if (!m_LoggedNoTotal)
		{
			m_LoggedNoTotal = true;
			LogNoticeF(this, L"UsageMonitor: Percent=1 was set on this measure with counter %s but that counter does not have a _Total instance",
				m_Options.counter.c_str());
		}
	}

	m_Value = value;
}

const WCHAR* MeasureUsageMonitor::GetStringValue()
{
	if (m_Options.category.empty() || m_Options.counter.empty()) return nullptr;

	// Without a name to look up, the instance was picked by usage, so there is no instance worth
	// naming when nothing is being used
	if (m_InstanceName.empty() && m_CurrentInstance.value == 0.0) return CheckSubstitute(L"");

	return CheckSubstitute(m_CurrentInstance.name.c_str());
}
