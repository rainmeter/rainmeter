// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "DialogPackage.h"
#include "SkinInstaller.h"
#include "DialogInstall.h"
#include "resource.h"
#include "Util.h"
#include "../Common/FileUtil.h"
#include "../Common/ShellDialog.h"
#include "../Common/StringParser.h"
#include "../Common/StringUtil.h"
#include "../Version.h"

#include "iowin32.h"

#define WM_DELAYED_CLOSE WM_APP + 0

extern GlobalData g_Data;
DialogPackage* DialogPackage::c_Dialog = nullptr;

DialogPackage::DialogPackage() : Dialog(),
	m_LoadLayout(false),
	m_MergeSkins(false),
	m_ProfileLoaded(false),
	m_OptionsCreated(false),
	m_PackagerThread(),
	m_ZipFile(),
	m_AllowNonAsciiFilenames(false)
{
}

DialogPackage::~DialogPackage()
{
}

// The options of previously created packages are remembered so that publishing a new version of a
// skin does not require selecting everything again. Each skin folder gets a section of its own.
static std::wstring GetProfileFile()
{
	return g_Data.settingsPath + L"SkinPackager.ini";
}

static std::wstring ReadProfileString(const WCHAR* section, const WCHAR* key, const std::wstring& file)
{
	WCHAR buffer[MAX_LINE_LENGTH] = { 0 };
	GetPrivateProfileString(section, key, L"", buffer, _countof(buffer), file.c_str());
	return buffer;
}

static void WriteProfileString(const WCHAR* section, const WCHAR* key, const std::wstring& value, const std::wstring& file)
{
	if (value.empty()) return;

	WritePrivateProfileString(section, key, value.c_str(), file.c_str());
}

static bool FolderExists(const std::wstring& path)
{
	const DWORD attributes = GetFileAttributes(path.c_str());
	return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY);
}

static bool FileExists(const std::wstring& path)
{
	const DWORD attributes = GetFileAttributes(path.c_str());
	return attributes != INVALID_FILE_ATTRIBUTES && !(attributes & FILE_ATTRIBUTE_DIRECTORY);
}

// Plugins are stored on one line as "x32 <path> | x64 <path>".
static std::pair<std::wstring, std::wstring> GetPluginPaths(const std::wstring& value)
{
	std::pair<std::wstring, std::wstring> paths;

	StringParser::ForEachToken(value, L'|', [&](std::wstring_view token)
	{
		StringParser plugin(token);
		if (plugin.Consume(L"x32", StringParser::SkipWhitespace))
		{
			paths.first = plugin.ConsumeRest();
		}
		else if (plugin.Consume(L"x64", StringParser::SkipWhitespace))
		{
			paths.second = plugin.ConsumeRest();
		}
	});

	return paths;
}

static std::wstring GetLocalTimeString()
{
	SYSTEMTIME time = { 0 };
	GetLocalTime(&time);

	WCHAR buffer[32] = { 0 };
	_snwprintf_s(buffer, _TRUNCATE, L"%04u-%02u-%02u %02u:%02u",
		(UINT)time.wYear, (UINT)time.wMonth, (UINT)time.wDay, (UINT)time.wHour, (UINT)time.wMinute);
	return buffer;
}

static std::wstring GetCurrentRainmeterVersion()
{
	WCHAR buffer[32] = { 0 };
	_snwprintf_s(buffer, _TRUNCATE, L"%s.%i", APPVERSION, revision_number);
	return buffer;
}

void DialogPackage::Create(HINSTANCE hInstance, LPWSTR lpCmdLine)
{
	(void)hInstance;
	(void)lpCmdLine;

	HANDLE hMutex;
	if (IsRunning(L"Rainmeter Skin Packager", &hMutex))
	{
		HWND hwnd = FindWindow(L"#32770", GetString(IDS_RainmeterSkinPackager));
		SetForegroundWindow(hwnd);
	}
	else
	{
		DialogPackage dialog;
		c_Dialog = &dialog;
		dialog.ShowDialogWindow(
			GetString(IDS_RainmeterSkinPackager),
			0, 0, 320, 283,
			DS_CENTER | WS_POPUP | WS_MINIMIZEBOX | WS_CAPTION | WS_SYSMENU,
			WS_EX_APPWINDOW | WS_EX_CONTROLPARENT,
			nullptr,
			false);
		c_Dialog = nullptr;
		ReleaseMutex(hMutex);
	}
}

INT_PTR DialogPackage::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	const INT_PTR baseResult = Dialog::HandleMessage(uMsg, wParam, lParam);

	switch (uMsg)
	{
	case WM_INITDIALOG:
		return OnInitDialog(wParam, lParam);

	case WM_COMMAND:
		return OnCommand(wParam, lParam);

	case WM_CLOSE:
		if (!m_PackagerThread)
		{
			EndDialog(m_Window, 0);
		}
		return TRUE;
	}

	return baseResult;
}

INT_PTR DialogPackage::OnInitDialog(WPARAM wParam, LPARAM lParam)
{
	const Control controls[] =
	{
		Control::Button(DialogPackage::Id_BackButton, IDS_Back,
			123, 264, 50, 14,
			WS_TABSTOP, 0),
		Control::Button(DialogPackage::Id_NextButton, IDS_Next,
			208, 264, 50, 14,
			WS_VISIBLE | WS_TABSTOP | WS_DISABLED | BS_DEFPUSHBUTTON, 0),
		Control::Button(DialogPackage::Id_CreatePackageButton, IDS_CreatePackage,
			178, 264, 80, 14,
			WS_TABSTOP, 0),
		Control::Button(DialogPackage::Id_CancelButton, IDS_Cancel,
			263, 264, 50, 14,
			WS_VISIBLE | WS_TABSTOP, 0),
		Control::Button(DialogPackage::Id_LoadPreviousButton, IDS_LoadPrevious,
			6, 264, 100, 14,
			WS_VISIBLE | WS_TABSTOP, 0),
		Control::Tab(DialogPackage::Id_Tab, 0,
			6, 6, 308, 254,
			WS_VISIBLE | WS_TABSTOP | TCS_FIXEDWIDTH, 0)
	};

	CreateControls(controls, _countof(controls), GetString);

	Dialog::SetMenuButton(GetDlgItem(m_Window, DialogPackage::Id_LoadPreviousButton));
	SetLoadPreviousButtonState();

	AddPage(m_TabInfo);

	HICON hIcon = GetIcon(IDI_SKININSTALLER, true);
	SendMessage(m_Window, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);  // Titlebar icon: 16x16
	SendMessage(m_Window, WM_SETICON, ICON_BIG, (LPARAM)hIcon);    // Taskbar icon:  32x32

	m_TabInfo.Activate();

	return FALSE;
}

INT_PTR DialogPackage::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(wParam))
	{
	case DialogPackage::Id_NextButton:
		{
			if (!m_OptionsCreated)
			{
				AddTab(DialogPackage::Id_Tab, m_TabOptions, GetString(IDS_Options));
				AddTab(DialogPackage::Id_Tab, m_TabAdvanced, GetString(IDS_Advanced));
				m_OptionsCreated = true;
			}

			HWND item = GetDlgItem(m_Window, DialogPackage::Id_NextButton);
			ShowWindow(item, SW_HIDE);

			item = GetDlgItem(m_Window, DialogPackage::Id_LoadPreviousButton);
			ShowWindow(item, SW_HIDE);

			item = GetDlgItem(m_Window, DialogPackage::Id_BackButton);
			ShowWindow(item, SW_SHOWNORMAL);

			item = GetDlgItem(m_Window, DialogPackage::Id_CreatePackageButton);
			ShowWindow(item, SW_SHOWNORMAL);
			SendMessage(m_Window, DM_SETDEFID, DialogPackage::Id_CreatePackageButton, 0);

			HWND tab = GetDlgItem(m_Window, DialogPackage::Id_Tab);
			EnableWindow(tab, TRUE);
			ShowWindow(tab, SW_SHOWNORMAL);
			ShowWindow(m_TabInfo.GetWindow(), SW_HIDE);
			SelectTab(0);
		}
		break;

	case DialogPackage::Id_BackButton:
		{
			HWND item = GetDlgItem(m_Window, DialogPackage::Id_BackButton);
			ShowWindow(item, SW_HIDE);

			item = GetDlgItem(m_Window, DialogPackage::Id_CreatePackageButton);
			ShowWindow(item, SW_HIDE);

			item = GetDlgItem(m_Window, DialogPackage::Id_NextButton);
			ShowWindow(item, SW_SHOWNORMAL);
			SendMessage(m_Window, DM_SETDEFID, DialogPackage::Id_NextButton, 0);

			item = GetDlgItem(m_Window, DialogPackage::Id_LoadPreviousButton);
			ShowWindow(item, SW_SHOWNORMAL);

			HWND tab = GetDlgItem(m_Window, DialogPackage::Id_Tab);
			EnableWindow(tab, FALSE);
			ShowWindow(tab, SW_HIDE);
			EnableWindow(m_TabOptions.GetWindow(), FALSE);
			EnableWindow(m_TabAdvanced.GetWindow(), FALSE);
			ShowWindow(m_TabInfo.GetWindow(), SW_SHOWNORMAL);
			m_TabInfo.Activate();
		}
		break;

	case DialogPackage::Id_CreatePackageButton:
		{
			HWND item = GetDlgItem(m_Window, DialogPackage::Id_CreatePackageButton);
			EnableWindow(item, FALSE);

			item = GetDlgItem(m_Window, DialogPackage::Id_BackButton);
			EnableWindow(item, FALSE);

			item = GetDlgItem(m_Window, DialogPackage::Id_CancelButton);
			EnableWindow(item, FALSE);

			SelectTab(0);
			item = GetDlgItem(m_Window, DialogPackage::Id_Tab);
			EnableWindow(item, FALSE);
			EnableWindow(m_TabOptions.GetWindow(), FALSE);
			EnableWindow(m_TabAdvanced.GetWindow(), FALSE);

			item = GetDlgItem(m_TabOptions.GetWindow(), DialogPackage::TabOptions::Id_CreatingText);
			ShowWindow(item, SW_SHOWNORMAL);

			item = GetDlgItem(m_TabOptions.GetWindow(), DialogPackage::TabOptions::Id_CreatingBar);
			ShowWindow(item, SW_SHOWNORMAL);
			SendMessage(item, PBM_SETMARQUEE, (WPARAM)TRUE, 0);

			m_PackagerThread = (HANDLE)_beginthreadex(nullptr, 0, PackagerThreadProc, this, 0, nullptr);
			if (!m_PackagerThread)
			{
				MessageBox(m_Window, GetString(IDS_UnknownError), GetString(IDS_RainmeterSkinPackager), MB_ERROR);
				EndDialog(m_Window, 0);
			}
		}
		break;

	case DialogPackage::Id_LoadPreviousButton:
		if (HIWORD(wParam) == BN_CLICKED)
		{
			ShowLoadPreviousMenu((HWND)lParam);
		}
		break;

	case DialogPackage::Id_CancelButton:
		if (!m_PackagerThread)
		{
			EndDialog(m_Window, 0);
		}
		break;

	default:
		return FALSE;
	}

	return TRUE;
}

