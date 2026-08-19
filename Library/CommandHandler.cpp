// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "../Common/Map.h"
#include "../Common/PathUtil.h"
#include "../Common/StringUtil.h"
#include "CommandHandler.h"
#include "ConfigParser.h"
#include "DialogAbout.h"
#include "DialogDebug.h"
#include "DialogManage.h"
#include "Measure.h"
#include "Meter.h"
#include "Logger.h"
#include "Rainmeter.h"
#include "System.h"
#include "TrayIcon.h"
#include "resource.h"

namespace {

struct BangInfo
{
	Bang bang;
	const WCHAR* name;
	uint8_t argCount;
	void (* handlerFunc)(const BangInfo& bangInfo, std::vector<std::wstring>& args, Skin* skin);
};

struct SectionBangInfo
{
	enum class Type : BYTE
	{
		Meter,
		Measure,

		// Aimed at the skin rather than at one of its sections, so |typeId| means nothing.
		Skin
	};

	const WCHAR* name;
	uint8_t argCount;

	Type type;
	UINT typeId;

	union
	{
		MeterBangFunc meterFunc;
		MeasureBangFunc measureFunc;
		SkinBangFunc skinFunc;
	};
};

std::wstring BuildConfigPath(const std::wstring& folderPath, const std::wstring& file)
{
	return std::wstring(folderPath + (file.empty() ? L"" : L"\\") + file);
}

bool DoesConfigExist(const std::wstring& folderPath, const std::wstring& file = std::wstring())
{
	if (!folderPath.empty())
	{
		std::wstring path = GetRainmeter().GetSkinPath() + BuildConfigPath(folderPath, file);
		return (_waccess_s(path.c_str(), 0) == 0);
	}
	return false;
}

void DoBang(const BangInfo& bangInfo, std::vector<std::wstring>& args, Skin* skin)
{
	// !SetOption may leave out the section name in favor of the section running the action.
	if (bangInfo.bang == Bang::SetOption && skin && args.size() + 1 == bangInfo.argCount)
	{
		Section* section = skin->GetCurrentActionSection();
		if (!section)
		{
			LogWarningF(skin, L"!%s: Section name required", bangInfo.name);
			return;
		}

		args.insert(args.begin(), section->GetOriginalName());
	}

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
					Skin* other = GetRainmeter().GetSkin(folderPath);
					if (other)
					{
						other->DoBang(bangInfo.bang, args);
					}
					else if (DoesConfigExist(folderPath))
					{
						LogWarningF(skin, L"!%s: Skin \"%s\" is not active", bangInfo.name, folderPath.c_str());
					}
					else
					{
						LogErrorF(skin, L"!%s: Skin \"%s\" does not exist", bangInfo.name, folderPath.c_str());
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

void DoSectionBang(const SectionBangInfo& bangInfo, std::vector<std::wstring>& args, Skin* skin)
{
	if (!skin)
	{
		LogErrorF(L"!%s: Not run from a skin", bangInfo.name);
		return;
	}

	if (bangInfo.type == SectionBangInfo::Type::Skin)
	{
		if (args.size() != bangInfo.argCount)
		{
			LogErrorF(skin, L"!%s: Incorrect number of arguments", bangInfo.name);
			return;
		}

		bangInfo.skinFunc(args, skin);
		return;
	}

	Section* section = nullptr;
	if (args.size() == (size_t)bangInfo.argCount + 1U)
	{
		const std::wstring& name = args[0];
		section = skin->GetSection(name);
		if (!section)
		{
			const bool isMeter = bangInfo.type == SectionBangInfo::Type::Meter;
			LogErrorF(skin, L"!%s: %s [%s] not found", bangInfo.name, isMeter ? L"Meter" : L"Measure", name.c_str());
			return;
		}

		args.erase(args.begin());
	}
	else if (args.size() == bangInfo.argCount)
	{
		section = skin->GetCurrentActionSection();
		if (!section)
		{
			LogWarningF(skin, L"!%s: Section name required", bangInfo.name);
			return;
		}
	}
	else
	{
		LogErrorF(skin, L"!%s: Incorrect number of arguments", bangInfo.name);
		return;
	}

	if (section->GetTypeID() != bangInfo.typeId)
	{
		LogErrorF(skin, L"!%s: Invalid bang for [%s]", bangInfo.name, section->GetName());
		return;
	}

	if (bangInfo.type == SectionBangInfo::Type::Meter)
	{
		bangInfo.meterFunc((Meter*)section, args, skin);
	}
	else
	{
		bangInfo.measureFunc((Measure*)section, args, skin);
	}
}

void Internal_DoActivateBang(std::vector<std::wstring>& args, Skin* skin, LPCWSTR bangName)
{
	// References: DoActivateSkinBang, DoToggleSkinBang
	std::wstring folderPath;
	std::wstring file;
	const size_t argCount = args.size();
	if (argCount > 0)
	{
		folderPath = args[0];
		if (argCount == 1)
		{
			if (GetRainmeter().ActivateSkin(folderPath)) return;
		}
		else
		{
			file = args[1];
			if (GetRainmeter().ActivateSkin(folderPath, file)) return;
		}
	}

	if (!folderPath.empty())
	{
		std::wstring path = BuildConfigPath(folderPath, file);
		if (DoesConfigExist(folderPath, file))
		{
			LogNoticeF(skin, L"!%s: \"%s\" exists, but is not available. Please refresh Rainmeter.", bangName, path.c_str());
		}
		else
		{
			LogErrorF(skin, L"!%s: \"%s\" does not exist", bangName, path.c_str());
		}
		return;
	}

	LogErrorF(skin, L"!%s: Invalid parameters", bangName);
}

}  // namespace

void CommandHandler::ExecuteCommand(const WCHAR* command, Skin* skin, bool multi)
{
	// Remove any leading whitespace
	while (iswspace(command[0])) ++command;

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

			std::wstring_view bang;
			std::vector<std::wstring> args;

			// Find the first space
			const WCHAR* pos = wcschr(command, L' ');
			if (pos)
			{
				bang = std::wstring_view(command, pos - command);
				args = ParseString(pos + 1, skin ? &skin->GetParser() : nullptr);
			}
			else
			{
				bang = command;
			}

			ExecuteBang(bang, args, skin);
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
					// !Bang found

					// Pre-parse any "bang replacement variables"
					if (skin)
					{
						std::wstring currentBang = bangs.substr(start, i - start + 1);
						if (ConfigParser::IsSectionVariableKey(currentBang[1]) &&
							skin->GetParser().ReplaceMeasures(currentBang))
						{
							// Surround the replacement bang with brackets (if needed) since
							// there could be more trailing bangs from the original
							if (currentBang[0] != L'[')
							{
								currentBang.insert(0, L"[");
								currentBang.append(L"]");
							}

							currentBang.append(bangs.substr(i + 1));  // Append trailing bangs
							ExecuteCommand(currentBang.c_str(), skin, true);
							return;
						}
					}

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
							auto delay = skin->GetParser().ParseUInt(args[0].c_str(), 0);
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
			// If the command is a section variable or a new style variable,
			// surround the command with brackets and replace it with the variable.
			// This allows for section variables to completely replace a bang sequence.
			// ex. LeftMouseUpAction=[SomeMeasureName]  or  LeftMouseUpAction=[#NewStyleVar]
			// Note: This assumes the |command| does not start with a variable key (&, #, $, \)
			bool isVar = (ConfigParser::IsSectionVariableKey(tmpSz[0]) || skin->GetMeasure(tmpSz));
			if (isVar)
			{
				tmpSz.insert(0, L"[");
				tmpSz.append(L"]");
			}

			if (skin->GetParser().ReplaceMeasures(tmpSz) && isVar)
			{
				ExecuteCommand(tmpSz.c_str(), skin, true);
				return;
			}
		}

		RunCommand(tmpSz);
	}
}

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

