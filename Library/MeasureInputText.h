// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "Measure.h"
#include <memory>
#include <optional>

// Everything a prompt is drawn from. The measure reads one of these from its own section, and each
// line of a batch takes a copy with the settings written on that line laid over it - so the values
// here are the skin's, in skin units, and are turned into pixels only when a box opens with them.
struct InputTextOptions
{
	// The defaults are those of the WinForms box this grew out of, so that a skin that sets none
	// of them gets the box it has always had.
	std::wstring text;
	std::wstring fontFace = L"Microsoft Sans Serif";
	double fontSize = 8.25;
	bool bold = false;
	bool italic = false;
	DWORD align = ES_LEFT;
	int x = 0;
	int y = 0;
	int w = 200;
	int h = 22;
	COLORREF fontColor = GetSysColor(COLOR_WINDOWTEXT);
	COLORREF backColor = GetSysColor(COLOR_WINDOW);

	// SolidColor's alpha, which dims the whole box rather than only what is behind the text.
	BYTE opacity = 255;

	int maxLength = 0;
	bool numeric = false;
	bool password = false;
	bool focusDismiss = true;

	// Unset follows the skin, which is what TopMost=AUTO means.
	std::optional<bool> topMost;
};

// A measure the user types into. !CommandMeasure opens a box over the skin and waits for it: given
// one word, the word names a variable and the text becomes its value; given "ExecuteBatch", the
// measure's own Command1..N lines are run in order, opening a box for each one that holds a
// $UserInput$ token and stopping where a box is dismissed.
//
// The box is a modeless window of its own rather than anything the skin draws. It uses Rainmeter's
// message loop, so skins keep updating while it waits and every part of a run stays on the main
// thread.
class MeasureInputText : public Measure
{
public:
	MeasureInputText(Skin* skin, const WCHAR* name);
	virtual ~MeasureInputText();

	MeasureInputText(const MeasureInputText& other) = delete;
	MeasureInputText& operator=(MeasureInputText other) = delete;

	UINT GetTypeID() override { return TypeID<MeasureInputText>(); }

	// Closes every box open over |skin|. What one was placed and sized with came from a scale that
	// has just changed, so it is dismissed rather than left standing at the old one.
	static void HandleSkinScaleChange(Skin* skin);

protected:
	void UpdateValue() override {}
	std::optional<std::wstring_view> GetStringValue() override;
	void Command(const std::wstring& command) override;

private:
	struct Step
	{
		std::wstring command;
		std::wstring variable;
		InputTextOptions options;
		bool prompts = false;
	};

	class InputBox;

	bool ReadSteps(ConfigParser::OptionReader& reader, const std::wstring& command);
	void RunSteps();
	void HandleInput(const std::optional<std::wstring>& input);
	void EndRun(bool dismissed);
	void CloseBox();

	InputTextOptions m_Options;
	std::wstring m_DismissAction;

	std::vector<Step> m_Steps;
	size_t m_StepIndex = 0;

	std::wstring m_Input;

	// Outlives the measure, so a window callback can notice when a bang refreshed the skin out from
	// under it.
	std::shared_ptr<MeasureInputText*> m_MeasureRef;
	std::shared_ptr<InputBox> m_Box;
};
