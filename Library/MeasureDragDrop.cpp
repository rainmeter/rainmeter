/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "MeasureDragDrop.h"
#include "ConfigParser.h"
#include "Logger.h"
#include "Meter.h"
#include "Rainmeter.h"
#include "Skin.h"
#include "SkinDropTarget.h"
#include "../Common/PathUtil.h"
#include "../Common/ParseUtil.h"

namespace {

bool EnsureDirectoryExists(const std::wstring& path)
{
	const int result = SHCreateDirectoryEx(nullptr, path.c_str(), nullptr);
	return result == ERROR_SUCCESS || result == ERROR_FILE_EXISTS || result == ERROR_ALREADY_EXISTS;
}

std::wstring GetFileStem(const std::wstring& fileName)
{
	const auto dotPos = fileName.find_last_of(L'.');
	return dotPos == std::wstring::npos ? fileName : fileName.substr(0, dotPos);
}

std::wstring ReplaceToken(std::wstring value, const WCHAR* token, const std::wstring& replacement)
{
	size_t pos = 0;
	const size_t tokenLength = wcslen(token);
	while ((pos = value.find(token, pos)) != std::wstring::npos)
	{
		value.replace(pos, tokenLength, replacement);
		pos += replacement.length();
	}
	return value;
}

}  // namespace

MeasureDragDrop::MeasureDragDrop(Skin* skin, const WCHAR* name) : Measure(skin, name),
	m_Action(DropAction::None),
	m_Path(),
	m_OnDropAction(),
	m_OnEnterAction(),
	m_OnOverAction(),
	m_OnLeaveAction(),
	m_DropTarget(),
	m_BoundsMeter(),
	m_BoundsFormulas(),
	m_StringValue(),
	m_DropActive(false),
	m_ProcessAllFiles(false),
	m_OverrideExisting(false),
	m_Silent(false),
	m_UsingFixedBounds(false)
{
}

MeasureDragDrop::~MeasureDragDrop()
{
}

void MeasureDragDrop::ReadOptions(ConfigParser& parser, const WCHAR* section)
{
	Measure::ReadOptions(parser, section);

	m_Path = parser.ReadString(section, L"Path", L"");
	m_Skin->MakePathAbsolute(m_Path);

	m_OnDropAction = parser.ReadString(section, L"OnDropAction", L"");
	if (m_OnDropAction.empty())
	{
		// For backwards compatibility.
		m_OnDropAction = parser.ReadString(section, L"OnDroppedAction", L"");
	}

	m_OnEnterAction = parser.ReadString(section, L"OnEnterAction", L"");
	m_OnOverAction = parser.ReadString(section, L"OnOverAction", L"");
	m_OnLeaveAction = parser.ReadString(section, L"OnLeaveAction", L"");

	m_ProcessAllFiles = parser.ReadBool(section, L"ProcessAllFiles", false);
	m_OverrideExisting = parser.ReadBool(section, L"OverrideExisting", false);
	m_Silent = parser.ReadBool(section, L"Silent", false);

	m_Action = DropAction::None;
	const auto* action = parser.ReadString(section, L"Action", L"").c_str();
	if (_wcsicmp(action, L"Move") == 0)
	{
		m_Action = DropAction::Move;
	}
	else if (_wcsicmp(action, L"Copy") == 0)
	{
		m_Action = DropAction::Copy;
	}
	else if (_wcsicmp(action, L"Delete") == 0)
	{
		m_Action = DropAction::Delete;
	}
	else if (_wcsicmp(action, L"Shortcut") == 0)
	{
		m_Action = DropAction::Shortcut;
	}
	else if (_wcsicmp(action, L"Path") == 0)
	{
		m_Action = DropAction::Path;
	}
	else if (!parser.GetLastDefaultUsed() && _wcsicmp(action, L"None") != 0)
	{
		LogErrorF(this, L"Invalid Action=%s", action);
		m_Disabled = true;
	}

	if ((m_Action == DropAction::Move || m_Action == DropAction::Copy || m_Action == DropAction::Shortcut) && m_Path.empty())
	{
		LogWarningF(this, L"No Path specified, disabling measure");
		m_Disabled = true;
	}

	m_UsingFixedBounds = false;
	m_BoundsMeter.clear();
	m_BoundsFormulas.fill(L"");

	const std::wstring& bounds = parser.ReadString(section, L"Bounds", L"", false);
	if (!bounds.empty())
	{
		const auto tokens = ConfigParser::TokenizeWithPairedPunctuation(bounds, L',', PairedPunctuation::Parentheses);
		if (tokens.size() == 1)
		{
			m_BoundsMeter = tokens[0];
		}
		else if (tokens.size() == 4)
		{
			m_UsingFixedBounds = true;
			for (size_t i = 0; i < tokens.size(); ++i)
			{
				m_BoundsFormulas[i] = parser.ParseFormulaWithModifiers(tokens[i]);
			}
		}
		else
		{
			LogErrorF(this, L"Bounds must contain either a meter name or X,Y,W,H");
			m_Disabled = true;
		}
	}

	m_DropTarget = m_Disabled ? nullptr : GetSkin()->GetDropTarget();
}

