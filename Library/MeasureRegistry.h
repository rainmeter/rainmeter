// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "Measure.h"

class MeasureRegistry : public Measure
{
public:
	MeasureRegistry(Skin* skin, const WCHAR* name);
	virtual ~MeasureRegistry();

	MeasureRegistry(const MeasureRegistry& other) = delete;
	MeasureRegistry& operator=(MeasureRegistry other) = delete;

	virtual UINT GetTypeID() { return TypeID<MeasureRegistry>(); }

	virtual const WCHAR* GetStringValue();

protected:
	virtual void ReadOptions(ConfigParser& parser, const WCHAR* section);
	virtual void UpdateValue();

private:
	enum class OutputType : BYTE
	{
		Value = 0,
		SubKeyList,
		ValueList
	};

	void Dispose();

	OutputType m_OutputType;
	std::wstring m_OutputDelimiter;
	std::wstring m_RegKeyName;
	std::wstring m_RegValueName;
	std::wstring m_StringValue;
    HKEY m_RegKey;
    HKEY m_HKey;
};
