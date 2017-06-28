/* Copyright (C) 2013 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "../Common/PathUtil.h"
#include "CommandHandler.h"
#include "ConfigParser.h"
#include "DialogAbout.h"
#include "DialogManage.h"
#include "Measure.h"
#include "Logger.h"
#include "Rainmeter.h"
#include "System.h"
#include "TrayIcon.h"
#include "resource.h"

namespace {

typedef void (* BangHandlerFunc)(std::vector<std::wstring>& args, Skin* skin);

struct BangInfo
{
	Bang bang;
	WCHAR* name;
	uint8_t argCount;
};

struct CustomBangInfo
{
	Bang bang;
	WCHAR* name;
	BangHandlerFunc handlerFunc;
};

// Bangs that are to be handled with DoBang().
const BangInfo s_Bangs[] =
{
	{ Bang::Refresh, L"Refresh", 0 },
	{ Bang::Redraw, L"Redraw", 0 },
	{ Bang::Update, L"Update", 0 },
	{ Bang::Hide, L"Hide", 0 },
	{ Bang::Show, L"Show", 0 },
	{ Bang::Toggle, L"Toggle", 0 },
	{ Bang::HideFade, L"HideFade", 0 },
	{ Bang::ShowFade, L"ShowFade", 0 },
	{ Bang::ToggleFade, L"ToggleFade", 0 },
	{ Bang::FadeDuration, L"FadeDuration", 1 },
	{ Bang::HideMeter, L"HideMeter", 1 },
	{ Bang::ShowMeter, L"ShowMeter", 1 },
	{ Bang::ToggleMeter, L"ToggleMeter", 1 },
	{ Bang::MoveMeter, L"MoveMeter", 3 },
	{ Bang::UpdateMeter, L"UpdateMeter", 1 },
	{ Bang::DisableMeasure, L"DisableMeasure", 1 },
	{ Bang::EnableMeasure, L"EnableMeasure", 1 },
	{ Bang::ToggleMeasure, L"ToggleMeasure", 1 },
	{ Bang::PauseMeasure, L"PauseMeasure", 1 },
	{ Bang::UnpauseMeasure, L"UnpauseMeasure", 1 },
	{ Bang::TogglePauseMeasure, L"TogglePauseMeasure", 1 },
	{ Bang::UpdateMeasure, L"UpdateMeasure", 1 },
	{ Bang::CommandMeasure, L"CommandMeasure", 2 },
	{ Bang::PluginBang, L"PluginBang", 1 },
	{ Bang::ShowBlur, L"ShowBlur", 0 },
	{ Bang::HideBlur, L"HideBlur", 0 },
	{ Bang::ToggleBlur, L"ToggleBlur", 0 },
	{ Bang::AddBlur, L"AddBlur", 1 },
	{ Bang::RemoveBlur, L"RemoveBlur", 1 },
	{ Bang::Move, L"Move", 2 },
	{ Bang::ZPos, L"ZPos", 1 },
	{ Bang::ZPos, L"ChangeZPos", 1 },  // For backwards compatibility.
	{ Bang::ChangeZPos, L"ChangeZPos", 1 },
	{ Bang::ClickThrough, L"ClickThrough", 1 },
	{ Bang::Draggable, L"Draggable", 1 },
	{ Bang::SnapEdges, L"SnapEdges", 1 },
	{ Bang::KeepOnScreen, L"KeepOnScreen", 1 },
	{ Bang::AutoSelectScreen, L"AutoSelectScreen", 1 },
	{ Bang::SetTransparency, L"SetTransparency", 1 },
	{ Bang::SetVariable, L"SetVariable", 2 },
	{ Bang::SetOption, L"SetOption", 3 },
	{ Bang::SetOptionGroup, L"SetOptionGroup", 3 },
	{ Bang::HideMeterGroup, L"HideMeterGroup", 1 },
	{ Bang::ShowMeterGroup, L"ShowMeterGroup", 1 },
	{ Bang::ToggleMeterGroup, L"ToggleMeterGroup", 1 },
	{ Bang::UpdateMeterGroup, L"UpdateMeterGroup", 1 },
	{ Bang::DisableMeasureGroup, L"DisableMeasureGroup", 1 },
	{ Bang::EnableMeasureGroup, L"EnableMeasureGroup", 1 },
	{ Bang::ToggleMeasureGroup, L"ToggleMeasureGroup", 1 },
	{ Bang::PauseMeasureGroup, L"PauseMeasureGroup", 1 },
	{ Bang::UnpauseMeasureGroup, L"UnpauseMeasureGroup", 1 },
	{ Bang::TogglePauseMeasureGroup, L"TogglePauseMeasureGroup", 1 },
	{ Bang::UpdateMeasureGroup, L"UpdateMeasureGroup", 1 },
	{ Bang::SkinCustomMenu, L"SkinCustomMenu", 0 }
};

// Bangs that are to be handled with DoGroupBang().
// TODO: Better handling of Bang-id
const BangInfo s_GroupBangs[] =
{
	{ Bang::Refresh, L"RefreshGroup", 0 },
	{ Bang::Update, L"UpdateGroup", 0 },
	{ Bang::Redraw, L"RedrawGroup", 0 },
	{ Bang::Hide, L"HideGroup", 0 },
	{ Bang::Show, L"ShowGroup", 0 },
	{ Bang::Toggle, L"ToggleGroup", 0 },
	{ Bang::HideFade, L"HideFadeGroup", 0 },
	{ Bang::ShowFade, L"ShowFadeGroup", 0 },
	{ Bang::ToggleFade, L"ToggleFadeGroup", 0 },
	{ Bang::ZPos, L"ZPosGroup", 1 },
	{ Bang::ClickThrough, L"ClickThroughGroup", 1 },
	{ Bang::Draggable, L"DraggableGroup", 1 },
	{ Bang::SnapEdges, L"SnapEdgesGroup", 1 },
	{ Bang::FadeDuration, L"FadeDurationGroup", 1 },
	{ Bang::KeepOnScreen, L"KeepOnScreenGroup", 1 },
	{ Bang::AutoSelectScreen, L"AutoSelectScreenGroup", 1 },
	{ Bang::SetTransparency, L"SetTransparencyGroup", 1 },
	{ Bang::SetVariable, L"SetVariableGroup", 2 }
};

// Bangs that are to be handled using a custom handler function.
const CustomBangInfo s_CustomBangs[] =
{
	{ Bang::ActivateConfig, L"ActivateConfig", CommandHandler::DoActivateSkinBang },
	{ Bang::DeactivateConfig, L"DeactivateConfig", CommandHandler::DoDeactivateSkinBang },
	{ Bang::ToggleConfig, L"ToggleConfig", CommandHandler::DoToggleSkinBang },
	{ Bang::DeactivateConfigGroup, L"DeactivateConfigGroup", CommandHandler::DoDeactivateSkinGroupBang },
	{ Bang::WriteKeyValue, L"WriteKeyValue", CommandHandler::DoWriteKeyValueBang },
	{ Bang::LoadLayout, L"LoadLayout", CommandHandler::DoLoadLayoutBang },
	{ Bang::SetClip, L"SetClip", CommandHandler::DoSetClipBang },
	{ Bang::SetWallpaper, L"SetWallpaper", CommandHandler::DoSetWallpaperBang },
	{ Bang::About, L"About", CommandHandler::DoAboutBang },
	{ Bang::Manage, L"Manage", CommandHandler::DoManageBang },
	{ Bang::SkinMenu, L"SkinMenu", CommandHandler::DoSkinMenuBang },
	{ Bang::TrayMenu, L"TrayMenu", CommandHandler::DoTrayMenuBang },
	{ Bang::ResetStats, L"ResetStats", CommandHandler::DoResetStatsBang },
	{ Bang::Log, L"Log", CommandHandler::DoLogBang },
	{ Bang::RefreshApp, L"RefreshApp", CommandHandler::DoRefreshApp },
	{ Bang::Quit, L"Quit", CommandHandler::DoQuitBang },
	{ Bang::EditSkin, L"EditSkin", CommandHandler::DoEditSkinBang },
	{ Bang::LsBoxHook, L"LsBoxHook", CommandHandler::DoLsBoxHookBang }
};

void DoBang(const BangInfo& bangInfo, std::vector<std::wstring>& args, Skin* skin)
{
	const size_t argsCount = args.size();
	if (argsCount >= bangInfo.argCount)
	{
		if (argsCount == bangInfo.argCount && skin)
		{
			skin->DoBang(bangInfo.bang, args);
		}
		else
		{
			// Use the specified window instead of skin parameter.
			if (argsCount > bangInfo.argCount)
			{
				std::wstring& folderPath = args[bangInfo.argCount];
				if (!folderPath.empty() && (folderPath.length() != 1 || folderPath[0] != L'*'))
				{
					Skin* skin = GetRainmeter().GetSkin(folderPath);
					if (skin)
					{
						skin->DoBang(bangInfo.bang, args);
					}
					else
					{
						LogErrorF(skin, L"!%s: Skin \"%s\" not found", bangInfo.name, folderPath.c_str());
					}
					return;
				}
			}

			// No skin defined -> apply to all.
			for (const auto& ip : GetRainmeter().GetAllSkins())
			{
				ip.second->DoBang(bangInfo.bang, args);
			}
		}
	}
	else
	{
		// For backwards compatibility.
		if (bangInfo.bang == Bang::CommandMeasure && argsCount >= 1)
		{
			std::wstring& firstArg = args[0];
			std::wstring::size_type pos = firstArg.find_first_of(L' ');
			if (pos != std::wstring::npos)
			{
				std::wstring newArg = firstArg.substr(0, pos);
				firstArg.erase(0, pos + 1);
				args.insert(args.begin(), newArg);

				LogWarningF(skin, L"!%s: Two parameters required, only one given", bangInfo.name);
				DoBang(bangInfo, args, skin);
				return;
			}
		}

		LogErrorF(skin, L"!%s: Incorrect number of arguments", bangInfo.name);
	}
}

void DoGroupBang(const BangInfo& bangInfo, std::vector<std::wstring>& args, Skin* skin)
{
	if (args.size() > bangInfo.argCount)
	{
		std::multimap<int, Skin*> windows;
		GetRainmeter().GetSkinsByLoadOrder(windows, args[bangInfo.argCount]);

		// Remove extra parameters (including group).
		args.resize(bangInfo.argCount);

		for (const auto& ip : windows)
		{
			DoBang(bangInfo, args, ip.second);
		}
	}
	else
	{
		LogErrorF(skin, L"!%s: Incorrect number of arguments", bangInfo.name);
	}
}

}  // namespace

/*
** Parses and executes the given command.
**
*/
void CommandHandler::ExecuteCommand(const WCHAR* command, Skin* skin, bool multi)
{
	if (command[0] == L'!')	// Bang
	{
		++command;	// Skip "!"

		if (_wcsnicmp(L"Execute", command, 7) == 0)
		{
			command += 7;
			command = wcschr(command, L'[');
			if (!command) return;
		}
		else
		{
			if (_wcsnicmp(command, L"Rainmeter", 9) == 0)
			{
				// Skip "Rainmeter" for backwards compatibility
				command += 9;
			}

			std::wstring bang;
			std::vector<std::wstring> args;

			// Find the first space
			const WCHAR* pos = wcschr(command, L' ');
			if (pos)
			{
				bang.assign(command, 0, pos - command);
				args = ParseString(pos + 1, skin ? &skin->GetParser() : nullptr);
			}
			else
			{
				bang = command;
			}

			ExecuteBang(bang.c_str(), args, skin);
			return;
		}
	}

	if (multi && command[0] == L'[')	// Multi-bang
	{
		std::wstring bangs = command;
		std::wstring::size_type start = std::wstring::npos;
		int count = 0;
		for (size_t i = 0, isize = bangs.size(); i < isize; ++i)
		{
			if (bangs[i] == L'[')
			{
				if (count == 0)
				{
					start = i;
				}
				++count;
			}
			else if (bangs[i] == L']')
			{
				--count;

				if (count == 0 && start != std::wstring::npos)
				{
					// Change ] to nullptr
					bangs[i] = L'\0';

					// Skip whitespace
					start = bangs.find_first_not_of(L" \t\r\n", start + 1, 4);

					const WCHAR* newCommand = bangs.c_str() + start;
					if (skin && _wcsnicmp(newCommand, L"!Delay ", wcslen(L"!Delay ")) == 0)
					{
						auto args = ParseString(newCommand + wcslen(L"!Delay "), &skin->GetParser());
						if (args.size() == 1)
						{
							auto delay = ConfigParser::ParseUInt(args[0].c_str(), 0);
							skin->DoDelayedCommand(bangs.c_str() + i + 1, delay);
							return;
						}
					}
					else
					{
						ExecuteCommand(newCommand, skin, false);
					}
				}
			}
			else if (bangs[i] == L'"' && isize > (i + 2) && bangs[i + 1] == L'"' && bangs[i + 2] == L'"')
			{
				i += 3;

				std::wstring::size_type pos = bangs.find(L"\"\"\"", i);
				if (pos != std::wstring::npos)
				{
					i = pos + 2;	// Skip "", loop will skip last "
				}
			}
		}
	}
	else
	{
		// Check for built-ins
		if (_wcsnicmp(L"PLAY", command, 4) == 0)
		{
			if (command[4] == L' ' ||                      // PLAY
				_wcsnicmp(L"LOOP ", &command[4], 5) == 0)  // PLAYLOOP
			{
				command += 4;	// Skip PLAY

				DWORD flags = SND_FILENAME | SND_ASYNC;

				if (command[0] != L' ')
				{
					flags |= SND_LOOP | SND_NODEFAULT;
					command += 4;	// Skip LOOP
				}

				++command;	// Skip the space
				if (command[0] != L'\0')
				{
					std::wstring sound = command;

					// Strip the quotes
					std::wstring::size_type len = sound.length();
					if (len >= 2 && sound[0] == L'"' && sound[len - 1] == L'"')
					{
						len -= 2;
						sound.assign(sound, 1, len);
					}

					if (skin)
					{
						skin->GetParser().ReplaceMeasures(sound);
						skin->MakePathAbsolute(sound);
					}

					PlaySound(sound.c_str(), nullptr, flags);
				}
				return;
			}
			else if (_wcsnicmp(L"STOP", &command[4], 4) == 0)  // PLAYSTOP
			{
				PlaySound(nullptr, nullptr, SND_PURGE);
				return;
			}
		}

		// Run command
		std::wstring tmpSz = command;
		if (skin)
		{
			skin->GetParser().ReplaceMeasures(tmpSz);
		}
		RunCommand(tmpSz);
	}
}