void MeasureDragDrop::UpdateValue()
{
	m_Value = 0.0;
}

const WCHAR* MeasureDragDrop::GetStringValue()
{
	return CheckSubstitute(m_StringValue.c_str());
}

bool MeasureDragDrop::ContainsPoint(const POINTL& screenPoint) const
{
	if (m_Disabled) return false;

	RECT bounds = { 0 };
	if (!ResolveBounds(bounds))
	{
		return false;
	}

	POINT point = { screenPoint.x, screenPoint.y };
	point = m_Skin->PhysicalToRelativeLogical(point);
	return point.x >= bounds.left && point.x <= (bounds.left + bounds.right) &&
		point.y >= bounds.top && point.y <= (bounds.top + bounds.bottom);
}

void MeasureDragDrop::HandleDragEnter(const std::vector<std::wstring>& files, const POINTL& screenPoint)
{
	if (ContainsPoint(screenPoint) && !m_DropActive)
	{
		m_DropActive = true;
		ExecuteAction(m_OnEnterAction, files, screenPoint, false);
	}
}

void MeasureDragDrop::HandleDragOver(const std::vector<std::wstring>& files, const POINTL& screenPoint)
{
	if (!ContainsPoint(screenPoint))
	{
		if (m_DropActive)
		{
			m_DropActive = false;
			ExecuteAction(m_OnLeaveAction, files, screenPoint, false);
		}
	}
	else
	{
		if (!m_DropActive)
		{
			m_DropActive = true;
			ExecuteAction(m_OnEnterAction, files, screenPoint, false);
		}

		ExecuteAction(m_OnOverAction, files, screenPoint, false);
	}
}

void MeasureDragDrop::HandleDragLeave(const std::vector<std::wstring>& files)
{
	if (m_DropActive)
	{
		m_DropActive = false;
		const POINTL invalidPoint = { INT_MIN, INT_MIN };
		ExecuteAction(m_OnLeaveAction, files, invalidPoint, false);
	}
}

void MeasureDragDrop::HandleDrop(const std::vector<std::wstring>& files, const POINTL& screenPoint)
{
	if (ContainsPoint(screenPoint))
	{
		m_DropActive = false;
		ExecuteAction(m_OnDropAction, files, screenPoint, true);
	}
}

DWORD MeasureDragDrop::GetDropEffect() const
{
	switch (m_Action)
	{
	case DropAction::Move:
	case DropAction::Delete:
		return DROPEFFECT_MOVE;

	case DropAction::Copy:
	case DropAction::Shortcut:
		return DROPEFFECT_COPY;

	case DropAction::Path:
		return DROPEFFECT_LINK;

	default:
		return DROPEFFECT_NONE;
	}
}