void DialogPackage::SetNextButtonState()
{
	BOOL state = !(m_Name.empty() || m_Author.empty() || m_SkinFolder.second.empty());
	EnableWindow(GetDlgItem(m_Window, DialogPackage::Id_NextButton), state);
}

void DialogPackage::SetLoadPreviousButtonState()
{
	EnableWindow(GetDlgItem(m_Window, DialogPackage::Id_LoadPreviousButton), !GetProfiles().empty());
}

std::vector<DialogPackage::Profile> DialogPackage::GetProfiles()
{
	const std::wstring file = GetProfileFile();

	std::vector<WCHAR> buffer(32768);
	const DWORD length = GetPrivateProfileSectionNames(buffer.data(), (DWORD)buffer.size(), file.c_str());

	// The section names come back null separated.
	std::vector<Profile> profiles;
	StringParser::ForEachToken(std::wstring_view(buffer.data(), length), L'\0', [&](std::wstring_view token)
	{
		const std::wstring section(token);
		profiles.emplace_back(Profile{
			section,
			ReadProfileString(section.c_str(), L"SkinFolder", file),
			ReadProfileString(section.c_str(), L"Timestamp", file) });
	}, StringParser::None);

	std::sort(profiles.begin(), profiles.end(), [](const Profile& lhs, const Profile& rhs)
	{
		return _wcsicmp(lhs.name.c_str(), rhs.name.c_str()) < 0;
	});

	return profiles;
}

void DialogPackage::ShowLoadPreviousMenu(HWND button)
{
	const auto profiles = GetProfiles();

	HMENU menu = CreatePopupMenu();
	for (size_t i = 0; i < profiles.size(); ++i)
	{
		// A tab puts the date where a menu would put the accelerator, which right aligns it.
		std::wstring text = profiles[i].name;
		if (!profiles[i].timestamp.empty())
		{
			text += L'\t';
			text += profiles[i].timestamp;
		}

		// The skin folder may have been moved or deleted since the package was created.
		const UINT flags = MF_STRING | (FolderExists(profiles[i].skinFolder) ? 0 : MF_GRAYED);
		AppendMenu(menu, flags, (UINT_PTR)i + 1, text.c_str());
	}

	const UINT command = Dialog::ShowMenuButtonPopupMenu(menu, button, m_Window, TPM_RETURNCMD | TPM_NONOTIFY);
	DestroyMenu(menu);

	if (command >= 1 && command <= profiles.size())
	{
		LoadProfile(profiles[command - 1].skinFolder);
	}
}

void DialogPackage::SetSkinFolder(const std::wstring& skinFolder)
{
	m_SkinFolder.second = skinFolder;

	// The file can be edited by hand, so the trailing slash cannot be taken for granted.
	if (m_SkinFolder.second.back() != L'\\') m_SkinFolder.second += L'\\';

	m_SkinFolder.first = PathFindFileName(m_SkinFolder.second.c_str());
	m_SkinFolder.first.pop_back();	// Remove slash
}

void DialogPackage::LoadProfile(const std::wstring& skinFolder)
{
	if (skinFolder.empty()) return;

	SetSkinFolder(skinFolder);

	const std::wstring file = GetProfileFile();
	const WCHAR* section = m_SkinFolder.first.c_str();

	// Every package has a name, so its absence means that this skin has not been packaged before.
	// Whatever has already been entered is then left as is.
	std::wstring name = ReadProfileString(section, L"Name", file);
	if (!name.empty())
	{
		m_ProfileLoaded = true;

		m_Name = std::move(name);
		m_Author = ReadProfileString(section, L"Author", file);
		m_Version = ReadProfileString(section, L"Version", file);
		m_MinimumRainmeter = ReadProfileString(section, L"MinimumRainmeter", file);
		m_VariableFiles = ReadProfileString(section, L"VariableFiles", file);
		m_MergeSkins = GetPrivateProfileInt(section, L"MergeSkins", 0, file.c_str()) != 0;
		m_LoadLayout = _wcsicmp(ReadProfileString(section, L"LoadType", file).c_str(), L"Layout") == 0;
		m_Load = ReadProfileString(section, L"Load", file);

		m_OutputFolder = ReadProfileString(section, L"OutputFolder", file);
		if (!FolderExists(m_OutputFolder)) m_OutputFolder.clear();

		m_HeaderFile = ReadProfileString(section, L"HeaderImage", file);
		if (!FileExists(m_HeaderFile)) m_HeaderFile.clear();

		WCHAR key[32] = { 0 };

		m_LayoutFolders.clear();
		for (int i = 1; ; ++i)
		{
			_snwprintf_s(key, _TRUNCATE, L"Layout%i", i);
			std::wstring folder = ReadProfileString(section, key, file);
			if (folder.empty()) break;
			if (!FolderExists(folder)) continue;

			std::wstring layoutName = PathFindFileName(folder.c_str());
			layoutName.pop_back();	// Remove slash
			m_LayoutFolders.emplace(layoutName, folder);
		}

		m_PluginFolders.clear();
		for (int i = 1; ; ++i)
		{
			_snwprintf_s(key, _TRUNCATE, L"Plugin%i", i);
			const std::wstring value = ReadProfileString(section, key, file);
			if (value.empty()) break;

			// A package needs both architectures, so one without them is of no use here.
			auto paths = GetPluginPaths(value);
			if (!FileExists(paths.first) || !FileExists(paths.second)) continue;

			const std::wstring name = PathFindFileName(paths.first.c_str());
			m_PluginFolders.emplace(name, std::move(paths));
		}

		// The skin to load after installation is stored relative to the skins folder, so the skin
		// folder in front of it has to be swapped for the real one.
		if (!m_LoadLayout && !m_Load.empty())
		{
			StringParser load(m_Load);
			const bool hasSkinFolder = !load.ConsumeUntil(L'\\').empty();

			std::wstring path = m_SkinFolder.second;
			path += load.ConsumeRest();

			if (!hasSkinFolder || !FileExists(path)) m_Load.clear();
		}
	}

	UpdateTabs();
	SetNextButtonState();
}

void DialogPackage::SaveProfile()
{
	const std::wstring file = GetProfileFile();
	const WCHAR* section = m_SkinFolder.first.c_str();

	// Rewrite the whole section so that removed layouts and plugins do not linger.
	WritePrivateProfileString(section, nullptr, nullptr, file.c_str());

	WriteProfileString(section, L"Timestamp", GetLocalTimeString(), file);
	WriteProfileString(section, L"Name", m_Name, file);
	WriteProfileString(section, L"Author", m_Author, file);
	WriteProfileString(section, L"Version", m_Version, file);
	WriteProfileString(section, L"SkinFolder", m_SkinFolder.second, file);

	int index = 1;
	WCHAR key[32] = { 0 };
	for (const auto& layout : m_LayoutFolders)
	{
		_snwprintf_s(key, _TRUNCATE, L"Layout%i", index++);
		WriteProfileString(section, key, layout.second, file);
	}

	index = 1;
	for (const auto& plugin : m_PluginFolders)
	{
		std::wstring value = L"x32 ";
		value += plugin.second.first;
		value += L" | x64 ";
		value += plugin.second.second;

		_snwprintf_s(key, _TRUNCATE, L"Plugin%i", index++);
		WriteProfileString(section, key, value, file);
	}

	std::wstring outputFolder = m_TargetFile;
	outputFolder.resize(PathFindFileName(outputFolder.c_str()) - outputFolder.c_str());
	WriteProfileString(section, L"OutputFolder", outputFolder, file);

	if (!m_Load.empty())
	{
		WriteProfileString(section, L"LoadType", m_LoadLayout ? L"Layout" : L"Skin", file);
		WriteProfileString(section, L"Load", m_Load, file);
	}

	// The minimum version follows the installed Rainmeter unless it has been set to something else.
	if (m_MinimumRainmeter != GetCurrentRainmeterVersion())
	{
		WriteProfileString(section, L"MinimumRainmeter", m_MinimumRainmeter, file);
	}

	WriteProfileString(section, L"HeaderImage", m_HeaderFile, file);

	if (m_MergeSkins)
	{
		WriteProfileString(section, L"MergeSkins", L"1", file);
	}
	else
	{
		WriteProfileString(section, L"VariableFiles", m_VariableFiles, file);
	}
}