namespace {

void DoActivateSkinBang(const BangInfo& bangInfo, std::vector<std::wstring>& args, Skin* skin)
{
	Internal_DoActivateBang(args, skin, bangInfo.name);
}

void DoDeactivateSkinBang(const BangInfo& bangInfo, std::vector<std::wstring>& args, Skin* skin)
{
	if (!args.empty())
	{
		skin = GetRainmeter().GetSkin(args[0]);
		if (!skin)
		{
			if (!DoesConfigExist(args[0]))
			{
				LogErrorF(L"!%s: \"%s\" does not exist", bangInfo.name, args[0].c_str());
			}
			return;
		}
	}

	if (skin)
	{
		GetRainmeter().DeactivateSkin(skin, -1);
	}
	else
	{
		LogErrorF(L"!%s: Invalid parameters", bangInfo.name);
	}
}

void DoToggleSkinBang(const BangInfo& bangInfo, std::vector<std::wstring>& args, Skin* skin)
{
	if (args.size() >= 1)
	{
		Skin* other = GetRainmeter().GetSkin(args[0]);
		if (other)
		{
			GetRainmeter().DeactivateSkin(other, -1);
			return;
		}

		// If the skin wasn't active, activate it.
		Internal_DoActivateBang(args, other, bangInfo.name);
	}
	else
	{
		LogErrorF(skin, L"!%s: Invalid parameters", bangInfo.name);
	}
}

void DoDeactivateSkinGroupBang(const BangInfo& bangInfo, std::vector<std::wstring>& args, Skin* skin)
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

void DoLoadLayoutBang(const BangInfo& bangInfo, std::vector<std::wstring>& args, Skin* skin)
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

void DoSetClipBang(const BangInfo& bangInfo, std::vector<std::wstring>& args, Skin* skin)
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

void DoSetWallpaperBang(const BangInfo& bangInfo, std::vector<std::wstring>& args, Skin* skin)
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

void DoAboutBang(const BangInfo& bangInfo, std::vector<std::wstring>& args, Skin* skin)
{
	if (!args.empty())
	{
		if (_wcsicmp(args[0].c_str(), L"Version") == 0)
		{
			DialogAbout::Open();
		}
		else
		{
			DialogDebug::Open(args[0].c_str());
		}
	}
	else
	{
		DialogDebug::Open();
	}
}

void DoDebugBang(const BangInfo& bangInfo, std::vector<std::wstring>& args, Skin* skin)
{
	if (args.empty())
	{
		DialogDebug::Open();
	}
	else if (args.size() == 1)
	{
		DialogDebug::Open(args[0].c_str());
	}
	else if (args.size() == 2 && _wcsicmp(args[0].c_str(), L"Skin") == 0)
	{
		if (auto* selectedSkin = GetRainmeter().GetSkin(args[1]))
		{
			DialogDebug::OpenSkin(selectedSkin);
		}
		else
		{
			LogErrorF(skin, L"!Debug: Config not active: %s", args[1].c_str());
		}
	}
	else
	{
		LogErrorF(skin, L"!Debug: Invalid parameters");
	}
}

void DoManageBang(const BangInfo& bangInfo, std::vector<std::wstring>& args, Skin* skin)
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

void DoSkinMenuBang(const BangInfo& bangInfo, std::vector<std::wstring>& args, Skin* skin)
{
	if (!args.empty())
	{
		skin = GetRainmeter().GetSkin(args[0]);
		if (!skin)
		{
			if (DoesConfigExist(args[0]))
			{
				LogWarningF(L"!SkinMenu: \"%s\" is not active", args[0].c_str());
			}
			else
			{
				LogErrorF(L"!SkinMenu: \"%s\" does not exist", args[0].c_str());
			}
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
		LogErrorF(L"!SkinMenu: Invalid parameter");
	}
}

void DoTrayMenuBang(const BangInfo& bangInfo, std::vector<std::wstring>& args, Skin* skin)
{
	POINT pos = System::GetCursorPosition();
	GetRainmeter().ShowContextMenu(pos, nullptr);
}

void DoResetStatsBang(const BangInfo& bangInfo, std::vector<std::wstring>& args, Skin* skin)
{
	GetRainmeter().ResetStats();
}

void DoWriteKeyValueBang(const BangInfo& bangInfo, std::vector<std::wstring>& args, Skin* skin)
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

	const std::wstring& skinPath = GetRainmeter().GetSkinPath();
	const std::wstring& settingsPath = GetRainmeter().GetSettingsPath();
	if (_wcsnicmp(iniFile, skinPath.c_str(), skinPath.size()) != 0 &&
		_wcsnicmp(iniFile, settingsPath.c_str(), settingsPath.size()) != 0)
	{
		LogErrorF(skin, L"!WriteKeyValue: Illegal path: %s", iniFile);
		return;
	}

	// Verify whether the file exists.
	if (_waccess_s(iniFile, 0) != 0)
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
	BOOL write = FALSE;

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
		if (write != FALSE)
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
		if (write == FALSE)  // failed
		{
			LogErrorF(skin, L"!WriteKeyValue: Failed to write to: %s", iniFile);
		}
	}
}

