// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "Measure.h"

class MeasureDiskSpace : public Measure
{
public:
	MeasureDiskSpace(Skin* skin, const WCHAR* name);
	virtual ~MeasureDiskSpace();

	MeasureDiskSpace(const MeasureDiskSpace& other) = delete;
	MeasureDiskSpace& operator=(MeasureDiskSpace other) = delete;

	virtual UINT GetTypeID() { return TypeID<MeasureDiskSpace>(); }

	virtual std::optional<std::wstring_view> GetStringValue();

protected:
	void ReadOptions(ConfigParser::OptionReader& reader) override;
	virtual void UpdateValue();

private:
	std::wstring m_Drive;
	std::wstring m_StringValue;
	bool m_Type;
	bool m_Total;
	bool m_Label;
	bool m_IgnoreRemovable;
	bool m_DiskQuota;

	ULONGLONG m_OldTotalBytes;
};