void DialogPackage::UpdateTabs()
{
	if (m_TabInfo.IsInitialized()) m_TabInfo.Update();
	if (m_TabOptions.IsInitialized()) m_TabOptions.Update();
	if (m_TabAdvanced.IsInitialized()) m_TabAdvanced.Update();
}

bool DialogPackage::CreatePackage()
{
	// Create options file
	WCHAR tempFile[MAX_PATH];
	GetTempPath(MAX_PATH, tempFile);
	GetTempFileName(tempFile, L"ini", 0, tempFile);

	WritePrivateProfileString(L"rmskin", L"Name", m_Name.c_str(), tempFile);
	WritePrivateProfileString(L"rmskin", L"Author", m_Author.c_str(), tempFile);
	WritePrivateProfileString(L"rmskin", L"Version", m_Version.c_str(), tempFile);

	if (!c_Dialog->m_Load.empty())
	{
		WritePrivateProfileString(L"rmskin", L"LoadType", c_Dialog->m_LoadLayout ? L"Layout" : L"Skin", tempFile);
		WritePrivateProfileString(L"rmskin", L"Load", c_Dialog->m_Load.c_str(), tempFile);
	}

	if (c_Dialog->m_MergeSkins)
	{
		WritePrivateProfileString(L"rmskin", L"MergeSkins", L"1", tempFile);
	}
	else
	{
		// "Merge skins" not compatible with "Variable files"
		if (!c_Dialog->m_VariableFiles.empty())
		{
			WritePrivateProfileString(L"rmskin", L"VariableFiles", m_VariableFiles.c_str(), tempFile);
		}
	}

	WritePrivateProfileString(L"rmskin", L"MinimumRainmeter", m_MinimumRainmeter.c_str(), tempFile);

	// Only Skin Installer in Rainmeter 3.0.1 support UTF-8 filenames.
	m_AllowNonAsciiFilenames = DialogInstall::CompareVersions(m_MinimumRainmeter, L"3.0.1") != -1;

	// Create archive and add options file and header bitmap
	zlib_filefunc64_def zlibFileFunc = { 0 };
	fill_win32_filefunc64W(&zlibFileFunc);
	m_ZipFile = zipOpen2_64(m_TargetFile.c_str(), APPEND_STATUS_CREATE, nullptr, &zlibFileFunc);

	auto cleanup = [&]()->bool
	{
		zipClose(m_ZipFile, nullptr);
		DeleteFile(m_TargetFile.c_str());
		return false;
	};

	if (!m_ZipFile ||
		(!c_Dialog->m_HeaderFile.empty() && !AddFileToPackage(c_Dialog->m_HeaderFile.c_str(), L"RMSKIN.bmp")) ||
		!AddFileToPackage(tempFile, L"RMSKIN.ini"))
	{
		std::wstring error = GetString(IDS_UnableCreatePackage);
		error += L"\n\n";
		error += GetString(IDS_CloseMessage);
		MessageBox(m_Window, error.c_str(), GetString(IDS_RainmeterSkinPackager), MB_OK | MB_ICONERROR);
		DeleteFile(tempFile);
		return cleanup();
	}

	// Add skin
	{
		std::wstring zipPrefix = L"Skins\\" + m_SkinFolder.first;
		zipPrefix += L'\\';
		if (!AddFolderToPackage(m_SkinFolder.second, L"", zipPrefix.c_str()))
		{
			return cleanup();
		}
	}

	// Add layouts
	for (auto iter = m_LayoutFolders.cbegin(); iter != m_LayoutFolders.cend(); ++iter)
	{
		std::wstring realPath = (*iter).second + L"Rainmeter.ini";
		std::wstring zipPath = L"Layouts\\" + (*iter).first;
		zipPath += L"\\Rainmeter.ini";
		if (!AddFileToPackage(realPath.c_str(), zipPath.c_str()))
		{
			std::wstring error = GetFormattedString(IDS_LayoutError, (*iter).first.c_str());
			error += L"\n\n";
			error += GetString(IDS_CloseMessage);
			MessageBox(m_Window, error.c_str(), GetString(IDS_RainmeterSkinPackager), MB_OK | MB_ICONERROR);
			return cleanup();
		}
	}

	// Add plugins
	for (auto iter = m_PluginFolders.cbegin(); iter != m_PluginFolders.cend(); ++iter)
	{
		// Add 32bit and 64bit versions
		for (int i = 0; i < 2; ++i)
		{
			const std::wstring& realPath = (i == 0) ? (*iter).second.first : (*iter).second.second;
			std::wstring zipPath = ((i == 0) ? L"Plugins\\32bit\\" : L"Plugins\\64bit\\") + (*iter).first;
			if (!AddFileToPackage(realPath.c_str(), zipPath.c_str()))
			{
				std::wstring error = GetFormattedString(IDS_PluginError, (*iter).first.c_str());
				error += L"\n\n";
				error += GetString(IDS_CloseMessage);
				MessageBox(m_Window, error.c_str(), GetString(IDS_RainmeterSkinPackager), MB_OK | MB_ICONERROR);
				return cleanup();
			}
		}
	}

	// Add footer
	FILE* file = nullptr;
	if (zipClose(m_ZipFile, nullptr) == ZIP_OK &&
		(file = _wfopen(m_TargetFile.c_str(), L"r+b")) != nullptr)
	{
		fseek(file, 0, SEEK_END);
		DialogInstall::PackageFooter footer = { _ftelli64(file), 0, "RMSKIN" };
		fwrite(&footer, sizeof(footer), 1, file);
		fclose(file);

		SaveProfile();
	}
	else
	{
		std::wstring error = GetString(IDS_UnableCreatePackage);
		error += L"\n\n";
		error += GetString(IDS_CloseMessage);
		MessageBox(m_Window, error.c_str(), GetString(IDS_RainmeterSkinPackager), MB_OK | MB_ICONERROR);
		return false;
	}

	return true;
}

unsigned __stdcall DialogPackage::PackagerThreadProc(void* pParam)
{
	DialogPackage* dialog = (DialogPackage*)pParam;

	if (dialog->CreatePackage())
	{
		// Stop the progress bar
		HWND item = GetDlgItem(dialog->m_TabOptions.GetWindow(), DialogPackage::TabOptions::Id_CreatingBar);
		SendMessage(item, PBM_SETMARQUEE, (WPARAM)FALSE, 0);

		FlashWindow(dialog->m_Window, TRUE);

		std::wstring message = GetString(IDS_RmskinSuccessfullyCreated);
		message += L"\n\n";
		message += GetString(IDS_CloseMessage);
		MessageBox(c_Dialog->GetWindow(), message.c_str(), GetString(IDS_RainmeterSkinPackager), MB_OK | MB_ICONINFORMATION);
	}
	else
	{
		DeleteFile(dialog->m_TargetFile.c_str());
	}

	EndDialog(dialog->m_Window, 0);

	return 0;
}

bool DialogPackage::AddFileToPackage(const WCHAR* filePath, const WCHAR* zipPath)
{
	std::string zipPathUTF8 = StringUtil::NarrowUTF8(zipPath);
	for (size_t i = 0, isize = zipPathUTF8.length(); i < isize; ++i)
	{
		if ((zipPathUTF8[i] & 0x80) != 0)
		{
			// UTF-8 lead bit is not zero so the string is non-ASCII.
			if (!m_AllowNonAsciiFilenames)
			{
				return false;
			}
		}

		if (zipPathUTF8[i] == '\\')
		{
			zipPathUTF8[i] = '/';
		}
	}

	const uLong ZIP_UTF8_FLAG = 1 << 11;
	zip_fileinfo fi = { 0 };
	if (zipOpenNewFileInZip4(
		m_ZipFile, zipPathUTF8.c_str(), &fi,
		nullptr, 0, nullptr, 0, nullptr, Z_DEFLATED, Z_DEFAULT_COMPRESSION,
		0, -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY, NULL, 0, 0, ZIP_UTF8_FLAG) != ZIP_OK)
	{
		return false;
	}

	bool result = true;

	if (filePath)
	{
		HANDLE file = CreateFile(filePath, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,  FILE_ATTRIBUTE_NORMAL, nullptr);
		if (file == INVALID_HANDLE_VALUE)
		{
			result = false;
		}
		else
		{
			do
			{
				const DWORD bufferSize = 16 * 1024;
				BYTE buffer[bufferSize] = { 0 };
				DWORD readSize = 0;
				if (!ReadFile(file, buffer, bufferSize, &readSize, nullptr))
				{
					result = false;
				}
				else if (readSize != 0)
				{
					result = zipWriteInFileInZip(m_ZipFile, buffer, (UINT)readSize) == ZIP_OK;
				}
				else
				{
					// EOF
					break;
				}
			}
			while (result);

			CloseHandle(file);
		}
	}
	else
	{
		// Directory entry, so nothing needs to be written.
	}

	return zipCloseFileInZip(m_ZipFile) == ZIP_OK && result;
}