/*
** Runs the given bang.
**
*/
void CommandHandler::ExecuteBang(const WCHAR* name, std::vector<std::wstring>& args, Skin* skin)
{
	for (const auto& bangInfo : s_Bangs)
	{
		if (_wcsicmp(bangInfo.name, name) == 0)
		{
			DoBang(bangInfo, args, skin);
			return;
		}
	}

	for (const auto& bangInfo : s_GroupBangs)
	{
		if (_wcsicmp(bangInfo.name, name) == 0)
		{
			DoGroupBang(bangInfo, args, skin);
			return;
		}
	}

	for (const auto& bangInfo : s_CustomBangs)
	{
		if (_wcsicmp(bangInfo.name, name) == 0)
		{
			bangInfo.handlerFunc(args, skin);
			return;
		}
	}

	LogErrorF(skin, L"Invalid bang: !%s", name);
}

/*
** Parses and runs the given command.
**
*/
void CommandHandler::RunCommand(std::wstring command)
{
	std::wstring args;

	size_t notwhite = command.find_first_not_of(L" \t\r\n");
	command.erase(0, notwhite);

	size_t quotePos = command.find(L'"');
	if (quotePos == 0)
	{
		size_t quotePos2 = command.find(L'"', quotePos + 1);
		if (quotePos2 != std::wstring::npos)
		{
			args.assign(command, quotePos2 + 1, command.length() - (quotePos2 + 1));
			command.assign(command, quotePos + 1, quotePos2 - quotePos - 1);
		}
		else
		{
			command.erase(0, 1);
		}
	}
	else
	{
		size_t spacePos = command.find(L' ');
		if (spacePos != std::wstring::npos)
		{
			args.assign(command, spacePos + 1, command.length() - (spacePos + 1));
			command.erase(spacePos);
		}
	}

	if (!command.empty())
	{
		RunFile(command.c_str(), args.c_str());
	}
}

