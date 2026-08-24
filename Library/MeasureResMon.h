// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "Measure.h"

class MeasureResMon : public Measure
{
public:
	MeasureResMon(Skin* skin, const WCHAR* name);
	virtual ~MeasureResMon();

	MeasureResMon(const MeasureResMon& other) = delete;
	MeasureResMon& operator=(MeasureResMon other) = delete;

	UINT GetTypeID() override { return TypeID<MeasureResMon>(); }

protected:
	void ReadOptions(ConfigParser& parser, std::wstring_view section) override;
	void UpdateValue() override;

private:
	enum class Type
	{
		GDI,
		USER,
		HANDLE,
		WINDOW
	};

	Type m_Type;
	std::wstring m_ProcessName;
};
