/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_LIBRARY_MEASUREQUOTE_H_
#define RM_LIBRARY_MEASUREQUOTE_H_

#include "Measure.h"

class MeasureQuote : public Measure
{
public:
	MeasureQuote(Skin* skin, const WCHAR* name);
	virtual ~MeasureQuote();

	MeasureQuote(const MeasureQuote& other) = delete;
	MeasureQuote& operator=(MeasureQuote other) = delete;

	virtual UINT GetTypeID() { return TypeID<MeasureQuote>(); }

	virtual const WCHAR* GetStringValue();

protected:
	virtual void ReadOptions(ConfigParser& parser, const WCHAR* section);
	virtual void UpdateValue();

private:
	static void ScanFolder(std::vector<std::wstring>& files, std::vector<std::wstring>& filters, bool subfolders, const std::wstring& path);

	std::wstring m_PathName;
	std::wstring m_Separator;
	std::vector<std::wstring> m_Files;
	std::wstring m_StringValue;
};

#endif
