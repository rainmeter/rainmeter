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
	virtual void ReadOptions(ConfigParser& parser, const WCHAR* section);
	virtual void BindMeasures(ConfigParser& parser, const WCHAR* section);

private:
	enum TEXTCASE
	{
		TEXTCASE_NONE,
		TEXTCASE_UPPER,
		TEXTCASE_LOWER,
		TEXTCASE_PROPER
	};

	std::wstring m_Prefix;
	std::wstring m_Postfix;
	AUTOSCALE m_AutoScale;
	TEXTCASE m_Case;
	double m_Scale;
	bool m_NoDecimals;
	bool m_Percentual;
	int m_NumOfDecimals;
	bool m_TrailingSpaces;
};
