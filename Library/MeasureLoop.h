// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "Measure.h"

class MeasureLoop : public Measure
{
public:
	MeasureLoop(Skin* skin, const WCHAR* name);
	virtual ~MeasureLoop();

	MeasureLoop(const MeasureLoop& other) = delete;
	MeasureLoop& operator=(MeasureLoop other) = delete;

	virtual void Command(const std::wstring& command);

	virtual UINT GetTypeID() { return TypeID<MeasureLoop>(); }

protected:
	void ReadOptions(ConfigParser& parser, std::wstring_view section) override;
	virtual void UpdateValue();

private:
	void Reset();

	int m_StartValue;
	int m_EndValue;
	int m_Increment;

	int m_LoopCount;
	int m_LoopCounter;

	bool m_SkipFirst;
	bool m_HasOverRun;
};
