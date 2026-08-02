// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "Measure.h"

struct FolderInfoParentMeasure;

class MeasureFolderInfo : public Measure
{
public:
	MeasureFolderInfo(Skin* skin, const WCHAR* name);
	virtual ~MeasureFolderInfo();

	MeasureFolderInfo(const MeasureFolderInfo& other) = delete;
	MeasureFolderInfo& operator=(MeasureFolderInfo other) = delete;

	UINT GetTypeID() override { return TypeID<MeasureFolderInfo>(); }

protected:
	void ReadOptions(ConfigParser& parser, const WCHAR* section) override;
	void UpdateValue() override;

private:
	enum class Type;

	FolderInfoParentMeasure* m_Parent;
	Type m_Type;
};