bool DialogPackage::AddFolderToPackage(const std::wstring& path, std::wstring base, const WCHAR* zipPrefix)
{
	std::wstring currentPath = path + base;
	currentPath += L'*';

	WIN32_FIND_DATA fd = { 0 };
	HANDLE hFind = FindFirstFileEx(
		currentPath.c_str(),
		FindExInfoBasic,
		&fd,
		FindExSearchNameMatch,
		nullptr,
		0);

	if (hFind == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	currentPath.pop_back();	// Remove *

	bool result = true;
	bool filesAdded = false;
	std::list<std::wstring> folders;
	do
	{
		if (fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
		{
			// Ignore hidden files and folders
			continue;
		}

		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if (!(fd.cFileName[0] == L'.' && (!fd.cFileName[1] || fd.cFileName[1] == L'.' && !fd.cFileName[2])))
			{
				folders.push_back(fd.cFileName);
			}
		}
		else
		{
			std::wstring filePath = currentPath + fd.cFileName;
			std::wstring zipPath = zipPrefix;
			zipPath.append(filePath, path.length(), filePath.length() - path.length());

			result = AddFileToPackage(filePath.c_str(), zipPath.c_str());
			if (!result)
			{
				std::wstring error = GetFormattedString(IDS_FileError, filePath.c_str());
				error += L"\n\n";
				error += GetString(IDS_CloseMessage);
				MessageBox(m_Window, error.c_str(), GetString(IDS_RainmeterSkinPackager), MB_OK | MB_ICONERROR);
				break;
			}

			filesAdded = true;
		}
	}
	while (FindNextFile(hFind, &fd));
	FindClose(hFind);

	if (result)
	{
		if (!filesAdded && folders.empty())
		{
			// Add directory entry if folder is empty.
			std::wstring zipPath = zipPrefix;
			zipPath.append(currentPath, path.length(), currentPath.length() - path.length());
			AddFileToPackage(nullptr, zipPath.c_str());
		}

		std::list<std::wstring>::const_iterator iter = folders.begin();
		for ( ; iter != folders.end(); ++iter)
		{
			std::wstring newBase = base + (*iter);
			newBase += L'\\';
			result = AddFolderToPackage(path, newBase, zipPrefix);
			if (!result) break;
		}
	}

	return result;
}

void DialogPackage::ShowHelp()
{
	const WCHAR* url = L"https://docs.rainmeter.net/manual/distributing-skins/";
	ShellExecute(m_Window, L"open", url, nullptr, nullptr, SW_SHOWNORMAL);
}

class DialogPackage::SelectFolderDialog : public Dialog
{
public:
	enum Id
	{
		Id_ExistingRadio = 1000,
		Id_ExistingCombo,
		Id_CustomRadio,
		Id_CustomEdit,
		Id_CustomBrowseButton
	};

	SelectFolderDialog(const std::wstring& existingPath) :
		m_ExistingPath(existingPath),
		m_Accepted(false)
	{
	}

	std::wstring ShowModal(HWND parent)
	{
		ShowDialogWindow(
			GetString(IDS_Add),
			0, 0, 200, 100,
			DS_CENTER | WS_POPUP | WS_CAPTION | WS_SYSMENU,
			WS_EX_DLGMODALFRAME | WS_EX_CONTROLPARENT,
			parent,
			false);
		return m_Accepted ? m_Result : std::wstring();
	}

protected:
	INT_PTR HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override
	{
		const INT_PTR baseResult = Dialog::HandleMessage(uMsg, wParam, lParam);
		switch (uMsg)
		{
		case WM_INITDIALOG:
			return OnInitDialog();
		case WM_COMMAND:
			return OnCommand(wParam, lParam);
		case WM_CLOSE:
			EndDialog(m_Window, 0);
			return TRUE;
		}
		return baseResult;
	}

private:
	INT_PTR OnInitDialog()
	{
		const Control controls[] =
		{
			Control::RadioButton(SelectFolderDialog::Id_ExistingRadio, 0,
				6, 6, 188, 13,
				WS_VISIBLE | WS_TABSTOP | WS_GROUP, 0),
			Control::ComboBox(SelectFolderDialog::Id_ExistingCombo, 0,
				17, 21, 176, 14,
				WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST, 0),
			Control::RadioButton(SelectFolderDialog::Id_CustomRadio, IDS_AddCustomFolder,
				6, 44, 188, 13,
				WS_VISIBLE | WS_TABSTOP, 0),
			Control::Edit(SelectFolderDialog::Id_CustomEdit, 0,
				17, 59, 146, 14,
				WS_VISIBLE | WS_TABSTOP | WS_BORDER | WS_DISABLED | ES_AUTOHSCROLL, 0),
			Control::Button(SelectFolderDialog::Id_CustomBrowseButton, IDS_Ellipsis,
				168, 59, 25, 14,
				WS_VISIBLE | WS_TABSTOP | WS_DISABLED, 0),
			Control::Button(IDOK, IDS_Add,
				146, 82, 50, 14,
				WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON, 0)
		};

		CreateControls(controls, _countof(controls), GetString);
		Button_SetCheck(GetControl(SelectFolderDialog::Id_ExistingRadio), BST_CHECKED);
		EnableThemeDialogTexture(m_Window, ETDT_ENABLETAB);

		std::wstring searchPath = m_ExistingPath + L'*';
		WIN32_FIND_DATA fd = { 0 };
		HANDLE hFind = FindFirstFileEx(searchPath.c_str(), FindExInfoBasic, &fd, FindExSearchNameMatch, nullptr, 0);
		if (hFind != INVALID_HANDLE_VALUE)
		{
			const WCHAR* folder = PathFindFileName(m_ExistingPath.c_str());
			std::wstring folderName(folder, wcslen(folder) - 1);
			std::wstring text = GetFormattedString(IDS_AddFolderFrom, folderName.c_str());
			SetWindowText(GetControl(SelectFolderDialog::Id_ExistingRadio), text.c_str());

			HWND combo = GetControl(SelectFolderDialog::Id_ExistingCombo);
			do
			{
				if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY &&
					!(fd.cFileName[0] == L'.' && (!fd.cFileName[1] || fd.cFileName[1] == L'.' && !fd.cFileName[2])) &&
					wcscmp(fd.cFileName, L"Backup") != 0 &&
					wcscmp(fd.cFileName, L"@Backup") != 0 &&
					wcscmp(fd.cFileName, L"@Vault") != 0)
				{
					ComboBox_InsertString(combo, -1, fd.cFileName);
				}
			}
			while (FindNextFile(hFind, &fd));

			ComboBox_SetCurSel(combo, 0);
			FindClose(hFind);
		}

		return TRUE;
	}

	INT_PTR OnCommand(WPARAM wParam, LPARAM lParam)
	{
		switch (LOWORD(wParam))
		{
		case SelectFolderDialog::Id_ExistingRadio:
			{
				HWND combo = GetControl(SelectFolderDialog::Id_ExistingCombo);
				EnableWindow(combo, TRUE);
				EnableWindow(GetControl(SelectFolderDialog::Id_CustomEdit), FALSE);
				EnableWindow(GetControl(SelectFolderDialog::Id_CustomBrowseButton), FALSE);
				EnableWindow(GetControl(IDOK), ComboBox_GetCurSel(combo) != -1);
			}
			break;

		case SelectFolderDialog::Id_CustomRadio:
			EnableWindow(GetControl(SelectFolderDialog::Id_ExistingCombo), FALSE);
			EnableWindow(GetControl(SelectFolderDialog::Id_CustomEdit), TRUE);
			EnableWindow(GetControl(SelectFolderDialog::Id_CustomBrowseButton), TRUE);
			SendMessage(m_Window, WM_COMMAND, MAKEWPARAM(SelectFolderDialog::Id_CustomEdit, EN_CHANGE), 0);
			break;

		case SelectFolderDialog::Id_CustomEdit:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				WCHAR buffer[MAX_PATH] = { 0 };
				Edit_GetText(GetControl(SelectFolderDialog::Id_CustomEdit), buffer, _countof(buffer));
				DWORD attributes = GetFileAttributes(buffer);
				BOOL state = attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY);
				EnableWindow(GetControl(IDOK), state);
			}
			break;

		case SelectFolderDialog::Id_CustomBrowseButton:
			{
				const auto path = ShellDialog::SelectFolder({ .parent = m_Window });
				if (path)
				{
					SetWindowText(GetControl(SelectFolderDialog::Id_CustomEdit), path->c_str());
				}
			}
			break;

		case IDOK:
			{
				WCHAR buffer[MAX_PATH] = { 0 };
				const bool existing = Button_GetCheck(GetControl(SelectFolderDialog::Id_ExistingRadio)) == BST_CHECKED;
				GetWindowText(GetControl(existing ? SelectFolderDialog::Id_ExistingCombo : SelectFolderDialog::Id_CustomEdit), buffer, _countof(buffer));
				m_Result = existing ? m_ExistingPath + buffer : buffer;
				m_Result += L'\\';
				m_Accepted = true;
				EndDialog(m_Window, 0);
			}
			break;

		case IDCANCEL:
			EndDialog(m_Window, 0);
			break;

		default:
			return FALSE;
		}

		return TRUE;
	}

	std::wstring m_ExistingPath;
	std::wstring m_Result;
	bool m_Accepted;
};

// The two architectures of a plugin are usually built into sibling folders whose names differ only
// in the architecture, like "x32" and "x64", or "Release32" and "Release64".
static const WCHAR* const s_ArchitectureNames[][2] =
{
	{ L"32", L"64" },
	{ L"86", L"64" }
};