/*
** Runs a file with the given arguments.
**
*/
void CommandHandler::RunFile(const WCHAR* file, const WCHAR* args)
{
	SHELLEXECUTEINFO si = {sizeof(SHELLEXECUTEINFO)};
	si.lpVerb = L"open";
	si.lpFile = file;
	si.nShow = SW_SHOWNORMAL;

	DWORD type = GetFileAttributes(si.lpFile);
	if (type & FILE_ATTRIBUTE_DIRECTORY && type != 0xFFFFFFFF)
	{
		ShellExecute(si.hwnd, si.lpVerb, si.lpFile, nullptr, nullptr, si.nShow);
	}
	else
	{
		std::wstring dir = PathUtil::GetFolderFromFilePath(file);
		si.lpDirectory = dir.c_str();
		si.lpParameters = args;
		si.fMask = SEE_MASK_DOENVSUBST | SEE_MASK_FLAG_NO_UI;
		ShellExecuteEx(&si);
	}
}

/*
** Splits strings into parts.
**
*/
std::vector<std::wstring> CommandHandler::ParseString(const WCHAR* str, ConfigParser* parser)
{
	std::vector<std::wstring> result;

	if (str)
	{
		std::wstring arg = str;

		// Split the argument between first space.
		// Or if string is in quotes, the after the second quote.

		auto addResult = [&](std::wstring& string, bool stripQuotes)
		{
			if (stripQuotes)
			{
				size_t pos = 0;
				do
				{
					pos = string.find(L'"', pos);
					if (pos != std::wstring::npos)
					{
						string.erase(pos, 1);
					}
				}
				while (pos != std::wstring::npos);
			}

			if (parser)
			{
				parser->ReplaceMeasures(string);
			}

			result.push_back(string);
		};

		size_t pos;
		std::wstring newStr;
		while ((pos = arg.find_first_not_of(L' ')) != std::wstring::npos)
		{
			size_t extra = 1;
			if (arg[pos] == L'"')
			{
				if (arg.size() > (pos + 2) &&
					arg[pos + 1] == L'"' && arg[pos + 2] == L'"')
				{
					// Eat found quotes and finding ending """
					arg.erase(0, pos + 3);

					extra = 4;
					if ((pos = arg.find(L"\"\"\" ")) == std::wstring::npos)
					{
						extra = 3;
						pos = arg.rfind(L"\"\"\"");  // search backward
					}
				}
				else
				{
					// Eat found quote and find ending quote 
					arg.erase(0, pos + 1);
					pos = arg.find_first_of(L'"');
				}
			}
			else
			{
				if (pos > 0)
				{
					// Eat everything until non-space (and non-quote) char
					arg.erase(0, pos);
				}

				// Find the second quote
				pos = arg.find_first_of(L' ');
			}

			if (pos != std::wstring::npos)
			{
				newStr.assign(arg, 0, pos);
				arg.erase(0, pos + extra);

				addResult(newStr, extra == 1);
			}
			else  // quote or space not found
			{
				addResult(arg, extra == 1);
				arg.clear();
				break;
			}
		}

		if (!arg.empty() && result.empty())
		{
			addResult(arg, true);
		}
	}

	return result;
}

