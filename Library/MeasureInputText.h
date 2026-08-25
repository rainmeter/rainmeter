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
	BYTE opacity = 255U;

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
// The box is a window of its own rather than anything the skin draws, so it is opened on a worker
// thread: the skins behind it go on updating while it waits, which they could not do from a modal
// loop on the main thread. Nothing but the box itself happens there - every option is read, and
// every bang is run, back on the main thread between one prompt and the next.
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
	void ReadOptions(ConfigParser& parser, std::wstring_view section) override;
	void UpdateValue() override;
	const WCHAR* GetStringValue() override;
	void Command(const std::wstring& command) override;

private:
	// One line of a run: a bang, and the box that has to be answered before it goes.
	struct Step
	{
		// The bang to run, with its $UserInput$ token still in it where it has one.
		std::wstring command;

		// Set instead of |command| by the one word form, which names a variable rather than
		// writing out the bang that sets it.
		std::wstring variable;

		InputTextOptions options;
		bool prompts = false;
	};

	struct SharedData;
	class InputBox;
	class PromptTask;

	// Fills m_Steps from the argument of !CommandMeasure. |false| where there is nothing to run.
	bool ReadSteps(const std::wstring& command);

	// Runs from m_StepIndex until a box has to be opened, or until there is nothing left.
	void RunSteps();

	// Where a box that was opened comes back to. Nothing means it was dismissed, which ends the
	// run wherever it had got to.
	void HandleInput(const std::optional<std::wstring>& input);

	// Clears the run. |dismissed| is the end that OnDismissAction is for.
	void EndRun(bool dismissed);

	// Closes the box, if one is open. Safe while none is, and while one is on its way up.
	void CloseBox();

	InputTextOptions m_Options;
	std::wstring m_DismissAction;

	std::vector<Step> m_Steps;
	size_t m_StepIndex = 0U;

	// The measure's value: the text last typed into it.
	std::wstring m_Input;

	// Outlives the measure, so that the box and the task still have somewhere to look when a bang
	// refreshes the skin out from under them.
	std::shared_ptr<SharedData> m_Data;
	PromptTask* m_Task = nullptr;
};