bool MeasureDragDrop::ResolveBounds(RECT& bounds) const
{
	if (!m_BoundsMeter.empty())
	{
		Meter* meter = m_Skin->GetMeter(m_BoundsMeter);
		if (!meter)
		{
			return false;
		}

		bounds.left = meter->GetX();
		bounds.top = meter->GetY();
		bounds.right = meter->GetW();
		bounds.bottom = meter->GetH();
		return bounds.right > 0 && bounds.bottom > 0;
	}

	if (m_UsingFixedBounds)
	{
		std::array<double, 4> values = { 0.0, 0.0, 0.0, 0.0 };
		for (size_t i = 0; i < m_BoundsFormulas.size(); ++i)
		{
			if (!m_Skin->GetParser().ParseFormula(m_BoundsFormulas[i], &values[i]))
			{
				return false;
			}
		}

		bounds.left = (LONG)values[0];
		bounds.top = (LONG)values[1];
		bounds.right = (LONG)values[2];
		bounds.bottom = (LONG)values[3];
		return bounds.right > 0 && bounds.bottom > 0;
	}

	bounds.left = 0;
	bounds.top = 0;
	bounds.right = m_Skin->GetW();
	bounds.bottom = m_Skin->GetH();
	return true;
}

bool MeasureDragDrop::ExecuteAction(const std::wstring& action, const std::vector<std::wstring>& files, const POINTL& screenPoint, bool isDrop)
{
	if (files.empty())
	{
		return false;
	}

	if (!m_ProcessAllFiles)
	{
		return ExecuteSingleAction(action, files.front(), screenPoint, 1, isDrop);
	}

	bool result = false;
	for (size_t i = 0; i < files.size(); ++i)
	{
		result = ExecuteSingleAction(action, files[i], screenPoint, (int)i + 1, isDrop) || result;
	}

	return result;
}

bool MeasureDragDrop::ExecuteSingleAction(const std::wstring& action, const std::wstring& file, const POINTL& screenPoint, int number, bool isDrop)
{
	std::wstring fileName;
	std::wstring fileType;
	std::wstring directory = PathUtil::GetFolderFromFilePath(file);
	PathUtil::RemoveTrailingBackslash(directory);

	const DWORD attributes = GetFileAttributes(file.c_str());
	const bool isFolder = attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

	const auto separatorPos = file.find_last_of(L"\\/");
	const std::wstring leafName = separatorPos == std::wstring::npos ? file : file.substr(separatorPos + 1);
	if (isFolder)
	{
		fileName = leafName;
		fileType = L"folder";
	}
	else
	{
		fileName = GetFileStem(leafName);
		const auto dotPos = leafName.find_last_of(L'.');
		if (dotPos != std::wstring::npos)
		{
			fileType = leafName.substr(dotPos + 1);
		}
	}

	std::wstring destinationFile;
	if (!m_Path.empty())
	{
		destinationFile = m_Path;
		PathUtil::AppendBackslashIfMissing(destinationFile);
		destinationFile += fileName;
		if (!fileType.empty() && fileType != L"folder" && m_Action != DropAction::Shortcut)
		{
			destinationFile += L'.';
			destinationFile += fileType;
		}
	}

	if (isDrop && !PerformDropAction(file, destinationFile, isFolder))
	{
		return false;
	}

	if (isDrop)
	{
		m_StringValue = file;
	}

	if (action.empty())
	{
		return isDrop;
	}

	std::wstring substituted = action;
	substituted = ReplaceToken(substituted, L"$Name$", fileName);
	substituted = ReplaceToken(substituted, L"$FileName$", fileName);
	substituted = ReplaceToken(substituted, L"$Type$", fileType);
	substituted = ReplaceToken(substituted, L"$FileType$", fileType);
	substituted = ReplaceToken(substituted, L"$Directory$", directory);
	substituted = ReplaceToken(substituted, L"$FileDirectory$", directory);
	substituted = ReplaceToken(substituted, L"$File$", file);
	substituted = ReplaceToken(substituted, L"$FilePath$", file);
	substituted = ReplaceToken(substituted, L"$FileNr$", std::to_wstring(number));
	substituted = ReplaceToken(substituted, L"$MouseX$", std::to_wstring(screenPoint.x));
	substituted = ReplaceToken(substituted, L"$MouseY$", std::to_wstring(screenPoint.y));
	GetRainmeter().ExecuteCommand(substituted.c_str(), m_Skin);
	return true;
}