void DoLogBang(const BangInfo& bangInfo, std::vector<std::wstring>& args, Skin* skin)
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

void DoRefreshApp(const BangInfo& bangInfo, std::vector<std::wstring>& args, Skin* skin)
{
	// Refresh needs to be delayed since it crashes if done during Update().
	PostMessage(GetRainmeter().GetWindow(), WM_RAINMETER_DELAYED_REFRESH_ALL, 0, 0);
}

void DoQuitBang(const BangInfo& bangInfo, std::vector<std::wstring>& args, Skin* skin)
{
	// Quit needs to be delayed since it crashes if done during Update().
	PostMessage(GetRainmeter().GetTrayIcon()->GetWindow(), WM_COMMAND, MAKEWPARAM(IDM_QUIT, 0), 0);
}

void DoEditSkinBang(const BangInfo& bangInfo, std::vector<std::wstring>& args, Skin* skin)
{
	const size_t argSize = args.size();
	if (argSize > 1)
	{
		const SkinRegistry::Indexes indexes = GetRainmeter().GetSkinRegistry().FindIndexes(args[0], args[1]);
		if (indexes.IsValid())
		{
			GetRainmeter().EditSkinFile(args[0], args[1]);
		}
		else
		{
			LogErrorF(skin, L"!%s: Invalid parameters", bangInfo.name);
		}
	}
	else if (argSize == 1)
	{
		std::wstring& config = args[0];
		Skin* other = GetRainmeter().GetSkin(config);
		if (other)
		{
			GetRainmeter().EditSkinFile(other->GetFolderPath(), other->GetFileName());
		}
		else if (DoesConfigExist(config))
		{
			LogWarningF(skin, L"!%s: \"%s\" is not active", bangInfo.name, config.c_str());
		}
		else
		{
			LogErrorF(skin, L"!%s: \"%s\" does not exist", bangInfo.name, config.c_str());
		}
	}
	else if (argSize == 0 && skin)
	{
		GetRainmeter().EditSkinFile(skin->GetFolderPath(), skin->GetFileName());
	}
	else
	{
		LogErrorF(skin, L"!%s: Invalid parameters", bangInfo.name);
	}
}