// Looks for the counterpart of |path| in the sibling folder of the other architecture.
static std::wstring FindOtherArchitecture(const std::wstring& path, bool is32bit)
{
	const WCHAR* fileName = PathFindFileName(path.c_str());
	if (fileName == path.c_str()) return std::wstring();

	const std::wstring folder(path.c_str(), fileName - path.c_str() - 1);  // Without the slash
	const WCHAR* folderName = PathFindFileName(folder.c_str());
	if (folderName == folder.c_str()) return std::wstring();

	const std::wstring parent(folder.c_str(), folderName - folder.c_str());
	const std::wstring name = folderName;
	const WORD wanted = is32bit ? IMAGE_FILE_MACHINE_AMD64 : IMAGE_FILE_MACHINE_I386;

	for (const auto& architecture : s_ArchitectureNames)
	{
		const std::wstring from = architecture[is32bit ? 0 : 1];
		const WCHAR* to = architecture[is32bit ? 1 : 0];

		const size_t pos = StringUtil::CaseInsensitiveFind(name, from);
		if (pos == std::wstring::npos) continue;

		std::wstring candidate = parent;
		candidate += name;
		candidate.replace(parent.length() + pos, from.length(), to);
		candidate += L'\\';
		candidate += fileName;

		WORD machine = 0;
		if (FileUtil::GetBinaryFileBitness(candidate.c_str(), machine) && machine == wanted)
		{
			return candidate;
		}
	}

	return std::wstring();
}

class DialogPackage::SelectPluginDialog : public Dialog
{
public:
	enum Id
	{
		Id_32BitEdit = 1000,
		Id_32BitBrowseButton,
		Id_64BitEdit,
		Id_64BitBrowseButton,
		Id_32BitLabel = 1100,
		Id_64BitLabel
	};

	SelectPluginDialog() : m_Accepted(false)
	{
	}

	std::pair<std::wstring, std::wstring> ShowModal(HWND parent)
	{
		ShowDialogWindow(
			GetString(IDS_Add),
			0, 0, 200, 100,
			DS_CENTER | WS_POPUP | WS_CAPTION | WS_SYSMENU,
			WS_EX_DLGMODALFRAME | WS_EX_CONTROLPARENT,
			parent,
			false);
		return m_Accepted ? m_Plugins : std::pair<std::wstring, std::wstring>();
	}

protected:
	INT_PTR HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override
	{
		const INT_PTR baseResult = Dialog::HandleMessage(uMsg, wParam, lParam);
		switch (uMsg)
		{
		case WM_INITDIALOG:
			return OnInitDialog();
		case WM_COMMAND:
			return OnCommand(wParam, lParam);
		case WM_CLOSE:
			EndDialog(m_Window, 0);
			return TRUE;
		}
		return baseResult;
	}

private:
	INT_PTR OnInitDialog()
	{
		const Control controls[] =
		{
			Control::Label(Id_32BitLabel, IDS_Bit32Dll,
				6, 6, 188, 13,
				WS_VISIBLE, 0),
			Control::Edit(SelectPluginDialog::Id_32BitEdit, 0,
				6, 20, 157, 14,
				WS_VISIBLE | WS_BORDER | ES_READONLY | ES_AUTOHSCROLL, 0),
			Control::Button(SelectPluginDialog::Id_32BitBrowseButton, IDS_Ellipsis,
				168, 20, 25, 14,
				WS_VISIBLE | WS_TABSTOP, 0),
			Control::Label(Id_64BitLabel, IDS_Bit64Dll,
				6, 42, 188, 13,
				WS_VISIBLE, 0),
			Control::Edit(SelectPluginDialog::Id_64BitEdit, 0,
				6, 55, 157, 14,
				WS_VISIBLE | WS_BORDER | ES_READONLY | ES_AUTOHSCROLL, 0),
			Control::Button(SelectPluginDialog::Id_64BitBrowseButton, IDS_Ellipsis,
				168, 55, 25, 14,
				WS_VISIBLE | WS_TABSTOP, 0),
			Control::Button(IDOK, IDS_Add,
				146, 82, 50, 14,
				WS_VISIBLE | WS_TABSTOP | WS_DISABLED | BS_DEFPUSHBUTTON, 0)
		};

		CreateControls(controls, _countof(controls), GetString);
		EnableThemeDialogTexture(m_Window, ETDT_ENABLETAB);
		return TRUE;
	}

	INT_PTR OnCommand(WPARAM wParam, LPARAM lParam)
	{
		switch (LOWORD(wParam))
		{
		case SelectPluginDialog::Id_32BitBrowseButton:
		case SelectPluginDialog::Id_64BitBrowseButton:
			{
				const COMDLG_FILTERSPEC filters[] = { { GetString(IDS_PluginsDll), L"*.dll" } };
				const auto path = ShellDialog::SelectFile({
					.parent = m_Window,
					.title = GetString(IDS_SelectPluginFile),
					.filters = filters,
					.defaultExtension = L"dll"
				});
				if (!path) break;

				const bool x32 = LOWORD(wParam) == SelectPluginDialog::Id_32BitBrowseButton;
				WORD machine = 0;
				if (FileUtil::GetBinaryFileBitness(path->c_str(), machine) &&
					((x32 && machine == IMAGE_FILE_MACHINE_I386) || (!x32 && machine == IMAGE_FILE_MACHINE_AMD64)))
				{
					const WCHAR* otherName = PathFindFileName(x32 ? m_Plugins.second.c_str() : m_Plugins.first.c_str());
					if (*otherName && _wcsicmp(otherName, PathFindFileName(path->c_str())) != 0)
					{
						MessageBox(m_Window, GetString(IDS_PluginsSameName), GetString(IDS_RainmeterSkinPackager), MB_OK | MB_TOPMOST);
						break;
					}

					PathSetDlgItemPath(m_Window, x32 ? SelectPluginDialog::Id_32BitEdit : SelectPluginDialog::Id_64BitEdit, path->c_str());
					(x32 ? m_Plugins.first : m_Plugins.second) = *path;

					std::wstring& other = x32 ? m_Plugins.second : m_Plugins.first;
					if (other.empty())
					{
						other = FindOtherArchitecture(*path, x32);
						if (!other.empty())
						{
							PathSetDlgItemPath(m_Window, x32 ? SelectPluginDialog::Id_64BitEdit : SelectPluginDialog::Id_32BitEdit, other.c_str());
						}
					}

					EnableWindow(GetControl(IDOK), !m_Plugins.first.empty() && !m_Plugins.second.empty());
					break;
				}

				MessageBox(m_Window, GetString(IDS_InvalidPlugin), GetString(IDS_RainmeterSkinPackager), MB_OK | MB_TOPMOST);
			}
			break;

		case IDOK:
			m_Accepted = true;
			EndDialog(m_Window, 0);
			break;

		case IDCANCEL:
			EndDialog(m_Window, 0);
			break;

		default:
			return FALSE;
		}

		return TRUE;
	}

	std::pair<std::wstring, std::wstring> m_Plugins;
	bool m_Accepted;
};

std::wstring DialogPackage::SelectFolder(HWND parent, const std::wstring& existingPath)
{
	SelectFolderDialog dialog(existingPath);
	return dialog.ShowModal(parent);
}

std::pair<std::wstring, std::wstring> DialogPackage::SelectPlugin(HWND parent)
{
	SelectPluginDialog dialog;
	return dialog.ShowModal(parent);
}

// -----------------------------------------------------------------------------------------------
//
//                                Info tab
//
// -----------------------------------------------------------------------------------------------

void DialogPackage::TabInfo::Create(HWND owner)
{
	Tab::CreateTabWindow(15, 15, 290, 238, owner);

	const Control controls[] =
	{
		Control::Label(Id_DescriptionLabel, IDS_PackageInfoDescription,
			0, 0, 284, 26,
			WS_VISIBLE, 0),
		Control::GroupBox(Id_InformationGroup, IDS_Information,
			0, 35, 290, 73,
			WS_VISIBLE, 0),
		Control::Label(Id_NameLabel, IDS_NameColon,
			6, 51, 35, 13,
			WS_VISIBLE, 0),
		Control::Edit(DialogPackage::TabInfo::Id_NameEdit, 0,
			56, 50, 228, 14,
			WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL, 0),
		Control::Label(Id_AuthorLabel, IDS_AuthorColon,
			6, 69, 35, 13,
			WS_VISIBLE, 0),
		Control::Edit(DialogPackage::TabInfo::Id_AuthorEdit, 0,
			56, 68, 228, 14,
			WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL, 0),
		Control::Label(Id_VersionLabel, IDS_VersionColon,
			6, 87, 35, 13,
			WS_VISIBLE, 0),
		Control::Edit(DialogPackage::TabInfo::Id_VersionEdit, 0,
			56, 86, 140, 14,
			WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL, 0),
		Control::GroupBox(Id_ComponentsGroup, IDS_Components,
			0, 113, 290, 108,
			WS_VISIBLE, 0),
		Control::ListView(DialogPackage::TabInfo::Id_ComponentsList, 0,
			6, 128, 192, 86,
			WS_VISIBLE | WS_TABSTOP | WS_BORDER | LVS_REPORT | LVS_SINGLESEL | LVS_NOSORTHEADER, 0),
		Control::Button(DialogPackage::TabInfo::Id_AddSkinButton, IDS_AddSkin,
			204, 128, 80, 14,
			WS_VISIBLE | WS_TABSTOP, 0,
			Control::ELLIPSIS),
		Control::Button(DialogPackage::TabInfo::Id_AddLayoutButton, IDS_AddLayout,
			204, 147, 80, 14,
			WS_VISIBLE | WS_TABSTOP, 0,
			Control::ELLIPSIS),
		Control::Button(DialogPackage::TabInfo::Id_AddPluginButton, IDS_AddPlugin,
			204, 165, 80, 14,
			WS_VISIBLE | WS_TABSTOP, 0,
			Control::ELLIPSIS),
		Control::Button(DialogPackage::TabInfo::Id_RemoveButton, IDS_Remove,
			204, 200, 80, 14,
			WS_VISIBLE | WS_TABSTOP | WS_DISABLED, 0),
		Control::LinkLabel(DialogPackage::TabInfo::Id_WhatIsLink, IDS_WhatIsRmskinPackageLink,
			0, 227, 284, 13,
			WS_VISIBLE | WS_TABSTOP, 0)
	};

	CreateControls(controls, _countof(controls), GetString);
}

