/*
  Copyright (C) 2013 Rainmeter Team

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "StdAfx.h"
#include "CommandHandler.h"
#include "ConfigParser.h"
#include "DialogAbout.h"
#include "DialogManage.h"
#include "Measure.h"
#include "Logger.h"
#include "Rainmeter.h"
#include "System.h"
#include "TrayWindow.h"
#include "resource.h"

extern CRainmeter* Rainmeter;

namespace {

typedef void (* BangHandlerFunc)(std::vector<std::wstring>& args, CMeterWindow* skin);

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
	{ Bang::HideMeter, L"HideMeter", 1 },
	{ Bang::ShowMeter, L"ShowMeter", 1 },
	{ Bang::ToggleMeter, L"ToggleMeter", 1 },
	{ Bang::MoveMeter, L"MoveMeter", 3 },
	{ Bang::UpdateMeter, L"UpdateMeter", 1 },
	{ Bang::DisableMeasure, L"DisableMeasure", 1 },
	{ Bang::EnableMeasure, L"EnableMeasure", 1 },
	{ Bang::ToggleMeasure, L"ToggleMeasure", 1 },
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
	{ Bang::UpdateMeasureGroup, L"UpdateMeasureGroup", 1 }
};

// Bangs that are to be handled with DoGroupBang().
const BangInfo s_GroupBangs[] =
{
	{ Bang::RefreshGroup, L"RefreshGroup", 0 },
	{ Bang::UpdateGroup, L"UpdateGroup", 0 },
	{ Bang::RedrawGroup, L"RedrawGroup", 0 },
	{ Bang::HideGroup, L"HideGroup", 0 },
	{ Bang::ShowGroup, L"ShowGroup", 0 },
	{ Bang::ToggleGroup, L"ToggleGroup", 0 },
	{ Bang::HideFadeGroup, L"HideFadeGroup", 0 },
	{ Bang::ShowFadeGroup, L"ShowFadeGroup", 0 },
	{ Bang::ToggleFadeGroup, L"ToggleFadeGroup", 0 },
	{ Bang::DeactivateConfigGroup, L"DeactivateConfigGroup" },
	{ Bang::ZPosGroup, L"ZPosGroup", 1 },
	{ Bang::ClickThroughGroup, L"ClickThroughGroup", 1 },
	{ Bang::DraggableGroup, L"DraggableGroup", 1 },
	{ Bang::SnapEdgesGroup, L"SnapEdgesGroup", 1 },
	{ Bang::KeepOnScreenGroup, L"KeepOnScreenGroup", 1 },
	{ Bang::SetTransparencyGroup, L"SetTransparencyGroup", 1 },
	{ Bang::SetVariableGroup, L"SetVariableGroup", 2 }
};

// Bangs that are to be handled using a custom handler function.
const CustomBangInfo s_CustomBangs[] =
{
	{ Bang::ActivateConfig, L"ActivateConfig", CCommandHandler::DoActivateSkinBang },
	{ Bang::DeactivateConfig, L"DeactivateConfig", CCommandHandler::DoDeactivateSkinBang },
	{ Bang::ToggleConfig, L"ToggleConfig", CCommandHandler::DoToggleSkinBang },
	{ Bang::WriteKeyValue, L"WriteKeyValue", CCommandHandler::DoWriteKeyValueBang },
	{ Bang::LoadLayout, L"LoadLayout", CCommandHandler::DoLoadLayoutBang },
	{ Bang::SetClip, L"SetClip", CCommandHandler::DoSetClipBang },
	{ Bang::SetWallpaper, L"SetWallpaper", CCommandHandler::DoSetWallpaperBang },
	{ Bang::About, L"About", CCommandHandler::DoAboutBang },
	{ Bang::Manage, L"Manage", CCommandHandler::DoManageBang },
	{ Bang::SkinMenu, L"SkinMenu", CCommandHandler::DoSkinMenuBang },
	{ Bang::TrayMenu, L"TrayMenu", CCommandHandler::DoTrayMenuBang  },
	{ Bang::ResetStats, L"ResetStats", CCommandHandler::DoResetStatsBang  },
	{ Bang::Log, L"Log", CCommandHandler::DoLogBang  },
	{ Bang::RefreshApp, L"RefreshApp", CCommandHandler::DoRefreshApp },
	{ Bang::Quit, L"Quit", CCommandHandler::DoQuitBang  },
	{ Bang::LsBoxHook, L"LsBoxHook", CCommandHandler::DoLsBoxHookBang }
};

void DoBang(const BangInfo& bangInfo, std::vector<std::wstring>& args, CMeterWindow* skin)
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
				const std::wstring& folderPath = args[bangInfo.argCount];
				if (!folderPath.empty() && (folderPath.length() != 1 || folderPath[0] != L'*'))
				{
					CMeterWindow* skin = Rainmeter->GetMeterWindow(folderPath);
					if (skin)
					{
						skin->DoBang(bangInfo.bang, args);
					}
					else
					{
						LogErrorF(L"!%s: Skin \"%s\" not found", folderPath.c_str(), bangInfo.name);
					}
					return;
				}
			}

			// No skin defined -> apply to all.
			for (const auto& ip : Rainmeter->GetAllMeterWindows())
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

				LogWarningF(L"!%s: Two parameters required, only one given", bangInfo.name);
				DoBang(bangInfo, args, skin);
				return;
			}
		}

		LogErrorF(L"!%s: Incorrect number of arguments", bangInfo.name);
	}
}

void DoGroupBang(const BangInfo& bangInfo, std::vector<std::wstring>& args, CMeterWindow* skin)
{
	if (args.size() > bangInfo.argCount)
	{
		std::multimap<int, CMeterWindow*> windows;
		Rainmeter->GetMeterWindowsByLoadOrder(windows, args[bangInfo.argCount]);

		// Remove extra parameters (including group).
		args.resize(bangInfo.argCount);

		std::multimap<int, CMeterWindow*>::const_iterator iter = windows.begin();
		for (const auto& ip : windows)
		{
			DoBang(bangInfo, args, ip.second);
		}
	}
	else
	{
		LogErrorF(L"!%s: Incorrect number of arguments", bangInfo.name);
	}
}

}  // namespace

/*
** Parses and executes the given command.
**
*/
void CCommandHandler::ExecuteCommand(const WCHAR* command, CMeterWindow* skin, bool multi)
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
				args = ParseString(pos + 1, skin ? &skin->GetParser() : NULL);
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
					// Change ] to NULL
					bangs[i] = L'\0';

					// Skip whitespace
					start = bangs.find_first_not_of(L" \t\r\n", start + 1, 4);

					ExecuteCommand(bangs.c_str() + start, skin, false);
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

					PlaySound(sound.c_str(), NULL, flags);
				}
				return;
			}
			else if (_wcsnicmp(L"STOP", &command[4], 4) == 0)  // PLAYSTOP
			{
				PlaySound(NULL, NULL, SND_PURGE);
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
void CCommandHandler::ExecuteBang(const WCHAR* name, std::vector<std::wstring>& args, CMeterWindow* skin)
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

	LogErrorF(L"Invalid bang: !%s", name);
}

