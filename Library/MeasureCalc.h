// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "Measure.h"
#include "../Common/MathParser.h"

class MeasureCalc : public Measure
{
public:
	MeasureCalc(Skin* skin, const WCHAR* name);
	virtual ~MeasureCalc();

	MeasureCalc(const MeasureCalc& other) = delete;
	MeasureCalc& operator=(MeasureCalc other) = delete;

	virtual UINT GetTypeID() { return TypeID<MeasureCalc>(); }

protected:
	virtual void ReadOptions(ConfigParser& parser, const WCHAR* section);
	virtual void UpdateValue();

private:
	static bool GetMeasureValue(const WCHAR* str, int len, double* value, void* context);

	void FormulaReplace();
	int GetRandom();

	std::wstring m_Formula;
	bool m_ParseError;
	MathParser m_MathParser;

	int m_LowBound;
	int m_HighBound;

	bool m_UpdateRandom;
	bool m_UniqueRandom;

	std::vector<int> m_UniqueNumbers;
	void UpdateUniqueNumberList();
};