void CommandHandler::DoActivateSkinBang(std::vector<std::wstring>& args, Skin* skin)
{
	if (args.size() == 1)
	{
		if (GetRainmeter().ActivateSkin(args[0])) return;
	}
	else if (args.size() > 1)
	{
		if (GetRainmeter().ActivateSkin(args[0], args[1])) return;
	}

	LogErrorF(skin, L"!ActivateConfig: Invalid parameters");
}

void CommandHandler::DoDeactivateSkinBang(std::vector<std::wstring>& args, Skin* skin)
{
	if (!args.empty())
	{
		skin = GetRainmeter().GetSkin(args[0]);
		if (!skin)
		{
			LogWarningF(L"!DeactivateConfig: \"%s\" not active", args[0].c_str());
			return;
		}
	}

	if (skin)
	{
		GetRainmeter().DeactivateSkin(skin, -1);
	}
	else
	{
		LogError(L"!DeactivateConfig: Invalid parameters");
	}
}

void CommandHandler::DoToggleSkinBang(std::vector<std::wstring>& args, Skin* skin)
{
	if (args.size() >= 2)
	{
		Skin* skin = GetRainmeter().GetSkin(args[0]);
		if (skin)
		{
			GetRainmeter().DeactivateSkin(skin, -1);
			return;
		}

		// If the skin wasn't active, activate it.
		DoActivateSkinBang(args, nullptr);
	}
	else
	{
		LogErrorF(skin, L"!ToggleConfig: Invalid parameters");
	}
}

