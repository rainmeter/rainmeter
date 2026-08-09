// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "Measure.h"

class MeasureString : public Measure
{
public:
	MeasureString(Skin* skin, const WCHAR* name);
	virtual ~MeasureString();

	MeasureString(const MeasureString& other) = delete;
	MeasureString& operator=(MeasureString other) = delete;

	virtual const WCHAR* GetStringValue();

	virtual UINT GetTypeID() { return TypeID<MeasureString>(); }

protected:
	virtual void ReadOptions(ConfigParser& parser, const WCHAR* section);
	virtual void UpdateValue();

private:
	Locale::NumberFormat m_NumberFormat;
	std::wstring m_String;
	std::wstring m_StringValue;
};