/*
** Parses and runs the given command.
**
*/
void CCommandHandler::RunCommand(std::wstring command)
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
void CCommandHandler::RunFile(const WCHAR* file, const WCHAR* args)
{
	SHELLEXECUTEINFO si = {sizeof(SHELLEXECUTEINFO)};
	si.lpVerb = L"open";
	si.lpFile = file;
	si.nShow = SW_SHOWNORMAL;

	DWORD type = GetFileAttributes(si.lpFile);
	if (type & FILE_ATTRIBUTE_DIRECTORY && type != 0xFFFFFFFF)
	{
		ShellExecute(si.hwnd, si.lpVerb, si.lpFile, NULL, NULL, si.nShow);
	}
	else
	{
		std::wstring dir = CRainmeter::ExtractPath(file);
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
std::vector<std::wstring> CCommandHandler::ParseString(LPCTSTR str, CConfigParser* parser)
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

void CCommandHandler::DoActivateSkinBang(std::vector<std::wstring>& args, CMeterWindow* skin)
{
	if (args.size() == 1)
	{
		int index = Rainmeter->FindSkinFolderIndex(args[0]);
		if (index != -1)
		{
			const CRainmeter::SkinFolder& skinFolder = Rainmeter->m_SkinFolders[index];
			if (!(skinFolder.active == 1 && skinFolder.files.size() == 1))
			{
				// Activate the next index.
				Rainmeter->ActivateSkin(index, (skinFolder.active < skinFolder.files.size()) ? skinFolder.active : 0);
			}
			return;
		}
	}
	else if (args.size() > 1)
	{
		std::pair<int, int> indexes = Rainmeter->GetMeterWindowIndex(args[0], args[1]);
		if (indexes.first != -1 && indexes.second != -1)
		{
			Rainmeter->ActivateSkin(indexes.first, indexes.second);
			return;
		}
	}

	LogError(L"!ActivateConfig: Invalid parameters");
}

void CCommandHandler::DoDeactivateSkinBang(std::vector<std::wstring>& args, CMeterWindow* skin)
{
	if (!args.empty())
	{
		skin = Rainmeter->GetMeterWindow(args[0]);
		if (!skin)
		{
			LogWarningF(L"!DeactivateConfig: \"%s\" not active", args[0].c_str());
			return;
		}
	}

	if (skin)
	{
		Rainmeter->DeactivateSkin(skin, -1);
	}
	else
	{
		LogError(L"!DeactivateConfig: Invalid parameters");
	}
}

void CCommandHandler::DoToggleSkinBang(std::vector<std::wstring>& args, CMeterWindow* skin)
{
	if (args.size() >= 2)
	{
		CMeterWindow* skin = Rainmeter->GetMeterWindow(args[0]);
		if (skin)
		{
			Rainmeter->DeactivateSkin(skin, -1);
			return;
		}

		// If the skin wasn't active, activate it.
		DoActivateSkinBang(args, nullptr);
	}
	else
	{
		LogError(L"!ToggleConfig: Invalid parameters");
	}
}

void CCommandHandler::DoDeactivateSkinGroupBang(std::vector<std::wstring>& args, CMeterWindow* skin)
{
	if (!args.empty())
	{
		std::multimap<int, CMeterWindow*> windows;
		Rainmeter->GetMeterWindowsByLoadOrder(windows, args[0]);
		for (const auto& ip : windows)
		{
			Rainmeter->DeactivateSkin(ip.second, -1);
		}
	}
	else
	{
		LogError(L"!DeactivateConfigGroup: Invalid parameters");
	}
}

void CCommandHandler::DoLoadLayoutBang(std::vector<std::wstring>& args, CMeterWindow* skin)
{
	if (args.size() == 1)
	{
		if (skin)
		{
			// Delay to avoid loading theme in the middle of an update.
			std::wstring command = L"!LoadLayout \"";
			command += args[0];
			command += L'"';
			Rainmeter->DelayedExecuteCommand(command.c_str());
		}
		else
		{
			// Not called from a skin (or called with delay).
			Rainmeter->LoadLayout(args[0]);
		}
	}
}

void CCommandHandler::DoSetClipBang(std::vector<std::wstring>& args, CMeterWindow* skin)
{
	if (!args.empty())
	{
		CSystem::SetClipboardText(args[0]);
	}
	else
	{
		LogError(L"!SetClip: Invalid parameter");
	}
}

void CCommandHandler::DoSetWallpaperBang(std::vector<std::wstring>& args, CMeterWindow* skin)
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

		CSystem::SetWallpaper(file, style);
	}
	else
	{
		LogError(L"!SetWallpaper: Invalid parameters");
	}
}