void CommandHandler::DoDeactivateSkinGroupBang(std::vector<std::wstring>& args, Skin* skin)
{
	if (!args.empty())
	{
		std::multimap<int, Skin*> windows;
		GetRainmeter().GetSkinsByLoadOrder(windows, args[0]);
		for (const auto& ip : windows)
		{
			GetRainmeter().DeactivateSkin(ip.second, -1);
		}
	}
	else
	{
		LogErrorF(skin, L"!DeactivateConfigGroup: Invalid parameters");
	}
}

void CommandHandler::DoLoadLayoutBang(std::vector<std::wstring>& args, Skin* skin)
{
	if (args.size() == 1)
	{
		if (skin)
		{
			// Delay to avoid loading theme in the middle of an update.
			std::wstring command = L"!LoadLayout \"";
			command += args[0];
			command += L'"';
			GetRainmeter().DelayedExecuteCommand(command.c_str());
		}
		else
		{
			// Not called from a skin (or called with delay).
			GetRainmeter().LoadLayout(args[0]);
		}
	}
}

void CommandHandler::DoSetClipBang(std::vector<std::wstring>& args, Skin* skin)
{
	if (!args.empty())
	{
		System::SetClipboardText(args[0]);
	}
	else
	{
		LogErrorF(skin, L"!SetClip: Invalid parameter");
	}
}

