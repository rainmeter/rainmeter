// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

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
