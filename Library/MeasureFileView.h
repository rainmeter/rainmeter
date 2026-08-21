// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "Measure.h"

struct FileViewParentData;

class MeasureFileView : public Measure
{
public:
	MeasureFileView(Skin* skin, const WCHAR* name);
	virtual ~MeasureFileView();

	MeasureFileView(const MeasureFileView& other) = delete;
	MeasureFileView& operator=(MeasureFileView other) = delete;

	UINT GetTypeID() override { return TypeID<MeasureFileView>(); }
	const WCHAR* GetStringValue() override;
	void Command(const std::wstring& command) override;

	class UpdateTask;

protected:
	void ReadOptions(ConfigParser& parser, std::wstring_view section) override;
	void UpdateValue() override;

private:
	friend struct FileViewParentData;

	enum MeasureType : BYTE;
	enum DateType : BYTE;

	void SetParent(FileViewParentData* parent);

	MeasureType m_Type;
	DateType m_DateType;
	int m_IconSize;
	int m_Index;
	bool m_IgnoreCount;

	std::wstring m_StrValue;
	FileViewParentData* m_Parent;
};