void CommandHandler::DoSetWallpaperBang(std::vector<std::wstring>& args, Skin* skin)
{
	const size_t argsSize = args.size();
	if (argsSize >= 1 && argsSize <= 2)
	{
		std::wstring& file = args[0];
		const std::wstring& style = (argsSize == 2) ? args[1] : L"";

		if (skin)
		{
			skin->MakePathAbsolute(file);
		}

		System::SetWallpaper(file, style);
	}
	else
	{
		LogErrorF(skin, L"!SetWallpaper: Invalid parameters");
	}
}

void CommandHandler::DoAboutBang(std::vector<std::wstring>& args, Skin* skin)
{
	DialogAbout::Open(args.empty() ? L"" : args[0].c_str());
}

void CommandHandler::DoManageBang(std::vector<std::wstring>& args, Skin* skin)
{
	const size_t argsSize = args.size();
	if (argsSize >= 2 && argsSize <= 3)
	{
		DialogManage::Open(args[0].c_str(),
			args[1].c_str(),
			(argsSize == 3) ? args[2].c_str() : L"");
	}
	else if (argsSize <= 1)
	{
		DialogManage::Open(args.empty() ? L"" : args[0].c_str());
	}
	else
	{
		LogErrorF(skin, L"!Manage: Invalid parameters");
	}
}

