// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "MeterStringBase.h"

class MeterString : public MeterStringBase
{
public:
	MeterString(Skin* skin, const WCHAR* name);
	virtual ~MeterString();

	MeterString(const MeterString& other) = delete;
	MeterString& operator=(MeterString other) = delete;

	virtual UINT GetTypeID() { return TypeID<MeterString>(); }

	virtual bool Update();
	virtual bool Draw(Gfx::Canvas& canvas);

protected:
	void ReadOptions(ConfigParser& parser, std::wstring_view section) override;
	void BindMeasures(ConfigParser& parser, std::wstring_view section) override;

private:
	std::wstring m_Prefix;
	std::wstring m_Postfix;
	FLOAT m_Angle;
	AUTOSCALE m_AutoScale;
	double m_Scale;
	bool m_NoDecimals;
	bool m_Percentual;
	int m_NumOfDecimals;
	bool m_TrailingSpaces;

	// The text is measured only when it changes, so what it was the update before is kept to
	// compare against.
	std::wstring m_PreviousString;
};