void DoSetWindowPositionBang(const BangInfo& bangInfo, std::vector<std::wstring>& args, Skin* skin)
{
	// Two variations:
	//  #1: !SetWindowPosition WindowX WindowY (config)
	//  #2: !SetWindowPosition WindowX WindowY AnchorX AnchorY (config)

	Skin* other = nullptr;

	size_t argCount = args.size();
	if (argCount == 5 || argCount == 3)
	{
		std::wstring& folderPath = args[argCount - 1];
		other = GetRainmeter().GetSkin(folderPath);
		if (!other)
		{
			LogErrorF(skin, L"!SetWindowPosition: Skin \"%s\" not found", folderPath.c_str());
			return;
		}

		args.pop_back();  // Remove "config"
		--argCount;
	}

	if (argCount == 4 || argCount == 2)
	{
		if (other)
		{
			other->DoBang(Bang::SetWindowPosition, args);
			return;
		}
		else if (skin)
		{
			skin->DoBang(Bang::SetWindowPosition, args);
			return;
		}
	}

	LogErrorF(skin, L"!SetWindowPosition: Invalid parameters");
}

void DoLsBoxHookBang(const BangInfo& bangInfo, std::vector<std::wstring>& args, Skin* skin)
{
	// Deprecated.
}

const BangInfo g_Bangs[] =
{
	{ Bang::Refresh, L"Refresh", 0, DoBang },
	{ Bang::Redraw, L"Redraw", 0, DoBang },
	{ Bang::Update, L"Update", 0, DoBang },
	{ Bang::SetUpdate, L"SetUpdate", 1, DoBang },
	{ Bang::Hide, L"Hide", 0, DoBang },
	{ Bang::Show, L"Show", 0, DoBang },
	{ Bang::Toggle, L"Toggle", 0, DoBang },
	{ Bang::HideFade, L"HideFade", 0, DoBang },
	{ Bang::ShowFade, L"ShowFade", 0, DoBang },
	{ Bang::ToggleFade, L"ToggleFade", 0, DoBang },
	{ Bang::FadeDuration, L"FadeDuration", 1, DoBang },
	{ Bang::HideMeter, L"HideMeter", 1, DoBang },
	{ Bang::ShowMeter, L"ShowMeter", 1, DoBang },
	{ Bang::ToggleMeter, L"ToggleMeter", 1, DoBang },
	{ Bang::MoveMeter, L"MoveMeter", 3, DoBang },
	{ Bang::UpdateMeter, L"UpdateMeter", 1, DoBang },
	{ Bang::DisableMouseAction, L"DisableMouseAction", 2, DoBang },
	{ Bang::ClearMouseAction, L"ClearMouseAction", 2, DoBang },
	{ Bang::EnableMouseAction, L"EnableMouseAction", 2, DoBang },
	{ Bang::ToggleMouseAction, L"ToggleMouseAction", 2, DoBang },
	{ Bang::DisableMeasure, L"DisableMeasure", 1, DoBang },
	{ Bang::EnableMeasure, L"EnableMeasure", 1, DoBang },
	{ Bang::ToggleMeasure, L"ToggleMeasure", 1, DoBang },
	{ Bang::PauseMeasure, L"PauseMeasure", 1, DoBang },
	{ Bang::UnpauseMeasure, L"UnpauseMeasure", 1, DoBang },
	{ Bang::TogglePauseMeasure, L"TogglePauseMeasure", 1, DoBang },
	{ Bang::UpdateMeasure, L"UpdateMeasure", 1, DoBang },
	{ Bang::CommandMeasure, L"CommandMeasure", 2, DoBang },
	{ Bang::PluginBang, L"PluginBang", 1, DoBang },
	{ Bang::ShowBlur, L"ShowBlur", 0, DoBang },
	{ Bang::HideBlur, L"HideBlur", 0, DoBang },
	{ Bang::ToggleBlur, L"ToggleBlur", 0, DoBang },
	{ Bang::AddBlur, L"AddBlur", 1, DoBang },
	{ Bang::RemoveBlur, L"RemoveBlur", 1, DoBang },
	{ Bang::Move, L"Move", 2, DoBang },
	{ Bang::SetAnchor, L"SetAnchor", 2, DoBang },
	{ Bang::SetZoomFactor, L"SetZoomFactor", 1, DoBang },
	{ Bang::ZPos, L"ZPos", 1, DoBang },
	{ Bang::ZPos, L"ChangeZPos", 1, DoBang },  // For backwards compatibility.
	{ Bang::ClickThrough, L"ClickThrough", 1, DoBang },
	{ Bang::Draggable, L"Draggable", 1, DoBang },
	{ Bang::SnapEdges, L"SnapEdges", 1, DoBang },
	{ Bang::KeepOnScreen, L"KeepOnScreen", 1, DoBang },
	{ Bang::AutoSelectScreen, L"AutoSelectScreen", 1, DoBang },
	{ Bang::SetTransparency, L"SetTransparency", 1, DoBang },
	{ Bang::SetVariable, L"SetVariable", 2, DoBang },
	{ Bang::SetOption, L"SetOption", 3, DoBang },
	{ Bang::SetOptionGroup, L"SetOptionGroup", 3, DoBang },
	{ Bang::HideMeterGroup, L"HideMeterGroup", 1, DoBang },
	{ Bang::ShowMeterGroup, L"ShowMeterGroup", 1, DoBang },
	{ Bang::ToggleMeterGroup, L"ToggleMeterGroup", 1, DoBang },
	{ Bang::UpdateMeterGroup, L"UpdateMeterGroup", 1, DoBang },
	{ Bang::DisableMouseActionGroup, L"DisableMouseActionGroup", 2, DoBang },
	{ Bang::ClearMouseActionGroup, L"ClearMouseActionGroup", 2, DoBang },
	{ Bang::EnableMouseActionGroup, L"EnableMouseActionGroup", 2, DoBang },
	{ Bang::ToggleMouseActionGroup, L"ToggleMouseActionGroup", 2, DoBang },
	{ Bang::DisableMeasureGroup, L"DisableMeasureGroup", 1, DoBang },
	{ Bang::EnableMeasureGroup, L"EnableMeasureGroup", 1, DoBang },
	{ Bang::ToggleMeasureGroup, L"ToggleMeasureGroup", 1, DoBang },
	{ Bang::PauseMeasureGroup, L"PauseMeasureGroup", 1, DoBang },
	{ Bang::UnpauseMeasureGroup, L"UnpauseMeasureGroup", 1, DoBang },
	{ Bang::TogglePauseMeasureGroup, L"TogglePauseMeasureGroup", 1, DoBang },
	{ Bang::UpdateMeasureGroup, L"UpdateMeasureGroup", 1, DoBang },
	{ Bang::CommandMeasureGroup, L"CommandMeasureGroup", 2, DoBang },
	{ Bang::SkinCustomMenu, L"SkinCustomMenu", 0, DoBang },

	// Group bangs.
	{ Bang::Refresh, L"RefreshGroup", 0, DoGroupBang },
	{ Bang::Update, L"UpdateGroup", 0, DoGroupBang },
	{ Bang::Redraw, L"RedrawGroup", 0, DoGroupBang },
	{ Bang::Hide, L"HideGroup", 0, DoGroupBang },
	{ Bang::Show, L"ShowGroup", 0, DoGroupBang },
	{ Bang::Toggle, L"ToggleGroup", 0, DoGroupBang },
	{ Bang::HideFade, L"HideFadeGroup", 0, DoGroupBang },
	{ Bang::ShowFade, L"ShowFadeGroup", 0, DoGroupBang },
	{ Bang::ToggleFade, L"ToggleFadeGroup", 0, DoGroupBang },
	{ Bang::ZPos, L"ZPosGroup", 1, DoGroupBang },
	{ Bang::ClickThrough, L"ClickThroughGroup", 1, DoGroupBang },
	{ Bang::Draggable, L"DraggableGroup", 1, DoGroupBang },
	{ Bang::SnapEdges, L"SnapEdgesGroup", 1, DoGroupBang },
	{ Bang::FadeDuration, L"FadeDurationGroup", 1, DoGroupBang },
	{ Bang::KeepOnScreen, L"KeepOnScreenGroup", 1, DoGroupBang },
	{ Bang::AutoSelectScreen, L"AutoSelectScreenGroup", 1, DoGroupBang },
	{ Bang::SetZoomFactor, L"SetZoomFactorGroup", 1, DoGroupBang },
	{ Bang::SetTransparency, L"SetTransparencyGroup", 1, DoGroupBang },
	{ Bang::SetVariable, L"SetVariableGroup", 2, DoGroupBang },
	{ Bang::DisableMouseActionSkinGroup, L"DisableMouseActionSkinGroup", 1, DoGroupBang },
	{ Bang::ClearMouseActionSkinGroup, L"ClearMouseActionSkinGroup", 1, DoGroupBang },
	{ Bang::EnableMouseActionSkinGroup, L"EnableMouseActionSkinGroup", 1, DoGroupBang },
	{ Bang::ToggleMouseActionSkinGroup, L"ToggleMouseActionSkinGroup", 1, DoGroupBang },

	// Bangs handled using a custom handler function with its own argument checking.
	{ Bang::ActivateConfig, L"ActivateConfig", 0, DoActivateSkinBang },
	{ Bang::DeactivateConfig, L"DeactivateConfig", 0, DoDeactivateSkinBang },
	{ Bang::ToggleConfig, L"ToggleConfig", 0, DoToggleSkinBang },
	{ Bang::DeactivateConfigGroup, L"DeactivateConfigGroup", 0, DoDeactivateSkinGroupBang },
	{ Bang::WriteKeyValue, L"WriteKeyValue", 0, DoWriteKeyValueBang },
	{ Bang::LoadLayout, L"LoadLayout", 0, DoLoadLayoutBang },
	{ Bang::SetClip, L"SetClip", 0, DoSetClipBang },
	{ Bang::SetWallpaper, L"SetWallpaper", 0, DoSetWallpaperBang },
	{ Bang::About, L"About", 0, DoAboutBang },
	{ Bang::Debug, L"Debug", 0, DoDebugBang },
	{ Bang::Manage, L"Manage", 0, DoManageBang },
	{ Bang::SkinMenu, L"SkinMenu", 0, DoSkinMenuBang },
	{ Bang::TrayMenu, L"TrayMenu", 0, DoTrayMenuBang },
	{ Bang::ResetStats, L"ResetStats", 0, DoResetStatsBang },
	{ Bang::Log, L"Log", 0, DoLogBang },
	{ Bang::RefreshApp, L"RefreshApp", 0, DoRefreshApp },
	{ Bang::Quit, L"Quit", 0, DoQuitBang },
	{ Bang::EditSkin, L"EditSkin", 0, DoEditSkinBang },
	{ Bang::LsBoxHook, L"LsBoxHook", 0, DoLsBoxHookBang },
	{ Bang::SetWindowPosition, L"SetWindowPosition", 0, DoSetWindowPositionBang },

	// The same bangs in the "Skin:" namespace.
	{ Bang::ActivateConfig, L"Skin:Load", 0, DoActivateSkinBang },
	{ Bang::DeactivateConfig, L"Skin:Unload", 0, DoDeactivateSkinBang },
	{ Bang::ToggleConfig, L"Skin:Toggle", 0, DoToggleSkinBang },
	{ Bang::EditSkin, L"Skin:Edit", 0, DoEditSkinBang }
};