void CommandHandler::DoSkinMenuBang(std::vector<std::wstring>& args, Skin* skin)
{
	if (!args.empty())
	{
		skin = GetRainmeter().GetSkin(args[0]);
		if (!skin)
		{
			LogWarningF(L"!SkinMenu: \"%s\" not active", args[0].c_str());
			return;
		}
	}

	if (skin)
	{
		POINT pos = System::GetCursorPosition();
		GetRainmeter().ShowContextMenu(pos, skin);
	}
	else
	{
		LogError(L"!SkinMenu: Invalid parameter");
	}
}

void CommandHandler::DoTrayMenuBang(std::vector<std::wstring>& args, Skin* skin)
{
	POINT pos = System::GetCursorPosition();
	GetRainmeter().ShowContextMenu(pos, nullptr);
}

void CommandHandler::DoResetStatsBang(std::vector<std::wstring>& args, Skin* skin)
{
	GetRainmeter().ResetStats();
}

void CommandHandler::DoWriteKeyValueBang(std::vector<std::wstring>& args, Skin* skin)
{
	if (args.size() == 3 && skin)
	{
		// Add the skin file path to the args
		args.push_back(skin->GetFilePath());
	}
	else if (args.size() < 4)
	{
		LogErrorF(skin, L"!WriteKeyValue: Invalid parameters");
		return;
	}

	std::wstring& strIniFile = args[3];
	if (skin)
	{
		skin->MakePathAbsolute(strIniFile);
	}

	const WCHAR* iniFile = strIniFile.c_str();

	if (strIniFile.find(L"..\\") != std::wstring::npos || strIniFile.find(L"../") != std::wstring::npos)
	{
		LogErrorF(skin, L"!WriteKeyValue: Illegal path: %s", iniFile);
		return;
	}

	if (_wcsnicmp(iniFile, GetRainmeter().m_SkinPath.c_str(), GetRainmeter().m_SkinPath.size()) != 0 &&
		_wcsnicmp(iniFile, GetRainmeter().m_SettingsPath.c_str(), GetRainmeter().m_SettingsPath.size()) != 0)
	{
		LogErrorF(skin, L"!WriteKeyValue: Illegal path: %s", iniFile);
		return;
	}

	// Verify whether the file exists.
	if (_waccess(iniFile, 0) == -1)
	{
		LogErrorF(skin, L"!WriteKeyValue: File not found: %s", iniFile);
		return;
	}

	// Verify whether the file is read-only.
	DWORD attr = GetFileAttributes(iniFile);
	if (attr == -1 || (attr & FILE_ATTRIBUTE_READONLY))
	{
		LogWarningF(skin, L"!WriteKeyValue: File is read-only: %s", iniFile);
		return;
	}

	// Avoid "IniFileMapping"
	System::UpdateIniFileMappingList();
	std::wstring strIniWrite = System::GetTemporaryFile(strIniFile);
	if (strIniWrite.size() == 1 && strIniWrite[0] == L'?')  // error occurred
	{
		return;
	}

	bool temporary = !strIniWrite.empty();

	if (temporary)
	{
		if (GetRainmeter().GetDebug())
		{
			LogDebugF(skin, L"!WriteKeyValue: Writing to: %s (Temp: %s)", iniFile, strIniWrite.c_str());
		}
	}
	else
	{
		if (GetRainmeter().GetDebug())
		{
			LogDebugF(skin, L"!WriteKeyValue: Writing to: %s", iniFile);
		}
		strIniWrite = strIniFile;
	}

	const WCHAR* iniWrite = strIniWrite.c_str();
	const WCHAR* section = args[0].c_str();
	const WCHAR* key = args[1].c_str();
	const std::wstring& strValue = args[2];

	bool formula = false;
	BOOL write = 0;

	if (skin)
	{
		double value;
		formula = skin->GetParser().ParseFormula(strValue, &value); 
		if (formula)
		{
			WCHAR buffer[256];
			int len = _snwprintf_s(buffer, _TRUNCATE, L"%.5f", value);
			Measure::RemoveTrailingZero(buffer, len);

			write = WritePrivateProfileString(section, key, buffer, iniWrite);
		}
	}

	if (!formula)
	{
		write = WritePrivateProfileString(section, key, strValue.c_str(), iniWrite);
	}

	if (temporary)
	{
		if (write != 0)
		{
			WritePrivateProfileString(nullptr, nullptr, nullptr, iniWrite);  // FLUSH

			// Copy the file back.
			if (!System::CopyFiles(strIniWrite, strIniFile))
			{
				LogErrorF(skin, L"!WriteKeyValue: Failed to copy temporary file to original filepath: %s (Temp: %s)", iniFile, iniWrite);
			}
		}
		else  // failed
		{
			LogErrorF(skin, L"!WriteKeyValue: Failed to write to: %s (Temp: %s)", iniFile, iniWrite);
		}

		// Remove the temporary file.
		System::RemoveFile(strIniWrite);
	}
	else
	{
		if (write == 0)  // failed
		{
			LogErrorF(skin, L"!WriteKeyValue: Failed to write to: %s", iniFile);
		}
	}
}

