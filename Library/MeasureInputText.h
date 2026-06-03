/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_LIBRARY_MEASUREINPUTTEXT_H_
#define RM_LIBRARY_MEASUREINPUTTEXT_H_

#include "Measure.h"
#include <unordered_map>

class MeasureInputText : public Measure
{
public:
	using Options = std::unordered_map<std::wstring, std::wstring>;

	MeasureInputText(Skin* skin, const WCHAR* name);
	virtual ~MeasureInputText();

	MeasureInputText(const MeasureInputText& other) = delete;
	MeasureInputText& operator=(MeasureInputText other) = delete;

	virtual UINT GetTypeID() { return TypeID<MeasureInputText>(); }
	virtual const WCHAR* GetStringValue();
	virtual void Command(const std::wstring& command);

protected:
	virtual void ReadOptions(ConfigParser& parser, const WCHAR* section);
	virtual void UpdateValue();

private:
	enum class CommandType
	{
		Unknown,
		SetVariable,
		ExecuteBatch
	};

	struct CommandParam
	{
		CommandType type = CommandType::Unknown;
		std::wstring command;
		std::wstring dismissAction;
		Options options;
		std::vector<Options> overrideOptions;
		std::vector<std::wstring> commands;
	};

	bool ReadCommandOptions(CommandParam& param);
	void ReadOption(const WCHAR* option, Options& options, bool formula = false);

	bool ExecuteLine(std::wstring line, const Options& options, const Options& overrides);
	bool GetUserInput(const Options& options, const Options& overrides, std::wstring& result);

	void ExecuteDismissAction(const std::wstring& action);
	void ExecuteRainmeterCommand(const std::wstring& command);

	std::wstring ParseInlineOption(const std::wstring& data, bool formula);
	std::wstring ScanAndReplace(std::wstring line, const WCHAR* tagName, Options& overrides, bool formula = false);
	static int FindTag(const std::wstring& line, const WCHAR* tagName);
	static std::wstring ReadTagData(const std::wstring& line, const WCHAR* tagName, int start);
	static void ReplaceInsensitive(std::wstring& str, const std::wstring& find, const std::wstring& replace);

	bool m_Executing;
	std::wstring m_StringValue;
};

#endif