void DialogPackage::TabInfo::Initialize()
{
	m_Initialized = true;

	HWND item = GetDlgItem(m_Window, DialogPackage::TabInfo::Id_NameEdit);
	Edit_SetCueBannerText(item, GetString(IDS_Ellipsis));

	item = GetDlgItem(m_Window, DialogPackage::TabInfo::Id_AuthorEdit);
	Edit_SetCueBannerText(item, GetString(IDS_Ellipsis));

	item = GetDlgItem(m_Window, DialogPackage::TabInfo::Id_VersionEdit);
	Edit_SetCueBannerText(item, GetString(IDS_Ellipsis));

	item = GetDlgItem(m_Window, DialogPackage::TabInfo::Id_ComponentsList);

	DWORD extendedFlags = LVS_EX_LABELTIP | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER;
	SetWindowTheme(item, L"explorer", nullptr);
	ListView_EnableGroupView(item, TRUE);
	ListView_SetExtendedListViewStyleEx(item, 0, extendedFlags);

	// Add columns
	LVCOLUMN lvc = { 0 };
	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvc.fmt = LVCFMT_LEFT;
	lvc.iSubItem = 0;
	lvc.cx = 266;
	lvc.pszText = (WCHAR*)GetString(IDS_Name);
	ListView_InsertColumn(item, 0, &lvc);

	// Add groups
	LVGROUP lvg = { 0 };
	lvg.cbSize = sizeof(LVGROUP);
	lvg.mask = LVGF_HEADER | LVGF_GROUPID | LVGF_STATE;
	lvg.state = LVGS_COLLAPSIBLE;
	lvg.iGroupId = 0;
	lvg.pszHeader = (WCHAR*)GetString(IDS_Skin);
	ListView_InsertGroup(item, -1, &lvg);
	lvg.iGroupId = 1;
	lvg.pszHeader = (WCHAR*)GetString(IDS_Layouts);
	ListView_InsertGroup(item, -1, &lvg);
	lvg.iGroupId = 2;
	lvg.pszHeader = (WCHAR*)GetString(IDS_Plugins);
	ListView_InsertGroup(item, -1, &lvg);
}

void DialogPackage::TabInfo::Update()
{
	// The edits write back to the members when their text changes, so the values must be copied
	// before they are set.
	const std::wstring name = c_Dialog->m_Name;
	const std::wstring author = c_Dialog->m_Author;
	const std::wstring version = c_Dialog->m_Version;
	SetDlgItemText(m_Window, DialogPackage::TabInfo::Id_NameEdit, name.c_str());
	SetDlgItemText(m_Window, DialogPackage::TabInfo::Id_AuthorEdit, author.c_str());
	SetDlgItemText(m_Window, DialogPackage::TabInfo::Id_VersionEdit, version.c_str());

	HWND item = GetDlgItem(m_Window, DialogPackage::TabInfo::Id_ComponentsList);
	ListView_DeleteAllItems(item);

	LVITEM lvi = { 0 };
	lvi.mask = LVIF_TEXT | LVIF_GROUPID;
	lvi.iSubItem = 0;

	if (!c_Dialog->m_SkinFolder.first.empty())
	{
		lvi.iGroupId = 0;
		lvi.pszText = (WCHAR*)c_Dialog->m_SkinFolder.first.c_str();
		ListView_InsertItem(item, &lvi);
		++lvi.iItem;
	}

	for (const auto& layout : c_Dialog->m_LayoutFolders)
	{
		lvi.iGroupId = 1;
		lvi.pszText = (WCHAR*)layout.first.c_str();
		ListView_InsertItem(item, &lvi);
		++lvi.iItem;
	}

	for (const auto& plugin : c_Dialog->m_PluginFolders)
	{
		lvi.iGroupId = 2;
		lvi.pszText = (WCHAR*)plugin.first.c_str();
		ListView_InsertItem(item, &lvi);
		++lvi.iItem;
	}

	item = GetDlgItem(m_Window, DialogPackage::TabInfo::Id_AddSkinButton);
	EnableWindow(item, c_Dialog->m_SkinFolder.first.empty());
}

INT_PTR DialogPackage::TabInfo::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_COMMAND:
		return c_Dialog->m_TabInfo.OnCommand(wParam, lParam);

	case WM_NOTIFY:
		return c_Dialog->m_TabInfo.OnNotify(wParam, lParam);
	}

	return FALSE;
}

INT_PTR DialogPackage::TabInfo::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(wParam))
	{
	case DialogPackage::TabInfo::Id_AddSkinButton:
		{
			const std::wstring folder = SelectFolder(m_Window, g_Data.skinsPath);
			if (!folder.empty())
			{
				c_Dialog->SetSkinFolder(folder);
				c_Dialog->UpdateTabs();
				c_Dialog->SetNextButtonState();
			}
		}
		break;

	case DialogPackage::TabInfo::Id_AddLayoutButton:
		{
			std::wstring folder = SelectFolder(m_Window, g_Data.settingsPath + L"Layouts\\");
			if (!folder.empty())
			{
				std::wstring name = PathFindFileName(folder.c_str());
				name.pop_back();	// Remove slash

				if (c_Dialog->m_LayoutFolders.emplace(name, folder).second)
				{
					c_Dialog->UpdateTabs();
				}
			}
		}
		break;

	case DialogPackage::TabInfo::Id_AddPluginButton:
		{
			std::pair<std::wstring, std::wstring> plugins = SelectPlugin(m_Window);
			std::wstring name = PathFindFileName(plugins.first.c_str());
			if (!name.empty() && c_Dialog->m_PluginFolders.emplace(name, plugins).second)
			{
				c_Dialog->UpdateTabs();
			}
		}
		break;

	case DialogPackage::TabInfo::Id_RemoveButton:
		{
			HWND item = GetDlgItem(m_Window, DialogPackage::TabInfo::Id_ComponentsList);
			int sel = ListView_GetNextItem(item, -1, LVNI_FOCUSED | LVNI_SELECTED);
			if (sel != -1)
			{
				WCHAR buffer[MAX_PATH] = { 0 };

				// Remove unchecked items from the component sets
				LVITEM lvi = { 0 };
				lvi.mask = LVIF_GROUPID | LVIF_TEXT;
				lvi.iSubItem = 0;
				lvi.iItem = sel;
				lvi.pszText = buffer;
				lvi.cchTextMax = _countof(buffer);
				ListView_GetItem(item, &lvi);

				const std::wstring name = buffer;
				switch (lvi.iGroupId)
				{
				case 0:
					c_Dialog->m_SkinFolder.first.clear();
					c_Dialog->m_SkinFolder.second.clear();
					break;

				case 1:
					c_Dialog->m_LayoutFolders.erase(name);
					break;

				case 2:
					c_Dialog->m_PluginFolders.erase(name);
					break;
				}

				c_Dialog->UpdateTabs();
				c_Dialog->SetNextButtonState();
			}
		}
		break;

	case DialogPackage::TabInfo::Id_NameEdit:
	case DialogPackage::TabInfo::Id_AuthorEdit:
	case DialogPackage::TabInfo::Id_VersionEdit:
		if (HIWORD(wParam) == EN_CHANGE)
		{
			WCHAR buffer[64] = { 0 };
			int len = GetWindowText((HWND)lParam, buffer, _countof(buffer));
			if (LOWORD(wParam) == DialogPackage::TabInfo::Id_NameEdit)
			{
				c_Dialog->m_Name.assign(buffer, len);
			}
			else if (LOWORD(wParam) == DialogPackage::TabInfo::Id_AuthorEdit)
			{
				c_Dialog->m_Author.assign(buffer, len);
			}
			else // if (LOWORD(wParam) == DialogPackage::TabInfo::Id_VersionEdit)
			{
				c_Dialog->m_Version.assign(buffer, len);
			}
			c_Dialog->SetNextButtonState();
		}
		break;


	default:
		return FALSE;
	}

	return TRUE;
}

INT_PTR DialogPackage::TabInfo::OnNotify(WPARAM wParam, LPARAM lParam)
{
	LPNMHDR nm = (LPNMHDR)lParam;
	switch (nm->code)
	{
	case LVN_ITEMCHANGED:
		{
			NMLISTVIEW* nmlv = (NMLISTVIEW*)lParam;
			if (nm->idFrom == DialogPackage::TabInfo::Id_ComponentsList)
			{
				BOOL selected = (nmlv->uNewState & LVIS_SELECTED);

				HWND item = GetDlgItem(m_Window, DialogPackage::TabInfo::Id_RemoveButton);
				EnableWindow(item, selected);
			}
		}
		break;

	case NM_CLICK:
		{
			if (nm->idFrom == DialogPackage::TabInfo::Id_WhatIsLink)
			{
				c_Dialog->ShowHelp();
			}
		}
		break;

	default:
		return FALSE;
	}

	return TRUE;
}

// -----------------------------------------------------------------------------------------------
//
//                                Options tab
//
// -----------------------------------------------------------------------------------------------