void CCommandHandler::DoAboutBang(std::vector<std::wstring>& args, CMeterWindow* meterWindow)
{
	CDialogAbout::Open(args.empty() ? L"" : args[0].c_str());
}

void CCommandHandler::DoManageBang(std::vector<std::wstring>& args, CMeterWindow* meterWindow)
{
	CDialogManage::Open(args.empty() ? L"" : args[0].c_str());
}

void CCommandHandler::DoSkinMenuBang(std::vector<std::wstring>& args, CMeterWindow* skin)
{
	if (!args.empty())
	{
		skin = Rainmeter->GetMeterWindow(args[0]);
		if (!skin)
		{
			LogWarningF(L"!SkinMenu: \"%s\" not active", args[0].c_str());
			return;
		}
	}

	if (skin)
	{
		POINT pos = CSystem::GetCursorPosition();
		Rainmeter->ShowContextMenu(pos, skin);
	}
	else
	{
		LogError(L"!SkinMenu: Invalid parameter");
	}
}

void CCommandHandler::DoTrayMenuBang(std::vector<std::wstring>& args, CMeterWindow* skin)
{
	POINT pos = CSystem::GetCursorPosition();
	Rainmeter->ShowContextMenu(pos, NULL);
}

void CCommandHandler::DoResetStatsBang(std::vector<std::wstring>& args, CMeterWindow* meterWindow)
{
	Rainmeter->ResetStats();
}

