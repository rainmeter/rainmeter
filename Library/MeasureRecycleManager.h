// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "Measure.h"

class MeasureRecycleManager : public Measure
{
public:
	MeasureRecycleManager(Skin* skin, const WCHAR* name);
	virtual ~MeasureRecycleManager();

	MeasureRecycleManager(const MeasureRecycleManager& other) = delete;
	MeasureRecycleManager& operator=(MeasureRecycleManager other) = delete;

	UINT GetTypeID() override { return TypeID<MeasureRecycleManager>(); }

	void Command(const std::wstring& command) override;

protected:
	void ReadOptions(ConfigParser& parser, const WCHAR* section) override;
	void UpdateValue() override;

private:
	enum class Type;
	Type m_Type;
};