void DialogPackage::TabOptions::Create(HWND owner)
{
	Tab::CreateTabWindow(15, 30, 290, 220, owner);

	const Control controls[] =
	{
		Control::Label(Id_SaveLabel, IDS_SavePackageTo,
			0, 0, 284, 13,
			WS_VISIBLE, 0),
		Control::Edit(DialogPackage::TabOptions::Id_FileEdit, 0,
			0, 17, 260, 14,
			WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_READONLY | ES_AUTOHSCROLL, 0),
		Control::Button(DialogPackage::TabOptions::Id_FileBrowseButton, IDS_Ellipsis,
			265, 17, 25, 14,
			WS_VISIBLE | WS_TABSTOP, 0),
		Control::GroupBox(Id_AfterInstallGroup, IDS_AfterInstallation,
			0, 119, 290, 58,
			WS_VISIBLE, 0),
		Control::RadioButton(DialogPackage::TabOptions::Id_DoNothingRadio, IDS_DoNothing,
			6, 134, 85, 13,
			WS_VISIBLE | WS_TABSTOP | WS_GROUP, 0),
		Control::RadioButton(DialogPackage::TabOptions::Id_LoadSkinRadio, IDS_LoadSkin,
			6, 147, 85, 13,
			WS_VISIBLE | WS_TABSTOP, 0),
		Control::Edit(DialogPackage::TabOptions::Id_LoadSkinEdit, 0,
			96, 144, 158, 14,
			WS_TABSTOP | WS_BORDER | ES_READONLY | ES_AUTOHSCROLL, 0),
		Control::Button(DialogPackage::TabOptions::Id_LoadSkinBrowseButton, IDS_Ellipsis,
			259, 144, 25, 14,
			WS_TABSTOP, 0),
		Control::RadioButton(DialogPackage::TabOptions::Id_LoadLayoutRadio, IDS_LoadLayout,
			6, 160, 85, 13,
			WS_VISIBLE | WS_TABSTOP, 0),
		Control::ComboBox(DialogPackage::TabOptions::Id_LoadLayoutCombo, 0,
			96, 157, 188, 14,
			WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST, 0),
		Control::GroupBox(Id_RequirementsGroup, IDS_MinimumRequirements,
			0, 182, 290, 35,
			WS_VISIBLE | WS_GROUP, 0),
		Control::Label(Id_RainmeterVersionLabel, IDS_RainmeterVersion,
			6, 198, 85, 13,
			WS_VISIBLE, 0),
		Control::Edit(DialogPackage::TabOptions::Id_RainmeterVersionEdit, 0,
			96, 195, 80, 14,
			WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL, 0),
		Control::Label(DialogPackage::TabOptions::Id_CreatingText, IDS_Creating,
			0, 0, 290, 100,
			0, 0),
		Control::ProgressBar(DialogPackage::TabOptions::Id_CreatingBar, 0,
			0, 15, 290, 11,
			PBS_MARQUEE | WS_BORDER, 0)
	};

	CreateControls(controls, _countof(controls), GetString);
}

void DialogPackage::TabOptions::Initialize()
{
	m_Initialized = true;

	HWND item = GetDlgItem(m_Window, DialogPackage::TabOptions::Id_LoadSkinEdit);
	Edit_SetCueBannerText(item, GetString(IDS_SelectSkin));

	Update();
}

void DialogPackage::TabOptions::Update()
{
	if (c_Dialog->m_OutputFolder.empty())
	{
		WCHAR buffer[MAX_PATH] = { 0 };
		SHGetFolderPath(nullptr, CSIDL_DESKTOPDIRECTORY, nullptr, SHGFP_TYPE_CURRENT, buffer);
		c_Dialog->m_OutputFolder = buffer;
		c_Dialog->m_OutputFolder += L'\\';
	}

	std::wstring fileName = c_Dialog->m_Name;
	fileName += L'_';
	fileName += c_Dialog->m_Version;

	// Escape reserved chars
	for (auto& ch : fileName)
	{
		if (wcschr(L"\\/:*?\"<>|", ch))
		{
			ch = L'_';
		}
	}

	c_Dialog->m_TargetFile = c_Dialog->m_OutputFolder + fileName;
	c_Dialog->m_TargetFile += L".rmskin";
	SetDlgItemText(m_Window, DialogPackage::TabOptions::Id_FileEdit, c_Dialog->m_TargetFile.c_str());

	HWND combo = GetDlgItem(m_Window, DialogPackage::TabOptions::Id_LoadLayoutCombo);
	ComboBox_ResetContent(combo);
	for (const auto& layout : c_Dialog->m_LayoutFolders)
	{
		ComboBox_AddString(combo, layout.first.c_str());
	}

	const bool hasLayouts = !c_Dialog->m_LayoutFolders.empty();
	EnableWindow(GetDlgItem(m_Window, DialogPackage::TabOptions::Id_LoadLayoutRadio), hasLayouts);

	if (c_Dialog->m_LoadLayout && !c_Dialog->m_Load.empty())
	{
		// Fall back to the first layout if the selected one is no longer included.
		if (c_Dialog->m_LayoutFolders.find(c_Dialog->m_Load) == c_Dialog->m_LayoutFolders.cend())
		{
			c_Dialog->m_LoadLayout = hasLayouts;
			c_Dialog->m_Load = hasLayouts ? (*c_Dialog->m_LayoutFolders.cbegin()).first : std::wstring();
		}
	}
	else if (!c_Dialog->m_ProfileLoaded && c_Dialog->m_Load.empty() && hasLayouts)
	{
		// A package that comes with a layout loads it by default.
		c_Dialog->m_LoadLayout = true;
		c_Dialog->m_Load = (*c_Dialog->m_LayoutFolders.cbegin()).first;
	}

	const bool loadSkin = !c_Dialog->m_LoadLayout && !c_Dialog->m_Load.empty();
	const bool loadLayout = c_Dialog->m_LoadLayout && !c_Dialog->m_Load.empty();

	Button_SetCheck(GetDlgItem(m_Window, DialogPackage::TabOptions::Id_DoNothingRadio),
		(!loadSkin && !loadLayout) ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(GetDlgItem(m_Window, DialogPackage::TabOptions::Id_LoadSkinRadio),
		loadSkin ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(GetDlgItem(m_Window, DialogPackage::TabOptions::Id_LoadLayoutRadio),
		loadLayout ? BST_CHECKED : BST_UNCHECKED);

	HWND item = GetDlgItem(m_Window, DialogPackage::TabOptions::Id_LoadSkinEdit);
	ShowWindow(item, loadSkin ? SW_SHOWNORMAL : SW_HIDE);
	if (loadSkin) SetWindowText(item, c_Dialog->m_Load.c_str());

	item = GetDlgItem(m_Window, DialogPackage::TabOptions::Id_LoadSkinBrowseButton);
	ShowWindow(item, loadSkin ? SW_SHOWNORMAL : SW_HIDE);

	ShowWindow(combo, loadLayout ? SW_SHOWNORMAL : SW_HIDE);
	if (loadLayout) ComboBox_SelectString(combo, -1, c_Dialog->m_Load.c_str());

	if (c_Dialog->m_MinimumRainmeter.empty())
	{
		c_Dialog->m_MinimumRainmeter = GetCurrentRainmeterVersion();
	}

	// The edit writes back to the member when its text changes, so the value must be copied first.
	const std::wstring minimumRainmeter = c_Dialog->m_MinimumRainmeter;
	SetDlgItemText(m_Window, DialogPackage::TabOptions::Id_RainmeterVersionEdit, minimumRainmeter.c_str());
}

INT_PTR DialogPackage::TabOptions::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_COMMAND:
		return c_Dialog->m_TabOptions.OnCommand(wParam, lParam);
	}

	return FALSE;
}