std::wstring_view BuildBangMapKey(std::wstring_view name, WCHAR* buffer, size_t bufferCount)
{
	if (!StringUtil::ToUpperCase(name, buffer, bufferCount)) return {};
	return std::wstring_view(buffer, name.length());
}

// Maps uppercased bang names to their entry in |g_Bangs|.
const StringMap<const BangInfo*>& GetBangMap()
{
	static const StringMap<const BangInfo*> s_Map = []()
	{
		StringMap<const BangInfo*> map;
		map.rehash(_countof(g_Bangs));

		for (const auto& bangInfo : g_Bangs)
		{
			WCHAR buffer[64];
			const auto key = BuildBangMapKey(bangInfo.name, buffer, _countof(buffer));
			map.emplace(std::wstring(key), &bangInfo);
		}

		return map;
	} ();

	return s_Map;
}

StringMap<SectionBangInfo>& GetSectionBangMap()
{
	static StringMap<SectionBangInfo> s_Map;
	return s_Map;
}

void RegisterSectionBang(const SectionBangInfo& bangInfo)
{
	WCHAR buffer[64];
	const auto key = BuildBangMapKey(bangInfo.name, buffer, _countof(buffer));
	if (key.empty())
	{
		LogErrorF(L"Invalid bang name: !%s", bangInfo.name);
		return;
	}

	// A built-in bang of the same name would be found first, leaving this one unreachable.
	if (GetBangMap().contains(key))
	{
		LogErrorF(L"Bang already exists: !%s", bangInfo.name);
		return;
	}

	GetSectionBangMap().emplace(std::wstring(key), bangInfo);
}

