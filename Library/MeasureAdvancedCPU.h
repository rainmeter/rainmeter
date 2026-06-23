/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_LIBRARY_MEASUREADVANCEDCPU_H_
#define RM_LIBRARY_MEASUREADVANCEDCPU_H_

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
	void ReadOptions(ConfigParser& parser, const WCHAR* section) override;
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

#endif