INT_PTR DialogPackage::TabOptions::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(wParam))
	{
	case DialogPackage::TabOptions::Id_FileBrowseButton:
		{
			WCHAR buffer[MAX_PATH] = { 0 };
			HWND item = GetDlgItem(m_Window, DialogPackage::TabOptions::Id_FileEdit);
			GetWindowText(item, buffer, _countof(buffer));

			const COMDLG_FILTERSPEC filters[] = { { GetString(IDS_SkinPackageRmskin), L"*.rmskin" } };
			const auto path = ShellDialog::SaveFile({
				.parent = c_Dialog->GetWindow(),
				.title = GetString(IDS_SelectSkinPackage),
				.filters = filters,
				.defaultExtension = L"rmskin",
				.initialPath = buffer
			});
			if (path)
			{
				c_Dialog->m_TargetFile = *path;
				c_Dialog->m_OutputFolder = *path;
				c_Dialog->m_OutputFolder.resize(PathFindFileName(path->c_str()) - path->c_str());
				SetWindowText(item, path->c_str());
			}
		}
		break;

	case DialogPackage::TabOptions::Id_DoNothingRadio:
		{
			HWND item = GetDlgItem(m_Window, DialogPackage::TabOptions::Id_LoadSkinEdit);
			ShowWindow(item, SW_HIDE);
			item = GetDlgItem(m_Window, DialogPackage::TabOptions::Id_LoadSkinBrowseButton);
			ShowWindow(item, SW_HIDE);
			item = GetDlgItem(m_Window, DialogPackage::TabOptions::Id_LoadLayoutCombo);
			ShowWindow(item, SW_HIDE);

			c_Dialog->m_Load.clear();
		}
		break;

	case DialogPackage::TabOptions::Id_LoadSkinRadio:
		{
			HWND item = GetDlgItem(m_Window, DialogPackage::TabOptions::Id_LoadSkinEdit);
			ShowWindow(item, SW_SHOWNORMAL);

			WCHAR buffer[MAX_PATH] = { 0 };
			GetWindowText(item, buffer, _countof(buffer));
			c_Dialog->m_Load = buffer;
			c_Dialog->m_LoadLayout = false;

			item = GetDlgItem(m_Window, DialogPackage::TabOptions::Id_LoadSkinBrowseButton);
			ShowWindow(item, SW_SHOWNORMAL);
			item = GetDlgItem(m_Window, DialogPackage::TabOptions::Id_LoadLayoutCombo);
			ShowWindow(item, SW_HIDE);
		}
		break;

	case DialogPackage::TabOptions::Id_LoadLayoutRadio:
		{
			HWND item = GetDlgItem(m_Window, DialogPackage::TabOptions::Id_LoadSkinEdit);
			ShowWindow(item, SW_HIDE);
			item = GetDlgItem(m_Window, DialogPackage::TabOptions::Id_LoadSkinBrowseButton);
			ShowWindow(item, SW_HIDE);
			item = GetDlgItem(m_Window, DialogPackage::TabOptions::Id_LoadLayoutCombo);
			ShowWindow(item, SW_SHOWNORMAL);

			WCHAR buffer[MAX_PATH] = { 0 };
			GetWindowText(item, buffer, _countof(buffer));
			c_Dialog->m_Load = buffer;
			c_Dialog->m_LoadLayout = true;
		}
		break;

	case DialogPackage::TabOptions::Id_LoadLayoutCombo:
		if (HIWORD(wParam) == CBN_SELENDOK)
		{
			WCHAR buffer[MAX_PATH] = { 0 };
			HWND item = GetDlgItem(m_Window, DialogPackage::TabOptions::Id_LoadLayoutCombo);
			GetWindowText(item, buffer, _countof(buffer));
			c_Dialog->m_Load = buffer;
			c_Dialog->m_LoadLayout = true;
		}
		break;

	case DialogPackage::TabOptions::Id_LoadSkinBrowseButton:
		{
			HWND item = GetDlgItem(m_Window, DialogPackage::TabOptions::Id_LoadSkinEdit);

			const std::wstring& skinFolder = c_Dialog->m_SkinFolder.second;
			const COMDLG_FILTERSPEC filters[] = { { GetString(IDS_SkinFiles), L"*.ini" } };
			const auto path = ShellDialog::SelectFile({
				.parent = c_Dialog->GetWindow(),
				.title = GetString(IDS_SelectSkinFile),
				.filters = filters,
				.defaultExtension = L"ini",
				.initialPath = skinFolder.c_str()
			});
			if (path)
			{
				// Make sure user didn't browse to some random folder
				if (_wcsnicmp(skinFolder.c_str(), path->c_str(), skinFolder.length()) == 0)
				{
					// Skip everything before actual skin folder
					const WCHAR* folderPath = path->c_str() + skinFolder.length() - c_Dialog->m_SkinFolder.first.length() - 1;
					SetWindowText(item, folderPath);
					c_Dialog->m_Load = folderPath;
				}
			}
		}
		break;

	case DialogPackage::TabOptions::Id_RainmeterVersionEdit:
		if (HIWORD(wParam) == EN_CHANGE)
		{
			WCHAR buffer[32] = { 0 };
			GetWindowText((HWND)lParam, buffer, _countof(buffer));

			// Get caret position
			DWORD sel = Edit_GetSel((HWND)lParam);

			// Only allow numbers and period
			WCHAR* version = buffer;
			while (*version)
			{
				if (iswdigit(*version) || *version == L'.')
				{
					++version;
				}
				else
				{
					*version = L'\0';
					SetWindowText((HWND)lParam, buffer);

					// Reset caret position
					Edit_SetSel((HWND)lParam, LOWORD(sel), HIWORD(sel));
					break;
				}
			}

			c_Dialog->m_MinimumRainmeter = buffer;
		}
		break;

	default:
		return FALSE;
	}

	return TRUE;
}

// -----------------------------------------------------------------------------------------------
//
//                                Advanced tab
//
// -----------------------------------------------------------------------------------------------

void DialogPackage::TabAdvanced::Create(HWND owner)
{
	Tab::CreateTabWindow(15, 30, 290, 220, owner);

	const Control controls[] =
	{
		Control::Label(Id_HeaderLabel, IDS_HeaderImage,
			0, 3, 85, 13,
			WS_VISIBLE, 0),
		Control::Edit(DialogPackage::TabAdvanced::Id_HeaderEdit, 0,
			90, 0, 170, 14,
			WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_READONLY | ES_AUTOHSCROLL, 0),
		Control::Button(DialogPackage::TabAdvanced::Id_HeaderBrowseButton, IDS_Ellipsis,
			265, 0, 25, 14,
			WS_VISIBLE | WS_TABSTOP, 0),
		Control::Label(Id_VariablesLabel, IDS_VariableFiles,
			0, 24, 85, 13,
			WS_VISIBLE, 0),
		Control::Edit(DialogPackage::TabAdvanced::Id_VariableFilesEdit, 0,
			90, 21, 200, 14,
			WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL, 0),
		Control::CheckBox(DialogPackage::TabAdvanced::Id_MergeSkinsCheck, IDS_MergeSkins,
			0, 42, 85, 13,
			WS_VISIBLE | WS_TABSTOP, 0),
		Control::LinkLabel(DialogPackage::TabAdvanced::Id_HelpLink, IDS_HelpLink,
			0, 210, 284, 13,
			WS_VISIBLE | WS_TABSTOP, 0)
	};

	CreateControls(controls, _countof(controls), GetString);
}

void DialogPackage::TabAdvanced::Initialize()
{
	m_Initialized = true;

	Update();
}

void DialogPackage::TabAdvanced::Update()
{
	SetDlgItemText(m_Window, DialogPackage::TabAdvanced::Id_HeaderEdit, c_Dialog->m_HeaderFile.c_str());

	Button_SetCheck(GetDlgItem(m_Window, DialogPackage::TabAdvanced::Id_MergeSkinsCheck),
		c_Dialog->m_MergeSkins ? BST_CHECKED : BST_UNCHECKED);

	// The edit writes back to the member when its text changes, so the value must be copied first.
	const std::wstring variableFiles = c_Dialog->m_VariableFiles;
	HWND item = GetDlgItem(m_Window, DialogPackage::TabAdvanced::Id_VariableFilesEdit);
	SetWindowText(item, variableFiles.c_str());
	Edit_Enable(item, !c_Dialog->m_MergeSkins);
}

INT_PTR DialogPackage::TabAdvanced::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_COMMAND:
		return c_Dialog->m_TabAdvanced.OnCommand(wParam, lParam);

	case WM_NOTIFY:
		return c_Dialog->m_TabAdvanced.OnNotify(wParam, lParam);
	}

	return FALSE;
}

INT_PTR DialogPackage::TabAdvanced::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(wParam))
	{
	case DialogPackage::TabAdvanced::Id_HeaderBrowseButton:
		{
			WCHAR buffer[MAX_PATH] = { 0 };
			HWND item = GetDlgItem(m_Window, DialogPackage::TabAdvanced::Id_HeaderEdit);
			GetWindowText(item, buffer, _countof(buffer));

			const COMDLG_FILTERSPEC filters[] = { { GetString(IDS_BitmapFiles), L"*.bmp" } };
			const auto path = ShellDialog::SelectFile({
				.parent = c_Dialog->GetWindow(),
				.title = GetString(IDS_SelectHeaderImage),
				.filters = filters,
				.defaultExtension = L"bmp",
				.initialPath = buffer
			});
			if (path)
			{
				// Validate bitmap and make sure size is 400x60
				std::wstring error;
				HBITMAP bitmap = (HBITMAP)LoadImage(nullptr, path->c_str(), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
				if (bitmap)
				{
					BITMAP bm = { 0 };
					GetObject(bitmap, sizeof(bm), &bm);
					if (bm.bmWidth == 400 && bm.bmHeight == 60)
					{
						c_Dialog->m_HeaderFile = *path;
						SetWindowText(item, path->c_str());
						break;
					}
					else
					{
						error = GetFormattedString(IDS_InvalidImageSize, path->c_str());
					}
				}
				else
				{
					error = GetFormattedString(IDS_InvalidBitmap, path->c_str());
				}

				MessageBox(m_Window, error.c_str(), GetString(IDS_RainmeterSkinPackager), MB_OK | MB_ICONERROR);
			}
		}
		break;

	case DialogPackage::TabAdvanced::Id_VariableFilesEdit:
		if (HIWORD(wParam) == EN_CHANGE)
		{
			int length = GetWindowTextLength((HWND)lParam);
			c_Dialog->m_VariableFiles.resize(length);
			GetWindowText((HWND)lParam, &c_Dialog->m_VariableFiles[0], length + 1);
		}
		break;

	case DialogPackage::TabAdvanced::Id_MergeSkinsCheck:
		{
			c_Dialog->m_MergeSkins = !c_Dialog->m_MergeSkins;

			// "Merge skins" not compatible with "Variable files"
			HWND item = GetDlgItem(m_Window, DialogPackage::TabAdvanced::Id_VariableFilesEdit);
			Edit_Enable(item, !c_Dialog->m_MergeSkins);
		}
		break;

	default:
		return FALSE;
	}

	return TRUE;
}

INT_PTR DialogPackage::TabAdvanced::OnNotify(WPARAM wParam, LPARAM lParam)
{
	LPNMHDR nm = (LPNMHDR)lParam;
	switch (nm->code)
	{
	case NM_CLICK:
		{
			if (nm->idFrom == DialogPackage::TabAdvanced::Id_HelpLink)
			{
				c_Dialog->ShowHelp();
			}
		}
		break;

	default:
		return FALSE;
	}

	return TRUE;
}