void CommandHandler::DoLogBang(std::vector<std::wstring>& args, Skin* skin)
{
	if (!args.empty())
	{
		Logger::Level level = Logger::Level::Notice;
		if (args.size() > 1)
		{
			const WCHAR* type = args[1].c_str();
			if (_wcsicmp(type, L"ERROR") == 0)
			{
				level = Logger::Level::Error;
			}
			else if (_wcsicmp(type, L"WARNING") == 0)
			{
				level = Logger::Level::Warning;
			}
			else if (_wcsicmp(type, L"DEBUG") == 0)
			{
				level = Logger::Level::Debug;
			}
			else if (_wcsicmp(type, L"NOTICE") != 0)
			{
				LogErrorF(skin, L"!Log: Invalid type");
				return;
			}
		}

		std::wstring source;
		if (skin)
		{
			source = skin->GetSkinPath();
		}

		GetLogger().Log(level, source.c_str(), args[0].c_str());
	}
}

void CommandHandler::DoRefreshApp(std::vector<std::wstring>& args, Skin* skin)
{
	// Refresh needs to be delayed since it crashes if done during Update().
	PostMessage(GetRainmeter().m_Window, WM_RAINMETER_DELAYED_REFRESH_ALL, 0, 0);
}

void CommandHandler::DoQuitBang(std::vector<std::wstring>& args, Skin* skin)
{
	// Quit needs to be delayed since it crashes if done during Update().
	PostMessage(GetRainmeter().GetTrayIcon()->GetWindow(), WM_COMMAND, MAKEWPARAM(IDM_QUIT, 0), 0);
}

void CommandHandler::DoEditSkinBang(std::vector<std::wstring>& args, Skin* skin)
{
	const size_t argSize = args.size();
	if (argSize > 1)
	{
		const SkinRegistry::Indexes indexes = GetRainmeter().m_SkinRegistry.FindIndexes(args[0], args[1]);
		if (indexes.IsValid())
		{
			GetRainmeter().EditSkinFile(args[0], args[1]);
		}
		else
		{
			LogErrorF(L"!EditSkin: Invalid parameters");
		}
	}
	else if (argSize == 0 && skin)
	{
		GetRainmeter().EditSkinFile(skin->GetFolderPath(), skin->GetFileName());
	}
	else
	{
		LogErrorF(skin, L"!EditSkin: Invalid parameters");
	}
}

void CommandHandler::DoLsBoxHookBang(std::vector<std::wstring>& args, Skin* skin)
{
	// Deprecated.
}
