// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "Meter.h"
#include "Measure.h"

class MeterString : public Meter
{
public:
	MeterString(Skin* skin, const WCHAR* name);
	virtual ~MeterString();

	MeterString(const MeterString& other) = delete;
	MeterString& operator=(MeterString other) = delete;

	virtual UINT GetTypeID() { return TypeID<MeterString>(); }

	virtual int GetX(bool abs = false);
	virtual int GetY(bool abs = false);

	virtual void Initialize();
	virtual void InvalidateDeviceResources() override;
	virtual bool Update();
	virtual bool Draw(Gfx::Canvas& canvas);

	void SetText(const WCHAR* text) { m_Text = text; }

	static void InitializeStatic();
	static void FinalizeStatic();

protected:
	virtual void ReadOptions(ConfigParser& parser, const WCHAR* section);
	virtual void BindMeasures(ConfigParser& parser, const WCHAR* section);

	virtual bool IsFixedSize(bool overwrite = false) { return overwrite; }

private:
	enum TEXTSTYLE
	{
		NORMAL,
		BOLD,
		ITALIC,
		BOLDITALIC
	};

	enum TEXTEFFECT
	{
		EFFECT_NONE,
		EFFECT_SHADOW,
		EFFECT_BORDER
	};

	enum TEXTCASE
	{
		TEXTCASE_NONE,
		TEXTCASE_UPPER,
		TEXTCASE_LOWER,
		TEXTCASE_PROPER
	};

	enum CLIPTYPE
	{
		CLIP_OFF,
		CLIP_ON,
		CLIP_AUTO
	};

	bool DrawString(Gfx::Canvas& canvas, D2D1_RECT_F* rect);

	D2D1_COLOR_F m_Color;
	D2D1_COLOR_F m_EffectColor;
	std::wstring m_Postfix;
	std::wstring m_Prefix;
	std::wstring m_Text;
	std::wstring m_FontFace;
	AUTOSCALE m_AutoScale;
	TEXTSTYLE m_Style;
	TEXTEFFECT m_Effect;
	TEXTCASE m_Case;
	FLOAT m_FontSize;
	double m_Scale;
	bool m_NoDecimals;
	bool m_Percentual;
	CLIPTYPE m_ClipType;
	bool m_NeedsClipping;
	int m_ClipStringW;
	int m_ClipStringH;
	Gfx::TextFormat* m_TextFormat;
	int m_NumOfDecimals;
	FLOAT m_Angle;
	int m_FontWeight;
	bool m_TrailingSpaces;

	std::wstring m_String;
};