void CCommandHandler::DoWriteKeyValueBang(std::vector<std::wstring>& args, CMeterWindow* skin)
{
	if (args.size() == 3 && skin)
	{
		// Add the skin file path to the args
		args.push_back(skin->GetFilePath());
	}
	else if (args.size() < 4)
	{
		LogError(L"!WriteKeyValue: Invalid parameters");
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
		LogErrorF(L"!WriteKeyValue: Illegal path: %s", iniFile);
		return;
	}

	if (_wcsnicmp(iniFile, Rainmeter->m_SkinPath.c_str(), Rainmeter->m_SkinPath.size()) != 0 &&
		_wcsnicmp(iniFile, Rainmeter->m_SettingsPath.c_str(), Rainmeter->m_SettingsPath.size()) != 0)
	{
		LogErrorF(L"!WriteKeyValue: Illegal path: %s", iniFile);
		return;
	}

	// Verify whether the file exists.
	if (_waccess(iniFile, 0) == -1)
	{
		LogErrorF(L"!WriteKeyValue: File not found: %s", iniFile);
		return;
	}

	// Verify whether the file is read-only.
	DWORD attr = GetFileAttributes(iniFile);
	if (attr == -1 || (attr & FILE_ATTRIBUTE_READONLY))
	{
		LogWarningF(L"!WriteKeyValue: File is read-only: %s", iniFile);
		return;
	}

	// Avoid "IniFileMapping"
	CSystem::UpdateIniFileMappingList();
	std::wstring strIniWrite = CSystem::GetTemporaryFile(strIniFile);
	if (strIniWrite.size() == 1 && strIniWrite[0] == L'?')  // error occurred
	{
		return;
	}

	bool temporary = !strIniWrite.empty();

	if (temporary)
	{
		if (Rainmeter->GetDebug())
		{
			LogDebugF(L"!WriteKeyValue: Writing to: %s (Temp: %s)", iniFile, strIniWrite.c_str());
		}
	}
	else
	{
		if (Rainmeter->GetDebug())
		{
			LogDebugF(L"!WriteKeyValue: Writing to: %s", iniFile);
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
		if (skin->GetParser().ParseFormula(strValue, &value))
		{
			WCHAR buffer[256];
			int len = _snwprintf_s(buffer, _TRUNCATE, L"%.5f", value);
			CMeasure::RemoveTrailingZero(buffer, len);

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
			WritePrivateProfileString(NULL, NULL, NULL, iniWrite);  // FLUSH

			// Copy the file back.
			if (!CSystem::CopyFiles(strIniWrite, strIniFile))
			{
				LogErrorF(L"!WriteKeyValue: Failed to copy temporary file to original filepath: %s (Temp: %s)", iniFile, iniWrite);
			}
		}
		else  // failed
		{
			LogErrorF(L"!WriteKeyValue: Failed to write to: %s (Temp: %s)", iniFile, iniWrite);
		}

		// Remove the temporary file.
		CSystem::RemoveFile(strIniWrite);
	}
	else
	{
		if (write == 0)  // failed
		{
			LogErrorF(L"!WriteKeyValue: Failed to write to: %s", iniFile);
		}
	}
}

void CCommandHandler::DoLogBang(std::vector<std::wstring>& args, CMeterWindow* skin)
{
	if (!args.empty())
	{
		CLogger::Level level = CLogger::Level::Notice;
		if (args.size() > 1)
		{
			const WCHAR* type = args[1].c_str();
			if (_wcsicmp(type, L"ERROR") == 0)
			{
				level = CLogger::Level::Error;
			}
			else if (_wcsicmp(type, L"WARNING") == 0)
			{
				level = CLogger::Level::Warning;
			}
			else if (_wcsicmp(type, L"DEBUG") == 0)
			{
				level = CLogger::Level::Debug;
			}
			else if (_wcsicmp(type, L"NOTICE") != 0)
			{
				LogError(L"!Log: Invalid type");
				return;
			}
		}

		CLogger::GetInstance().Log(level, args[0].c_str());
	}
}

void CCommandHandler::DoRefreshApp(std::vector<std::wstring>& args, CMeterWindow* meterWindow)
{
	// Refresh needs to be delayed since it crashes if done during Update().
	PostMessage(Rainmeter->m_Window, WM_RAINMETER_DELAYED_REFRESH_ALL, NULL, NULL);
}

void CCommandHandler::DoQuitBang(std::vector<std::wstring>& args, CMeterWindow* meterWindow)
{
	// Quit needs to be delayed since it crashes if done during Update().
	PostMessage(Rainmeter->GetTrayWindow()->GetWindow(), WM_COMMAND, MAKEWPARAM(IDM_QUIT, 0), NULL);
}

void CCommandHandler::DoLsBoxHookBang(std::vector<std::wstring>& args, CMeterWindow* meterWindow)
{
	// Deprecated.
}
