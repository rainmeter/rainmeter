// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "Measure.h"

class MeasureAdvancedCPU : public Measure
{
public:
	MeasureAdvancedCPU(Skin* skin, const WCHAR* name);
	virtual ~MeasureAdvancedCPU();

	MeasureAdvancedCPU(const MeasureAdvancedCPU& other) = delete;
	MeasureAdvancedCPU& operator=(MeasureAdvancedCPU other) = delete;

	UINT GetTypeID() override { return TypeID<MeasureAdvancedCPU>(); }
	const WCHAR* GetStringValue() override;

protected:
	void ReadOptions(ConfigParser& parser, std::wstring_view section) override;
	void UpdateValue() override;

private:
	void ReadProcessList(const std::wstring& value, std::vector<std::wstring>& processList);
	bool CheckProcess(const WCHAR* name);

	std::vector<std::wstring> m_Includes;
	std::vector<std::wstring> m_Excludes;
	std::wstring m_IncludesCache;
	std::wstring m_ExcludesCache;
	int m_TopProcess;
	std::wstring m_TopProcessName;
};
