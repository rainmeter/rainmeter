// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "Uninstall.h"
#include "../Library/resource.h"

#include <Windows.h>
#include <CommCtrl.h>
#include <ShellAPI.h>
#include <ShlObj.h>
#include <Shlwapi.h>
#include <Strsafe.h>

struct UninstallSettings
{
	bool silent;
	bool elevated;
	bool deleteAll;
	DWORD parentProcessId;
	WCHAR directory[MAX_PATH];
};

static UninstallSettings g_Settings;

using GetLanguageStringFunc = LPCWSTR (*)(UINT id);

static bool StartsWith(const WCHAR* value, const WCHAR* prefix)
{
	return CompareStringOrdinal(value, lstrlen(prefix), prefix, lstrlen(prefix), TRUE) == CSTR_EQUAL;
}

static void ParseCommandLine()
{
	int argc = 0;
	WCHAR** argv = CommandLineToArgvW(GetCommandLine(), &argc);
	for (int index = 1; index < argc; ++index)
	{
		const WCHAR* argument = argv[index];
		if (lstrcmpi(argument, L"/S") == 0)
		{
			g_Settings.silent = true;
		}
		else if (lstrcmpi(argument, L"/ELEVATED") == 0)
		{
			g_Settings.elevated = true;
		}
		else if (StartsWith(argument, L"/DELETEALL="))
		{
			g_Settings.deleteAll = argument[11] == L'1';
		}
		else if (StartsWith(argument, L"/PARENT="))
		{
			g_Settings.parentProcessId = (DWORD)StrToInt(argument + 8);
		}
		else if (StartsWith(argument, L"/D="))
		{
			StringCchCopy(g_Settings.directory, _countof(g_Settings.directory), argument + 3);
		}
	}

	LocalFree(argv);
}

static bool IsAdministrator()
{
	SID_IDENTIFIER_AUTHORITY authority = SECURITY_NT_AUTHORITY;
	PSID administrators = nullptr;
	BOOL member = FALSE;
	if (AllocateAndInitializeSid(&authority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &administrators))
	{
		CheckTokenMembership(nullptr, administrators, &member);
		FreeSid(administrators);
	}

	return member != FALSE;
}

static bool CloseRainmeter()
{
	for (int attempt = 0; attempt < 10; ++attempt)
	{
		HWND rainmeter = FindWindow(L"DummyRainWClass", L"Rainmeter control window");
		if (!rainmeter) return true;

		SendMessage(rainmeter, WM_CLOSE, 0, 0);
		Sleep(500);
	}

	return FindWindow(L"DummyRainWClass", L"Rainmeter control window") == nullptr;
}

static void DeleteTree(const WCHAR* path)
{
	WCHAR from[MAX_PATH + 2];
	StringCchCopy(from, _countof(from), path);
	from[lstrlen(from) + 1] = 0;
	SHFILEOPSTRUCT operation;
	SecureZeroMemory(&operation, sizeof(operation));
	operation.wFunc = FO_DELETE;
	operation.pFrom = from;
	operation.fFlags = FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT;
	SHFileOperation(&operation);
}

static void DeleteRegistryTree(HKEY root, const WCHAR* path)
{
	// NSIS writes installer registry keys to the 32-bit view.
	HKEY key;
	if (RegOpenKeyEx(root, path, 0, KEY_ALL_ACCESS | KEY_WOW64_32KEY, &key) == ERROR_SUCCESS)
	{
		RegDeleteTree(key, nullptr);
		RegCloseKey(key);
	}
	RegDeleteKeyEx(root, path, KEY_WOW64_32KEY, 0);
}

static void TerminateParentProcess()
{
	if (!g_Settings.parentProcessId) return;

	HANDLE process = OpenProcess(PROCESS_TERMINATE | SYNCHRONIZE, FALSE, g_Settings.parentProcessId);
	if (!process) return;

	if (TerminateProcess(process, 0)) WaitForSingleObject(process, INFINITE);
	CloseHandle(process);
}

static void RemoveCurrentUserData()
{
	HKEY key;
	if (RegOpenKeyEx(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &key) == ERROR_SUCCESS)
	{
		RegDeleteValue(key, L"Rainmeter");
		RegCloseKey(key);
	}

	WCHAR shortcut[MAX_PATH];
	SHGetFolderPath(nullptr, CSIDL_STARTUP, nullptr, SHGFP_TYPE_CURRENT, shortcut);
	StringCchCat(shortcut, _countof(shortcut), L"\\Rainmeter.lnk");
	DeleteFile(shortcut);
	SHGetFolderPath(nullptr, CSIDL_DESKTOPDIRECTORY, nullptr, SHGFP_TYPE_CURRENT, shortcut);
	StringCchCat(shortcut, _countof(shortcut), L"\\Rainmeter.lnk");
	DeleteFile(shortcut);

	if (!g_Settings.deleteAll) return;

	WCHAR path[MAX_PATH];
	SHGetFolderPath(nullptr, CSIDL_APPDATA, nullptr, SHGFP_TYPE_CURRENT, path);
	StringCchCat(path, _countof(path), L"\\Rainmeter");
	DeleteTree(path);
	SHGetFolderPath(nullptr, CSIDL_PERSONAL, nullptr, SHGFP_TYPE_CURRENT, path);
	StringCchCat(path, _countof(path), L"\\Rainmeter");
	DeleteTree(path);
}