std::optional<std::wstring_view> GetTargetGroup(std::wstring_view target)
{
	constexpr std::wstring_view selector = L"Group=";
	if (!target.starts_with(selector)) return std::nullopt;

	return target.substr(selector.length());
}

void DoSkinBang(std::wstring_view name, std::wstring_view bang, std::vector<std::wstring>& args, Skin* skin)
{
	if (bang.empty())
	{
		LogErrorF(skin, L"!%.*s: Bang name required", (int)name.length(), name.data());
		return;
	}

	if (args.empty())
	{
		LogErrorF(skin, L"!%.*s: Incorrect number of arguments", (int)name.length(), name.data());
		return;
	}

	const std::wstring target = std::move(args[0]);
	args.erase(args.begin());

	if (const auto group = GetTargetGroup(target))
	{
		std::multimap<int, Skin*> skins;
		GetRainmeter().GetSkinsByLoadOrder(skins, *group);

		for (const auto& ip : skins)
		{
			// The handlers may modify the arguments, so hand each skin its own copy.
			std::vector<std::wstring> skinArgs = args;
			GetRainmeter().ExecuteBang(bang, skinArgs, ip.second);
		}

		return;
	}

	Skin* other = GetRainmeter().GetSkin(target);
	if (!other)
	{
		if (DoesConfigExist(target))
		{
			LogWarningF(skin, L"!%.*s: Skin \"%s\" is not active",
				(int)name.length(), name.data(), target.c_str());
		}
		else
		{
			LogErrorF(skin, L"!%.*s: Skin \"%s\" does not exist",
				(int)name.length(), name.data(), target.c_str());
		}
		return;
	}

	GetRainmeter().ExecuteBang(bang, args, other);
}

}  // namespace