bool MeasureDragDrop::PerformDropAction(const std::wstring& file, const std::wstring& destinationFile, bool isFolder)
{
	switch (m_Action)
	{
	case DropAction::None:
	case DropAction::Path:
		return true;

	case DropAction::Copy:
		if (!EnsureDirectoryExists(m_Path))
		{
			LogErrorF(this, L"Could not create directory: %s", m_Path.c_str());
			return false;
		}
		return isFolder ? CopyDirectory(file, destinationFile) : CopyFile(file.c_str(), destinationFile.c_str(), !m_OverrideExisting) != 0;

	case DropAction::Move:
		if (!EnsureDirectoryExists(m_Path))
		{
			LogErrorF(this, L"Could not create directory: %s", m_Path.c_str());
			return false;
		}
		return MoveFile(file.c_str(), destinationFile.c_str()) != 0;

	case DropAction::Delete:
		{
			std::wstring from = file;
			from.push_back(L'\0');

			SHFILEOPSTRUCT operation = { 0 };
			operation.wFunc = FO_DELETE;
			operation.pFrom = from.c_str();
			operation.fFlags = FOF_ALLOWUNDO;
			if (m_Silent)
			{
				operation.fFlags |= FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOERRORUI;
			}

			return SHFileOperation(&operation) == 0;
		}

	case DropAction::Shortcut:
		if (!EnsureDirectoryExists(m_Path))
		{
			LogErrorF(this, L"Could not create directory: %s", m_Path.c_str());
			return false;
		}
		return CreateShortcut(file, destinationFile);
	}

	return false;
}

bool MeasureDragDrop::CopyDirectory(const std::wstring& from, const std::wstring& to)
{
	if (!CreateDirectory(to.c_str(), nullptr))
	{
		const DWORD error = GetLastError();
		if (error != ERROR_ALREADY_EXISTS)
		{
			return false;
		}
	}

	std::wstring spec = from;
	PathUtil::AppendBackslashIfMissing(spec);
	spec += L'*';

	WIN32_FIND_DATA findData = { 0 };
	HANDLE find = FindFirstFileEx(spec.c_str(), FindExInfoBasic, &findData, FindExSearchNameMatch, nullptr, FIND_FIRST_EX_LARGE_FETCH);
	if (find == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	bool success = true;
	do
	{
		if (PathUtil::IsDotOrDotDot(findData.cFileName))
		{
			continue;
		}

		std::wstring fromItem = from;
		PathUtil::AppendBackslashIfMissing(fromItem);
		fromItem += findData.cFileName;

		std::wstring toItem = to;
		PathUtil::AppendBackslashIfMissing(toItem);
		toItem += findData.cFileName;

		if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
		{
			success = CopyDirectory(fromItem, toItem);
		}
		else
		{
			success = CopyFile(fromItem.c_str(), toItem.c_str(), !m_OverrideExisting) != 0;
		}
	}
	while (success && FindNextFile(find, &findData) != FALSE);

	FindClose(find);
	return success;
}

bool MeasureDragDrop::CreateShortcut(const std::wstring& targetFile, const std::wstring& shortcutFile)
{
	Microsoft::WRL::ComPtr<IShellLink> shellLink;
	HRESULT hr = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&shellLink));
	if (SUCCEEDED(hr))
	{
		shellLink->SetPath(targetFile.c_str());
		shellLink->SetDescription(L"Automatically generated shortcut by Rainmeter DragDrop");

		Microsoft::WRL::ComPtr<IPersistFile> persistFile;
		hr = shellLink->QueryInterface(IID_PPV_ARGS(&persistFile));
		if (SUCCEEDED(hr))
		{
			hr = persistFile->Save(shortcutFile.c_str(), TRUE);
		}
	}

	if (FAILED(hr))
	{
		LogErrorF(this, L"Could not create shortcut: %s", shortcutFile.c_str());
		return false;
	}

	return true;
}