static bool UninstallElevated()
{
	if (!IsAdministrator()) return false;

	CloseRainmeter();

	DeleteRegistryTree(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Rainmeter");
	DeleteRegistryTree(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Rainmeter");
	DeleteRegistryTree(HKEY_CLASSES_ROOT, L"Rainmeter.SkinInstaller");
	DeleteRegistryTree(HKEY_CLASSES_ROOT, L".rmskin");

	TerminateParentProcess();

	const WCHAR* directories[] = {L"Defaults", L"Languages", L"Plugins", L"Runtime", L"Skins", L"VisualElements", L"Addons", L"Fonts"};
	for (const WCHAR* directory : directories)
	{
		WCHAR path[MAX_PATH];
		FormatString(path, _countof(path), L"%1!s!\\%2!s!", g_Settings.directory, directory);
		DeleteTree(path);
	}

	const WCHAR* files[] = {L"Rainmeter.dll", L"Rainmeter.exe", L"Rainmeter.exe.config", L"Rainmeter.VisualElementsManifest.xml", L"RestartRainmeter.exe",
	                        L"SkinInstaller.exe", L"SkinInstaller.dll", L"uninst.exe"};
	for (const WCHAR* file : files)
	{
		WCHAR path[MAX_PATH];
		FormatString(path, _countof(path), L"%1!s!\\%2!s!", g_Settings.directory, file);
		DeleteFile(path);
	}

	WCHAR programs[MAX_PATH];
	SHGetFolderPath(nullptr, CSIDL_COMMON_PROGRAMS, nullptr, SHGFP_TYPE_CURRENT, programs);
	StringCchCat(programs, _countof(programs), L"\\Rainmeter.lnk");
	DeleteFile(programs);

	RemoveDirectory(g_Settings.directory);
	return true;
}

static bool RunElevated()
{
	WCHAR executable[MAX_PATH];
	GetModuleFileName(nullptr, executable, _countof(executable));
	WCHAR parameters[MAX_PATH * 2];
	FormatString(parameters, _countof(parameters), L"/Uninstall /ELEVATED /DELETEALL=%1!u! /PARENT=%2!u! /D=\"%3!s!\"", g_Settings.deleteAll,
	             g_Settings.parentProcessId, g_Settings.directory);

	SHELLEXECUTEINFO info;
	SecureZeroMemory(&info, sizeof(info));
	info.cbSize = (DWORD)sizeof(info);
	info.fMask = SEE_MASK_NOCLOSEPROCESS;
	info.lpVerb = L"runas";
	info.lpFile = executable;
	info.lpParameters = parameters;
	info.nShow = SW_SHOWNORMAL;
	if (!ShellExecuteEx(&info)) return false;

	WaitForSingleObject(info.hProcess, INFINITE);
	DWORD exitCode = 1;
	GetExitCodeProcess(info.hProcess, &exitCode);
	CloseHandle(info.hProcess);
	return exitCode == 0;
}

struct UninstallDialogData
{
	const TASKDIALOGCONFIG* progressConfig;
	HWND window;
	LONG started;
	LONG completed;
	bool deleteAll;
	bool succeeded;
};

static DWORD WINAPI UninstallThread(void* parameter)
{
	UninstallDialogData* data = (UninstallDialogData*)parameter;
	data->succeeded = RunElevated();
	if (data->succeeded) RemoveCurrentUserData();
	InterlockedExchange(&data->completed, 1);
	SendMessage(data->window, TDM_SET_PROGRESS_BAR_MARQUEE, FALSE, 0);
	SendMessage(data->window, TDM_ENABLE_BUTTON, IDOK, TRUE);
	SendMessage(data->window, TDM_CLICK_BUTTON, IDOK, 0);
	return 0;
}

static HRESULT CALLBACK UninstallDialogCallback(HWND window, UINT notification, WPARAM wParam, LPARAM, LONG_PTR reference)
{
	UninstallDialogData* data = (UninstallDialogData*)reference;
	if (notification == TDN_CREATED)
	{
		data->window = window;
		SendMessage(GetDlgItem(window, IDOK), BCM_SETSHIELD, 0, TRUE);
	}
	else if (notification == TDN_VERIFICATION_CLICKED)
	{
		data->deleteAll = wParam != 0;
	}
	else if (notification == TDN_BUTTON_CLICKED)
	{
		if (wParam != IDOK) return data->started ? S_FALSE : S_OK;
		if (InterlockedCompareExchange(&data->completed, 0, 0)) return S_OK;
		if (InterlockedCompareExchange(&data->started, 1, 0)) return S_FALSE;

		g_Settings.deleteAll = data->deleteAll;
		SendMessage(window, TDM_NAVIGATE_PAGE, 0, (LPARAM)data->progressConfig);
		SendMessage(window, TDM_ENABLE_BUTTON, IDOK, FALSE);
		SendMessage(window, TDM_ENABLE_BUTTON, IDCANCEL, FALSE);
		SendMessage(window, TDM_SET_PROGRESS_BAR_MARQUEE, TRUE, 30);

		HANDLE thread = CreateThread(nullptr, 0, UninstallThread, data, 0, nullptr);
		if (!thread)
		{
			InterlockedExchange(&data->completed, 1);
			return S_OK;
		}
		CloseHandle(thread);
		return S_FALSE;
	}
	return S_OK;
}

static bool RunUninstallDialog()
{
	// Use Rainmeter.dll's language function so its code and dependencies do not bloat Rainmeter.exe.
	WCHAR library[MAX_PATH];
	FormatString(library, _countof(library), L"%1!s!\\Rainmeter.dll", g_Settings.directory);
	HMODULE module = LoadLibrary(library);
	auto getLanguageString = module ? (GetLanguageStringFunc)GetProcAddress(module, MAKEINTRESOURCEA(9)) : nullptr;
	auto getString = [getLanguageString](UINT id, const WCHAR* fallback)
	{
		const WCHAR* value = getLanguageString ? getLanguageString(id) : nullptr;
		return value && *value ? value : fallback;
	};

	const WCHAR* uninstallRainmeter = getString(IDS_UninstallRainmeter, L"Uninstall Rainmeter");
	const WCHAR* uninstallDescription = getString(IDS_UninstallDescription, L"Click Uninstall to start the uninstallation.");
	const WCHAR* uninstallOptions = getString(IDS_UninstallOptions, L"Uninstallation Options");
	const WCHAR* uninstallButton = getString(IDS_Uninstall, L"Uninstall");
	const WCHAR* uninstallSettings = getString(IDS_UninstallSettings, L"Remove all personal skins and settings");
	const WCHAR* uninstalling = getString(IDS_Uninstalling, L"Uninstalling");
	const WCHAR* complete = getString(IDS_UninstallComplete, L"Rainmeter uninstalled");
	const WCHAR* failed = getString(IDS_UninstallFailed, L"Uninstall failed");
	const WCHAR* failedDescription = getString(IDS_UninstallFailedDescription, L"Rainmeter could not be completely removed.");
	if (module) FreeLibrary(module);

	UninstallDialogData data;
	SecureZeroMemory(&data, sizeof(data));

	TASKDIALOG_BUTTON buttons[] = {{IDOK, uninstallButton}};

	TASKDIALOGCONFIG config;
	SecureZeroMemory(&config, sizeof(config));
	config.cbSize = sizeof(config);
	config.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SIZE_TO_CONTENT | TDF_USE_HICON_MAIN;
	config.dwCommonButtons = TDCBF_CANCEL_BUTTON;
	config.pszWindowTitle = uninstallRainmeter;
	config.hMainIcon = LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_RAINMETER));
	config.pszMainInstruction = uninstallRainmeter;
	config.pszContent = uninstallDescription;
	config.cButtons = _countof(buttons);
	config.pButtons = buttons;
	config.nDefaultButton = IDCANCEL;
	config.pszVerificationText = uninstallSettings;
	config.pszExpandedInformation = L" ";
	config.pszExpandedControlText = uninstallOptions;
	config.pszCollapsedControlText = uninstallOptions;
	config.pfCallback = UninstallDialogCallback;
	config.lpCallbackData = (LONG_PTR)&data;

	TASKDIALOGCONFIG progressConfig = config;
	progressConfig.dwFlags |= TDF_SHOW_MARQUEE_PROGRESS_BAR;
	progressConfig.pszMainInstruction = uninstalling;
	progressConfig.pszContent = nullptr;
	progressConfig.pszVerificationText = nullptr;
	progressConfig.pszExpandedInformation = nullptr;
	progressConfig.pszExpandedControlText = nullptr;
	progressConfig.pszCollapsedControlText = nullptr;
	data.progressConfig = &progressConfig;

	BOOL deleteAll = FALSE;
	HRESULT result = TaskDialogIndirect(&config, nullptr, nullptr, &deleteAll);
	if (data.started)
	{
		TaskDialog(nullptr, nullptr, uninstallRainmeter, data.succeeded ? complete : failed, data.succeeded ? nullptr : failedDescription, TDCBF_OK_BUTTON,
		           data.succeeded ? TD_INFORMATION_ICON : TD_ERROR_ICON, nullptr);
	}

	return SUCCEEDED(result) && data.started && data.succeeded;
}

int Uninstall()
{
	ParseCommandLine();
	int result = 1;
	if (g_Settings.directory[0])
	{
		if (g_Settings.elevated)
		{
			result = UninstallElevated() ? 0 : 1;
		}
		else
		{
			if (g_Settings.silent)
			{
				if (RunElevated())
				{
					RemoveCurrentUserData();
					result = 0;
				}
			}
			else if (RunUninstallDialog()) result = 0;
		}
	}

	WCHAR self[MAX_PATH];
	GetModuleFileName(nullptr, self, _countof(self));
	MoveFileEx(self, nullptr, MOVEFILE_DELAY_UNTIL_REBOOT);
	return result;
}