void CommandHandler::ExecuteBang(std::wstring_view name, std::vector<std::wstring>& args, Skin* skin)
{
	WCHAR buffer[64];
	const auto key = BuildBangMapKey(name, buffer, _countof(buffer));

	const auto& bangMap = GetBangMap();
	const auto iter = bangMap.find(key);
	if (iter != bangMap.end())
	{
		const BangInfo& bangInfo = *iter->second;
		bangInfo.handlerFunc(bangInfo, args, skin);
		return;
	}

	const auto& sectionBangMap = GetSectionBangMap();
	const auto sectionIter = sectionBangMap.find(key);
	if (sectionIter != sectionBangMap.end())
	{
		DoSectionBang(sectionIter->second, args, skin);
		return;
	}

	// A "Skin:" prefix aims the rest of the bang at the skin(s) named by the first argument.
	// Bangs registered in the namespace are found above, so they take precedence.
	constexpr std::wstring_view skinPrefix = L"Skin:";
	if (name.starts_with(skinPrefix))
	{
		DoSkinBang(name, name.substr(skinPrefix.length()), args, skin);
		return;
	}

	LogErrorF(skin, L"Invalid bang: !%.*s", (int)name.length(), name.data());
}

void CommandHandler::RegisterMeterBang(UINT typeId, const WCHAR* name, uint8_t argCount, MeterBangFunc handlerFunc)
{
	RegisterSectionBang({
		.name = name,
		.argCount = argCount,
		.type = SectionBangInfo::Type::Meter,
		.typeId = typeId,
		.meterFunc = handlerFunc
	});
}

void CommandHandler::RegisterMeasureBang(UINT typeId, const WCHAR* name, uint8_t argCount, MeasureBangFunc handlerFunc)
{
	RegisterSectionBang({
		.name = name,
		.argCount = argCount,
		.type = SectionBangInfo::Type::Measure,
		.typeId = typeId,
		.measureFunc = handlerFunc
	});
}

void CommandHandler::RegisterSkinBang(const WCHAR* name, uint8_t argCount, SkinBangFunc handlerFunc)
{
	RegisterSectionBang({
		.name = name,
		.argCount = argCount,
		.type = SectionBangInfo::Type::Skin,
		.skinFunc = handlerFunc
	});
}
