#include "Archive.h"
#include "resource.h"

#include <windows.h>
#include <commctrl.h>
#include <objbase.h>
#include <olectl.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <shlwapi.h>
#include <strsafe.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <wchar.h>

// Avoid the C++ standard library so the installer can link directly to msvcrt.dll without pulling
// in the UCRT or C++ runtime. The C++ compiler is used only for safer casts and basic language features.
#ifndef RM_UNINSTALLER
#include <brotli/decode.h>
#endif

#include "InstallerLanguages.generated.h"

#ifndef VERSION_FULL_W
#define VERSION_FULL_W L"0.0.0.0"
#endif
#ifndef VERSION_SHORT_W
#define VERSION_SHORT_W L"0.0"
#endif

struct Settings
{
	bool silent;
	bool elevated;
	bool portable;
	bool install64Bit;
	bool autoStartup;
	bool autoStartupSpecified;
	bool restart;
	bool restartSpecified;
	bool deleteSettings;
	bool nonDefaultLanguage;
	bool existingInstallation;
	bool showLanguageDialog;
	bool directorySpecified;
	bool targetExisted;
	bool verifyArchive;
	unsigned short language;
	WCHAR directory[MAX_PATH];
};

static HINSTANCE g_instance;
static Settings g_settings;
static const InstallerLanguage* g_language = &g_languages[0];
static int g_page;
static bool g_installed;
static IPicture* g_wizardImage;

static void CopyString(WCHAR* destination, size_t count, const WCHAR* source)
{
	if (!count) return;
	lstrcpynW(destination, source ? source : L"", static_cast<int>(count));
}

static void FormatString(WCHAR* destination, size_t count, const WCHAR* format, ...)
{
	if (!count) return;

	va_list arguments;
	va_start(arguments, format);
	DWORD length = FormatMessageW(FORMAT_MESSAGE_FROM_STRING, format, 0, 0, destination, static_cast<DWORD>(count), &arguments);
	va_end(arguments);
	if (!length) destination[0] = 0;
}

static bool StartsWith(const WCHAR* value, const WCHAR* prefix)
{
	return CompareStringOrdinal(value, lstrlenW(prefix), prefix, lstrlenW(prefix), TRUE) == CSTR_EQUAL;
}

static const char* GetLanguageString(InstallerString id)
{
	// Language strings are generated as null-separated UTF-8 blobs to avoid a resource lookup table
	// and keep the installer data compact.
	const char* value = g_language->strings;
	for (unsigned short index = 0; index < static_cast<unsigned short>(id); ++index)
		value += lstrlenA(value) + 1;
	return value;
}

static void GetText(InstallerString id, WCHAR* output, size_t count)
{
	WCHAR input[2048];
	MultiByteToWideChar(CP_UTF8, 0, GetLanguageString(id), -1, input, static_cast<int>(_countof(input)));
	output[0] = 0;
	for (const WCHAR* current = input; *current && lstrlenW(output) + 1 < static_cast<int>(count);)
	{
		if (StartsWith(current, L"$\\n"))
		{
			StringCchCatW(output, count, L"\r\n");
			current += 3;
		}
		else if (StartsWith(current, L"${VERSION_SHORT}"))
		{
			StringCchCatW(output, count, VERSION_SHORT_W);
			current += 16;
		}
		else if (StartsWith(current, L"$INSTDIR"))
		{
			StringCchCatW(output, count, g_settings.directory);
			current += 8;
		}
		else
		{
			WCHAR character[2] = {*current++, 0};
			StringCchCatW(output, count, character);
		}
	}
}

static void SetText(HWND window, int control, InstallerString id)
{
	WCHAR value[2048];
	GetText(id, value, _countof(value));
	SetDlgItemTextW(window, control, value);
}

static const InstallerLanguage* FindLanguage(unsigned short lcid)
{
	for (size_t index = 0; index < _countof(g_languages); ++index)
	{
		if (g_languages[index].lcid == lcid) return &g_languages[index];
	}

	return &g_languages[0];
}

static bool ReadRegistryString(HKEY root, const WCHAR* key, const WCHAR* name, WCHAR* value, size_t count)
{
	DWORD bytes = static_cast<DWORD>(count * sizeof(WCHAR));
	return RegGetValueW(root, key, name, RRF_RT_REG_SZ, nullptr, value, &bytes) == ERROR_SUCCESS;
}

static bool Is64BitWindows()
{
#ifdef _WIN64
	return true;
#else
	BOOL wow64 = FALSE;
	return IsWow64Process(GetCurrentProcess(), &wow64) && wow64;
#endif
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

static void ParseCommandLine()
{
	ZeroMemory(&g_settings, sizeof(g_settings));
	g_settings.install64Bit = Is64BitWindows();
	g_settings.language = static_cast<unsigned short>(GetUserDefaultUILanguage());
	g_settings.showLanguageDialog = true;
	WCHAR installedLanguage[16];
	if (ReadRegistryString(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Rainmeter", L"Language", installedLanguage, _countof(installedLanguage)))
	{
		g_settings.language = static_cast<unsigned short>(wcstoul(installedLanguage, nullptr, 10));
		g_settings.showLanguageDialog = false;
		g_settings.existingInstallation = true;
	}

	int argc = 0;
	WCHAR** argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	for (int index = 1; index < argc; ++index)
	{
		const WCHAR* argument = argv[index];
		if (lstrcmpiW(argument, L"/S") == 0)
			g_settings.silent = true;
		else if (lstrcmpiW(argument, L"/ELEVATED") == 0)
			g_settings.elevated = true;
		else if (lstrcmpiW(argument, L"/VERIFYARCHIVE") == 0)
			g_settings.verifyArchive = true;
		else if (StartsWith(argument, L"/LANGUAGE="))
			g_settings.language = static_cast<unsigned short>(wcstoul(argument + 10, nullptr, 10));
		else if (StartsWith(argument, L"/RESTART="))
		{
			g_settings.restartSpecified = true;
			g_settings.restart = argument[9] == L'1';
		}
		else if (StartsWith(argument, L"/AUTOSTARTUP="))
		{
			g_settings.autoStartupSpecified = true;
			g_settings.autoStartup = argument[13] == L'1';
		}
		else if (StartsWith(argument, L"/PORTABLE="))
			g_settings.portable = argument[10] == L'1';
		else if (StartsWith(argument, L"/VERSION="))
			g_settings.install64Bit = argument[9] != L'3' || argument[10] != L'2';
		else if (StartsWith(argument, L"/DELETEALL="))
			g_settings.deleteSettings = argument[11] == L'1';
		else if (StartsWith(argument, L"/D="))
		{
			g_settings.directorySpecified = true;
			CopyString(g_settings.directory, _countof(g_settings.directory), argument + 3);
		}
	}

	LocalFree(argv);
	if (!g_settings.autoStartupSpecified)
	{
		WCHAR startup[MAX_PATH];
		g_settings.autoStartup =
		    ReadRegistryString(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", L"Rainmeter", startup, _countof(startup)) ||
		    !g_settings.existingInstallation;
	}

	if (!g_settings.restartSpecified)
		g_settings.restart = !g_settings.silent;
	g_language = FindLanguage(g_settings.language);
}

static bool IsSupportedWindows()
{
	using RtlGetVersionFunction = LONG(WINAPI*)(OSVERSIONINFOW*);
	HMODULE module = GetModuleHandleW(L"ntdll.dll");
	auto getVersion = reinterpret_cast<RtlGetVersionFunction>(GetProcAddress(module, "RtlGetVersion"));
	OSVERSIONINFOW version = {static_cast<DWORD>(sizeof(version))};
	return getVersion && getVersion(&version) == 0 && version.dwMajorVersion >= 10 && version.dwBuildNumber >= 18362;
}

static void GetDefaultDirectory()
{
	if (g_settings.directory[0] && g_settings.portable) return;
	if (g_settings.portable)
	{
		if (ReadRegistryString(HKEY_CURRENT_USER, L"SOFTWARE\\Rainmeter", L"PortableInstallPath", g_settings.directory, _countof(g_settings.directory)) &&
		    GetFileAttributesW(g_settings.directory) != INVALID_FILE_ATTRIBUTES)
			return;
		GetModuleFileNameW(nullptr, g_settings.directory, _countof(g_settings.directory));
		WCHAR* fileName = wcsrchr(g_settings.directory, L'\\');
		if (fileName)
			*fileName = 0;
		StringCchCatW(g_settings.directory, _countof(g_settings.directory), L"\\Rainmeter");
		return;
	}

	if (ReadRegistryString(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Rainmeter", nullptr, g_settings.directory, _countof(g_settings.directory)))
	{
		g_settings.existingInstallation = true;
		return;
	}

	WCHAR programFiles[MAX_PATH] = {};
	if (!GetEnvironmentVariableW(g_settings.install64Bit ? L"ProgramW6432" : L"ProgramFiles(x86)", programFiles, _countof(programFiles)))
	{
		GetEnvironmentVariableW(L"ProgramFiles", programFiles, _countof(programFiles));
	}

	FormatString(g_settings.directory, _countof(g_settings.directory), L"%1!s!\\Rainmeter", programFiles);
}

static bool CloseRainmeter()
{
	for (int attempt = 0; attempt < 10; ++attempt)
	{
		HWND rainmeter = FindWindowW(L"DummyRainWClass", L"Rainmeter control window");
		if (!rainmeter) return true;
		SendMessageW(rainmeter, WM_CLOSE, 0, 0);
		Sleep(500);
	}

	return FindWindowW(L"DummyRainWClass", L"Rainmeter control window") == nullptr;
}

static void DeleteRelativeFile(const WCHAR* relative)
{
	WCHAR path[MAX_PATH];
	FormatString(path, _countof(path), L"%1!s!\\%2!s!", g_settings.directory, relative);
	DeleteFileW(path);
}

static void MoveRelativeDirectory(const WCHAR* source, const WCHAR* destination)
{
	WCHAR sourcePath[MAX_PATH];
	WCHAR destinationPath[MAX_PATH];
	FormatString(sourcePath, _countof(sourcePath), L"%1!s!\\%2!s!", g_settings.directory, source);
	FormatString(destinationPath, _countof(destinationPath), L"%1!s!\\%2!s!", g_settings.directory, destination);
	MoveFileW(sourcePath, destinationPath);
}

static void PrepareInstallationDirectory()
{
	const WCHAR* obsoleteFiles[] = {L"Rainmeter.chm",
	                                  L"Rainmeter.VisualElementsManifest.xml",
	                                  L"Default.ini",
	                                  L"Launcher.exe",
	                                  L"RestartRainmeter.exe",
	                                  L"SkinInstaller.dll",
	                                  L"Defaults\\Plugins\\FileView.dll",
	                                  L"Defaults\\Plugins\\AudioLevel.dll"};
	for (const WCHAR* file : obsoleteFiles)
		DeleteRelativeFile(file);
	if (g_settings.portable) return;
	WCHAR defaults[MAX_PATH];
	FormatString(defaults, _countof(defaults), L"%1!s!\\Defaults", g_settings.directory);
	CreateDirectoryW(defaults, nullptr);
	MoveRelativeDirectory(L"Skins", L"Defaults\\Skins");
	MoveRelativeDirectory(L"Themes", L"Defaults\\Layouts");
	MoveRelativeDirectory(L"Defaults\\Themes", L"Defaults\\Layouts");
}

static bool CreateDirectoryTree(WCHAR* path)
{
	for (WCHAR* current = path + 3; *current; ++current)
	{
		if (*current != L'\\')
			continue;
		*current = 0;
		CreateDirectoryW(path, nullptr);
		*current = L'\\';
	}

	return CreateDirectoryW(path, nullptr) || GetLastError() == ERROR_ALREADY_EXISTS;
}

static bool ReadExact(HANDLE file, void* data, size_t size)
{
	if (size > MAXDWORD) return false;
	DWORD read = 0;
	return ReadFile(file, data, static_cast<DWORD>(size), &read, nullptr) && read == size;
}

#ifndef RM_UNINSTALLER
static bool ExtractArchive(HWND window, bool verifyOnly = false)
{
	WCHAR executable[MAX_PATH];
	GetModuleFileNameW(nullptr, executable, _countof(executable));
	HANDLE input = CreateFileW(executable, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (input == INVALID_HANDLE_VALUE) return false;

	LARGE_INTEGER length;
	// The Brotli stream is appended to the signed installer stub. Its fixed-size footer records the
	// exact stream location and sizes, which also prevents reading past the executable.
	if (!GetFileSizeEx(input, &length) || length.QuadPart < static_cast<LONGLONG>(sizeof(ArchiveFooter)))
	{
		CloseHandle(input);
		return false;
	}

	LARGE_INTEGER footerPosition;
	footerPosition.QuadPart = length.QuadPart - sizeof(ArchiveFooter);
	if (!SetFilePointerEx(input, footerPosition, nullptr, FILE_BEGIN))
	{
		CloseHandle(input);
		return false;
	}

	ArchiveFooter footer;
	if (!ReadExact(input, &footer, sizeof(footer)) || footer.magic != g_ArchiveFooterMagic || footer.version != g_ArchiveVersion)
	{
		CloseHandle(input);
		return false;
	}

	if (footer.compressedSize > MAXDWORD || footer.uncompressedSize > MAXDWORD || footer.archiveOffset > static_cast<uint64_t>(footerPosition.QuadPart) ||
	    footer.compressedSize != static_cast<uint64_t>(footerPosition.QuadPart) - footer.archiveOffset)
	{
		CloseHandle(input);
		return false;
	}

	uint8_t* compressed = static_cast<uint8_t*>(HeapAlloc(GetProcessHeap(), 0, static_cast<SIZE_T>(footer.compressedSize)));
	uint8_t* archive = static_cast<uint8_t*>(HeapAlloc(GetProcessHeap(), 0, static_cast<SIZE_T>(footer.uncompressedSize)));
	LARGE_INTEGER archivePosition;
	archivePosition.QuadPart = footer.archiveOffset;
	bool decoded = SetFilePointerEx(input, archivePosition, nullptr, FILE_BEGIN) && compressed && archive &&
	               ReadExact(input, compressed, static_cast<size_t>(footer.compressedSize));
	size_t archiveSize = static_cast<size_t>(footer.uncompressedSize);
	if (decoded)
		decoded = BrotliDecoderDecompress(static_cast<size_t>(footer.compressedSize), compressed, &archiveSize, archive) == BROTLI_DECODER_RESULT_SUCCESS;
	CloseHandle(input);
	HeapFree(GetProcessHeap(), 0, compressed);
	if (!decoded || archiveSize != footer.uncompressedSize)
	{
		HeapFree(GetProcessHeap(), 0, archive);
		return false;
	}
	uint8_t* current = archive;
	uint8_t* end = archive + archiveSize;
	ArchiveHeader header;
	if (current + sizeof(header) > end)
	{
		HeapFree(GetProcessHeap(), 0, archive);
		return false;
	}

	CopyMemory(&header, current, sizeof(header));
	current += sizeof(header);
	if (header.magic != g_ArchiveMagic || header.version != g_ArchiveVersion)
	{
		HeapFree(GetProcessHeap(), 0, archive);
		return false;
	}

	bool success = true;
	for (uint32_t index = 0; index < header.fileCount && success; ++index)
	{
		ArchiveEntry entry;
		if (current + sizeof(entry) > end)
		{
			success = false;
			break;
		}

		CopyMemory(&entry, current, sizeof(entry));
		current += sizeof(entry);
		if (!entry.pathLength || entry.pathLength >= MAX_PATH || entry.size > MAXDWORD || static_cast<uint64_t>(end - current) < entry.pathLength ||
		    static_cast<uint64_t>(end - current - entry.pathLength) < entry.size)
		{
			success = false;
			break;
		}

		char pathUtf8[MAX_PATH];
		CopyMemory(pathUtf8, current, entry.pathLength);
		current += entry.pathLength;
		pathUtf8[entry.pathLength] = 0;
		for (uint16_t character = 0; character < entry.pathLength; ++character)
			if (pathUtf8[character] == '/')
				pathUtf8[character] = '\\';

		// Both architectures share one archive. Only common files and files for the selected platform
		// are written, so the other platform never needs a temporary extraction directory.
		bool extract = verifyOnly || entry.architecture == ArchiveArchitectureAny ||
		               (g_settings.install64Bit ? entry.architecture == ArchiveArchitecture64 : entry.architecture == ArchiveArchitecture32);
		if (!verifyOnly && g_settings.portable && (entry.flags & g_ArchiveFlagStandardOnly))
			extract = false;
		if (StrStrIA(pathUtf8, "..\\") || pathUtf8[0] == '\\')
			extract = false;
		if (!extract)
		{
			current += entry.size;
			continue;
		}
		if (verifyOnly)
		{
			current += entry.size;
			continue;
		}

		WCHAR relative[MAX_PATH];
		if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, pathUtf8, -1, relative, static_cast<int>(_countof(relative))))
		{
			success = false;
			break;
		}

		WCHAR destination[MAX_PATH];
		FormatString(destination, _countof(destination), L"%1!s!\\%2!s!", g_settings.directory, relative);
		WCHAR parent[MAX_PATH];
		CopyString(parent, _countof(parent), destination);
		WCHAR* slash = wcsrchr(parent, L'\\');
		if (slash)
		{
			*slash = 0;
			CreateDirectoryTree(parent);
		}

		if (success)
		{
			HANDLE output = CreateFileW(destination, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
			DWORD written = 0;
			success = output != INVALID_HANDLE_VALUE && WriteFile(output, current, static_cast<DWORD>(entry.size), &written, nullptr) &&
			          written == static_cast<DWORD>(entry.size);
			if (output != INVALID_HANDLE_VALUE)
				CloseHandle(output);
		}

		current += entry.size;
		if (window)
			SendDlgItemMessageW(window, IDC_PROGRESS, PBM_SETPOS, (index + 1) * 100 / header.fileCount, 0);
	}

	success = success && current == end;
	HeapFree(GetProcessHeap(), 0, archive);
	return success;
}
#else
static bool ExtractArchive(HWND, bool = false)
{
	return false;
}
#endif

static void WriteRegistryString(HKEY root, const WCHAR* key, const WCHAR* name, const WCHAR* value)
{
	HKEY handle;
	if (RegCreateKeyExW(root, key, 0, nullptr, 0, KEY_SET_VALUE, nullptr, &handle, nullptr) == ERROR_SUCCESS)
	{
		RegSetValueExW(handle, name, 0, REG_SZ, reinterpret_cast<const BYTE*>(value), static_cast<DWORD>((lstrlenW(value) + 1) * sizeof(WCHAR)));
		RegCloseKey(handle);
	}
}

static void WriteRegistryDword(HKEY root, const WCHAR* key, const WCHAR* name, DWORD value)
{
	HKEY handle;
	if (RegCreateKeyExW(root, key, 0, nullptr, 0, KEY_SET_VALUE, nullptr, &handle, nullptr) == ERROR_SUCCESS)
	{
		RegSetValueExW(handle, name, 0, REG_DWORD, reinterpret_cast<const BYTE*>(&value), sizeof(value));
		RegCloseKey(handle);
	}
}

static bool CreateShortcut(const WCHAR* path, const WCHAR* target)
{
	IShellLinkW* link = nullptr;
	if (FAILED(CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&link)))) return false;
	link->SetPath(target);
	link->SetIconLocation(target, 0);
	IPersistFile* file = nullptr;
	HRESULT result = link->QueryInterface(IID_PPV_ARGS(&file));
	if (SUCCEEDED(result))
	{
		result = file->Save(path, TRUE);
		file->Release();
	}

	link->Release();
	return SUCCEEDED(result);
}

static void RegisterInstallation()
{
	const WCHAR* uninstallKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Rainmeter";
	WCHAR executable[MAX_PATH];
	FormatString(executable, _countof(executable), L"%1!s!\\Rainmeter.exe", g_settings.directory);
	WCHAR uninstaller[MAX_PATH];
	FormatString(uninstaller, _countof(uninstaller), L"%1!s!\\uninst.exe", g_settings.directory);
	WriteRegistryString(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Rainmeter", nullptr, g_settings.directory);
	WCHAR language[16];
	FormatString(language, _countof(language), L"%1!u!", g_settings.language);
	WriteRegistryString(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Rainmeter", L"Language", language);
	WriteRegistryDword(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Rainmeter", L"NonDefault", g_settings.nonDefaultLanguage);
	WriteRegistryString(HKEY_LOCAL_MACHINE, uninstallKey, L"Publisher", L"Rainmeter");
	WriteRegistryString(HKEY_LOCAL_MACHINE, uninstallKey, L"Comments", L"Rainmeter - desktop customization tool");
	WriteRegistryString(HKEY_LOCAL_MACHINE, uninstallKey, L"DisplayIcon", executable);
	WriteRegistryString(HKEY_LOCAL_MACHINE, uninstallKey, L"DisplayName", L"Rainmeter");
	WriteRegistryString(HKEY_LOCAL_MACHINE, uninstallKey, L"DisplayVersion", VERSION_SHORT_W);
	WriteRegistryString(HKEY_LOCAL_MACHINE, uninstallKey, L"InstallLocation", g_settings.directory);
	WriteRegistryString(HKEY_LOCAL_MACHINE, uninstallKey, L"UninstallString", uninstaller);
	WriteRegistryString(HKEY_LOCAL_MACHINE, uninstallKey, L"HelpLink", L"https://docs.rainmeter.net/manual/");
	WriteRegistryString(HKEY_LOCAL_MACHINE, uninstallKey, L"URLInfoAbout", L"https://rainmeter.net");
	WriteRegistryString(HKEY_LOCAL_MACHINE, uninstallKey, L"URLUpdateInfo", L"https://rainmeter.net");
	WriteRegistryString(HKEY_LOCAL_MACHINE, uninstallKey, L"ReleaseType", L"Release");
	WriteRegistryDword(HKEY_LOCAL_MACHINE, uninstallKey, L"NoModify", 1);
	WriteRegistryDword(HKEY_LOCAL_MACHINE, uninstallKey, L"NoRepair", 1);
	WriteRegistryString(HKEY_CLASSES_ROOT, L".rmskin", nullptr, L"Rainmeter.SkinInstaller");
	WriteRegistryString(HKEY_CLASSES_ROOT, L"Rainmeter.SkinInstaller", nullptr, L"Rainmeter Skin Installer");
	WriteRegistryString(HKEY_CLASSES_ROOT, L"Rainmeter.SkinInstaller", L"FriendlyTypeName", L"Rainmeter Skin Package");
	WCHAR skinInstaller[MAX_PATH * 2];
	FormatString(skinInstaller, _countof(skinInstaller), L"%1!s!\\SkinInstaller.exe,0", g_settings.directory);
	WriteRegistryString(HKEY_CLASSES_ROOT, L"Rainmeter.SkinInstaller\\DefaultIcon", nullptr, skinInstaller);
	WriteRegistryString(HKEY_CLASSES_ROOT, L"Rainmeter.SkinInstaller\\shell", nullptr, L"open");
	WCHAR command[MAX_PATH * 2];
	StringCchCopyW(command, _countof(command), L"\"");
	StringCchCatW(command, _countof(command), g_settings.directory);
	StringCchCatW(command, _countof(command), L"\\SkinInstaller.exe\" \"%1\"");
	WriteRegistryString(HKEY_CLASSES_ROOT, L"Rainmeter.SkinInstaller\\shell\\open\\command", nullptr, command);
	WriteRegistryString(HKEY_CLASSES_ROOT, L"Rainmeter.SkinInstaller\\shell\\edit", nullptr, L"Install Rainmeter skin");
	WriteRegistryString(HKEY_CLASSES_ROOT, L"Rainmeter.SkinInstaller\\shell\\edit\\command", nullptr, command);

	WCHAR programs[MAX_PATH];
	SHGetFolderPathW(nullptr, CSIDL_COMMON_PROGRAMS, nullptr, SHGFP_TYPE_CURRENT, programs);
	WCHAR shortcut[MAX_PATH];
	FormatString(shortcut, _countof(shortcut), L"%1!s!\\Rainmeter.lnk", programs);
	CreateShortcut(shortcut, executable);
}

static void UpdateCurrentUser()
{
	// This runs in the original unelevated process. Per-user registry changes and the final launch
	// must not inherit the administrator account used for elevation.
	WCHAR executable[MAX_PATH];
	FormatString(executable, _countof(executable), L"%1!s!\\Rainmeter.exe", g_settings.directory);
	if (g_settings.portable)
	{
		WriteRegistryString(HKEY_CURRENT_USER, L"SOFTWARE\\Rainmeter", L"PortableInstallPath", g_settings.directory);
		WCHAR ini[MAX_PATH];
		FormatString(ini, _countof(ini), L"%1!s!\\Rainmeter.ini", g_settings.directory);
		if (GetFileAttributesW(ini) == INVALID_FILE_ATTRIBUTES && !g_settings.targetExisted)
		{
			WCHAR source[MAX_PATH];
			FormatString(source, _countof(source), L"%1!s!\\Defaults\\Layouts\\illustro default\\Rainmeter.ini", g_settings.directory);
			CopyFileW(source, ini, TRUE);
		}

		WCHAR language[16];
		FormatString(language, _countof(language), L"%1!u!", g_settings.language);
		WritePrivateProfileStringW(L"Rainmeter", L"Language", language, ini);
		return;
	}

	if (g_settings.autoStartup)
		WriteRegistryString(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", L"Rainmeter", executable);
	else
	{
		HKEY key;
		if (RegOpenKeyExW(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &key) == ERROR_SUCCESS)
		{
			RegDeleteValueW(key, L"Rainmeter");
			RegCloseKey(key);
		}
	}
}

static bool Install(HWND window)
{
	if (!g_settings.portable && !IsAdministrator()) return false;
	if (!CloseRainmeter()) return false;
	if (!CreateDirectoryTree(g_settings.directory)) return false;
	WCHAR existingExecutable[MAX_PATH];
	FormatString(existingExecutable, _countof(existingExecutable), L"%1!s!\\Rainmeter.exe", g_settings.directory);
	g_settings.targetExisted = GetFileAttributesW(existingExecutable) != INVALID_FILE_ATTRIBUTES;
	PrepareInstallationDirectory();
	if (!ExtractArchive(window)) return false;
	if (!g_settings.portable)
		RegisterInstallation();
	return true;
}

static void BuildElevatedParameters(WCHAR* parameters, size_t count, bool uninstall)
{
	if (uninstall)
	{
		FormatString(parameters, count, L"/ELEVATED /DELETEALL=%1!u! /D=\"%2!s!\"", g_settings.deleteSettings, g_settings.directory);
	}
	else
	{
		FormatString(parameters, count, L"/ELEVATED /S /PORTABLE=%1!u! /VERSION=%2!u! /AUTOSTARTUP=%3!u! /LANGUAGE=%4!u! /RESTART=0 /D=\"%5!s!\"", g_settings.portable,
		         g_settings.install64Bit ? 64 : 32, g_settings.autoStartup, g_settings.language, g_settings.directory);
	}

	parameters[count - 1] = 0;
}

static bool RunElevated(bool uninstall)
{
	WCHAR executable[MAX_PATH];
	GetModuleFileNameW(nullptr, executable, _countof(executable));
	WCHAR elevatedExecutable[MAX_PATH];
	CopyString(elevatedExecutable, _countof(elevatedExecutable), executable);
	if (uninstall)
	{
		// Run a temporary copy so the elevated process can remove the installed uninstaller along with
		// the rest of the installation.
		WCHAR temporary[MAX_PATH];
		GetTempPathW(_countof(temporary), temporary);
		FormatString(elevatedExecutable, _countof(elevatedExecutable), L"%1!s!Rainmeter-uninst-%2!u!.exe", temporary, GetCurrentProcessId());
		if (!CopyFileW(executable, elevatedExecutable, FALSE)) return false;
	}

	WCHAR parameters[MAX_PATH * 2];
	BuildElevatedParameters(parameters, _countof(parameters), uninstall);
	SHELLEXECUTEINFOW info = {static_cast<DWORD>(sizeof(info))};
	info.fMask = SEE_MASK_NOCLOSEPROCESS;
	info.lpVerb = L"runas";
	info.lpFile = elevatedExecutable;
	info.lpParameters = parameters;
	info.nShow = SW_SHOWNORMAL;
	if (!ShellExecuteExW(&info))
	{
		if (uninstall)
			DeleteFileW(elevatedExecutable);
		return false;
	}

	WaitForSingleObject(info.hProcess, INFINITE);
	DWORD exitCode = 1;
	GetExitCodeProcess(info.hProcess, &exitCode);
	CloseHandle(info.hProcess);
	if (uninstall)
		DeleteFileW(elevatedExecutable);
	return exitCode == 0;
}

static INT_PTR CALLBACK LanguageDialogProc(HWND window, UINT message, WPARAM wParam, LPARAM)
{
	if (message == WM_INITDIALOG)
	{
		int selected = 0;
		for (size_t index = 0; index < _countof(g_languages); ++index)
		{
			WCHAR name[128];
			MultiByteToWideChar(CP_UTF8, 0, g_languages[index].name, -1, name, static_cast<int>(_countof(name)));
			SendDlgItemMessageW(window, IDC_LANGUAGE, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(name));
			if (g_languages[index].lcid == g_settings.language)
				selected = static_cast<int>(index);
		}
		SendDlgItemMessageW(window, IDC_LANGUAGE, CB_SETCURSEL, selected, 0);
		return TRUE;
	}
	if (message == WM_COMMAND && LOWORD(wParam) == IDOK)
	{
		int selected = static_cast<int>(SendDlgItemMessageW(window, IDC_LANGUAGE, CB_GETCURSEL, 0, 0));
		g_language = &g_languages[selected < 0 ? 0 : selected];
		g_settings.nonDefaultLanguage = g_language->lcid != g_settings.language;
		g_settings.language = g_language->lcid;
		SetProcessDefaultLayout(g_language->rtl ? LAYOUT_RTL : 0);
		EndDialog(window, IDOK);
		return TRUE;
	}
	if (message == WM_CLOSE || (message == WM_COMMAND && LOWORD(wParam) == IDCANCEL))
	{
		EndDialog(window, IDCANCEL);
		return TRUE;
	}
	return FALSE;
}

static void LoadWizardImage()
{
	HRSRC resource = FindResourceW(g_instance, MAKEINTRESOURCEW(IDR_WIZARD_IMAGE), RT_RCDATA);
	if (!resource) return;
	DWORD size = SizeofResource(g_instance, resource);
	HGLOBAL data = GlobalAlloc(GMEM_MOVEABLE, size);
	if (!data) return;
	CopyMemory(GlobalLock(data), LockResource(LoadResource(g_instance, resource)), size);
	GlobalUnlock(data);
	IStream* stream = nullptr;
	if (SUCCEEDED(CreateStreamOnHGlobal(data, TRUE, &stream)))
	{
		OleLoadPicture(stream, static_cast<LONG>(size), FALSE, IID_IPicture, reinterpret_cast<void**>(&g_wizardImage));
		stream->Release();
	}
	else
	{
		GlobalFree(data);
	}
}

static void PaintWizardImage(HWND window)
{
	if (!g_wizardImage || g_page != 0) return;
	PAINTSTRUCT paint;
	HDC device = BeginPaint(window, &paint);
	LONG width = 0;
	LONG height = 0;
	g_wizardImage->get_Width(&width);
	g_wizardImage->get_Height(&height);
	g_wizardImage->Render(device, 0, 0, 165, 290, 0, height, width, -height, &paint.rcPaint);
	EndPaint(window, &paint);
}

static void ShowWizardPage(HWND window, int page)
{
	g_page = page;
	const int welcome[] = {IDC_STANDARD, IDC_STANDARD_DESCRIPTION, IDC_PORTABLE, IDC_PORTABLE_DESCRIPTION, IDC_VERSION};
	const int options[] = {IDC_DIRECTORY_GROUP, IDC_DIRECTORY, IDC_BROWSE, IDC_OPTIONS_GROUP, IDC_64BIT, IDC_STARTUP};
	const int progress[] = {IDC_PROGRESS, IDC_STATUS};
	for (int control : welcome)
		ShowWindow(GetDlgItem(window, control), page == 0 ? SW_SHOW : SW_HIDE);
	for (int control : options)
		ShowWindow(GetDlgItem(window, control), page == 1 ? SW_SHOW : SW_HIDE);
	for (int control : progress)
		ShowWindow(GetDlgItem(window, control), page == 2 ? SW_SHOW : SW_HIDE);
	ShowWindow(GetDlgItem(window, IDC_RUN), page == 3 ? SW_SHOW : SW_HIDE);
	ShowWindow(GetDlgItem(window, IDC_BACK), page == 1 ? SW_SHOW : SW_HIDE);
	ShowWindow(GetDlgItem(window, IDCANCEL), page < 2 ? SW_SHOW : SW_HIDE);
	ShowWindow(GetDlgItem(window, IDC_NEXT), page == 2 ? SW_HIDE : SW_SHOW);

	if (page == 0)
	{
		SetWindowTextW(GetDlgItem(window, IDC_PAGE_TITLE), L"Welcome to Rainmeter Setup");
		SetWindowTextW(GetDlgItem(window, IDC_PAGE_SUBTITLE), L"Select the type of install:");
		SetText(window, IDC_STANDARD, InstallerString::StandardInstall);
		SetText(window, IDC_STANDARD_DESCRIPTION, InstallerString::StandardInstallDescription);
		SetText(window, IDC_PORTABLE, InstallerString::PortableInstall);
		SetText(window, IDC_PORTABLE_DESCRIPTION, InstallerString::PortableInstallDescription);
		WCHAR version[64];
		FormatString(version, _countof(version), L"v%1!s!", VERSION_FULL_W);
		SetDlgItemTextW(window, IDC_VERSION, version);
		CheckRadioButton(window, IDC_STANDARD, IDC_PORTABLE, g_settings.portable ? IDC_PORTABLE : IDC_STANDARD);
		SetDlgItemTextW(window, IDC_NEXT, L"Next >");
	}
	else if (page == 1)
	{
		SetText(window, IDC_PAGE_TITLE, InstallerString::InstallOptions);
		SetText(window, IDC_PAGE_SUBTITLE, InstallerString::InstallOptionsDescription);
		SetText(window, IDC_OPTIONS_GROUP, InstallerString::AdditionalOptions);
		SetText(window, IDC_64BIT, InstallerString::Install64Bit);
		SetText(window, IDC_STARTUP, InstallerString::AutoStartup);
		SetDlgItemTextW(window, IDC_DIRECTORY, g_settings.directory);
		if (g_settings.existingInstallation && !g_settings.portable)
		{
			WCHAR executable[MAX_PATH];
			FormatString(executable, _countof(executable), L"%1!s!\\Rainmeter.exe", g_settings.directory);
			DWORD binaryType;
			if (GetBinaryTypeW(executable, &binaryType))
				g_settings.install64Bit = binaryType == SCS_64BIT_BINARY;
		}
		EnableWindow(GetDlgItem(window, IDC_DIRECTORY), !g_settings.existingInstallation || g_settings.portable);
		EnableWindow(GetDlgItem(window, IDC_BROWSE), !g_settings.existingInstallation || g_settings.portable);
		CheckDlgButton(window, IDC_64BIT, g_settings.install64Bit ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(window, IDC_STARTUP, g_settings.autoStartup ? BST_CHECKED : BST_UNCHECKED);
		EnableWindow(GetDlgItem(window, IDC_64BIT), Is64BitWindows());
		if (g_settings.existingInstallation && !g_settings.portable)
			EnableWindow(GetDlgItem(window, IDC_64BIT), FALSE);
		SetDlgItemTextW(window, IDC_NEXT, L"Install");
		SendDlgItemMessageW(window, IDC_NEXT, BCM_SETSHIELD, 0, g_settings.portable ? 0 : 1);
	}
	else if (page == 2)
	{
		SetWindowTextW(GetDlgItem(window, IDC_PAGE_TITLE), L"Installing Rainmeter");
		SetWindowTextW(GetDlgItem(window, IDC_PAGE_SUBTITLE), L"Please wait while Rainmeter is installed.");
		SetWindowTextW(GetDlgItem(window, IDC_STATUS), L"Extracting files...");
	}
	else
	{
		SetWindowTextW(GetDlgItem(window, IDC_PAGE_TITLE), L"Rainmeter Setup Complete");
		SetWindowTextW(GetDlgItem(window, IDC_PAGE_SUBTITLE), L"Rainmeter has been installed on your computer.");
		CheckDlgButton(window, IDC_RUN, g_settings.restart ? BST_CHECKED : BST_UNCHECKED);
		SetDlgItemTextW(window, IDC_NEXT, L"Finish");
	}
	InvalidateRect(window, nullptr, TRUE);
}

static void BrowseForDirectory(HWND window)
{
	BROWSEINFOW info = {};
	info.hwndOwner = window;
	info.lpszTitle = L"Select the folder in which to install Rainmeter:";
	info.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
	PIDLIST_ABSOLUTE item = SHBrowseForFolderW(&info);
	if (item)
	{
		WCHAR path[MAX_PATH];
		if (SHGetPathFromIDListW(item, path))
			SetDlgItemTextW(window, IDC_DIRECTORY, path);
		CoTaskMemFree(item);
	}
}

static INT_PTR CALLBACK WizardDialogProc(HWND window, UINT message, WPARAM wParam, LPARAM)
{
	if (message == WM_INITDIALOG)
	{
		GetDefaultDirectory();
		LoadWizardImage();
		ShowWizardPage(window, 0);
		return TRUE;
	}
	if (message == WM_PAINT && g_page == 0)
	{
		PaintWizardImage(window);
		return TRUE;
	}
	if (message == WM_COMMAND)
	{
		int control = LOWORD(wParam);
		if (control == IDCANCEL)
		{
			EndDialog(window, IDCANCEL);
			return TRUE;
		}
		if (control == IDC_BACK)
		{
			ShowWizardPage(window, 0);
			return TRUE;
		}
		if (control == IDC_BROWSE)
		{
			BrowseForDirectory(window);
			return TRUE;
		}
		if (control == IDC_NEXT)
		{
			if (g_page == 0)
			{
				g_settings.portable = IsDlgButtonChecked(window, IDC_PORTABLE) == BST_CHECKED;
				if (!g_settings.directorySpecified)
					g_settings.directory[0] = 0;
				GetDefaultDirectory();
				ShowWizardPage(window, 1);
			}
			else if (g_page == 1)
			{
				GetDlgItemTextW(window, IDC_DIRECTORY, g_settings.directory, static_cast<int>(_countof(g_settings.directory)));
				g_settings.install64Bit = IsDlgButtonChecked(window, IDC_64BIT) == BST_CHECKED;
				g_settings.autoStartup = IsDlgButtonChecked(window, IDC_STARTUP) == BST_CHECKED;
				ShowWizardPage(window, 2);
				UpdateWindow(window);
				g_installed = g_settings.portable || IsAdministrator() ? Install(window) : RunElevated(false);
				if (g_installed)
				{
					UpdateCurrentUser();
					ShowWizardPage(window, 3);
				}
				else
				{
					MessageBoxW(window, L"Rainmeter could not be installed.", L"Rainmeter Setup", MB_OK | MB_ICONERROR);
					EndDialog(window, IDCANCEL);
				}
			}
			else if (g_page == 3)
			{
				g_settings.restart = IsDlgButtonChecked(window, IDC_RUN) == BST_CHECKED;
				EndDialog(window, IDOK);
			}
			return TRUE;
		}
	}
	if (message == WM_CLOSE)
	{
		if (g_wizardImage)
		{
			g_wizardImage->Release();
			g_wizardImage = nullptr;
		}
		EndDialog(window, IDCANCEL);
		return TRUE;
	}
	return FALSE;
}

static void DeleteTree(const WCHAR* path)
{
	WCHAR from[MAX_PATH + 2];
	CopyString(from, _countof(from), path);
	from[lstrlenW(from) + 1] = 0;
	SHFILEOPSTRUCTW operation = {};
	operation.wFunc = FO_DELETE;
	operation.pFrom = from;
	operation.fFlags = FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT;
	SHFileOperationW(&operation);
}

static void RemoveCurrentUserData()
{
	HKEY key;
	if (RegOpenKeyExW(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &key) == ERROR_SUCCESS)
	{
		RegDeleteValueW(key, L"Rainmeter");
		RegCloseKey(key);
	}
	WCHAR shortcut[MAX_PATH];
	SHGetFolderPathW(nullptr, CSIDL_STARTUP, nullptr, SHGFP_TYPE_CURRENT, shortcut);
	StringCchCatW(shortcut, _countof(shortcut), L"\\Rainmeter.lnk");
	DeleteFileW(shortcut);
	SHGetFolderPathW(nullptr, CSIDL_DESKTOPDIRECTORY, nullptr, SHGFP_TYPE_CURRENT, shortcut);
	StringCchCatW(shortcut, _countof(shortcut), L"\\Rainmeter.lnk");
	DeleteFileW(shortcut);
	if (!g_settings.deleteSettings) return;
	WCHAR path[MAX_PATH];
	SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, SHGFP_TYPE_CURRENT, path);
	StringCchCatW(path, _countof(path), L"\\Rainmeter");
	DeleteTree(path);
	SHGetFolderPathW(nullptr, CSIDL_PERSONAL, nullptr, SHGFP_TYPE_CURRENT, path);
	StringCchCatW(path, _countof(path), L"\\Rainmeter");
	DeleteTree(path);
}

static bool UninstallElevated()
{
	if (!IsAdministrator()) return false;
	CloseRainmeter();
	const WCHAR* directories[] = {L"Defaults", L"Languages", L"Plugins", L"Runtime", L"Skins", L"VisualElements", L"Addons", L"Fonts"};
	for (const WCHAR* directory : directories)
	{
		WCHAR path[MAX_PATH];
		FormatString(path, _countof(path), L"%1!s!\\%2!s!", g_settings.directory, directory);
		DeleteTree(path);
	}
	const WCHAR* files[] = {L"Rainmeter.dll", L"Rainmeter.exe", L"Rainmeter.exe.config", L"SkinInstaller.exe", L"uninst.exe"};
	for (const WCHAR* file : files)
	{
		WCHAR path[MAX_PATH];
		FormatString(path, _countof(path), L"%1!s!\\%2!s!", g_settings.directory, file);
		DeleteFileW(path);
	}
	RegDeleteTreeW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Rainmeter");
	RegDeleteTreeW(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Rainmeter");
	RegDeleteTreeW(HKEY_CLASSES_ROOT, L"Rainmeter.SkinInstaller");
	RegDeleteTreeW(HKEY_CLASSES_ROOT, L".rmskin");
	WCHAR programs[MAX_PATH];
	SHGetFolderPathW(nullptr, CSIDL_COMMON_PROGRAMS, nullptr, SHGFP_TYPE_CURRENT, programs);
	StringCchCatW(programs, _countof(programs), L"\\Rainmeter.lnk");
	DeleteFileW(programs);
	WCHAR self[MAX_PATH];
	GetModuleFileNameW(nullptr, self, _countof(self));
	MoveFileExW(self, nullptr, MOVEFILE_DELAY_UNTIL_REBOOT);
	RemoveDirectoryW(g_settings.directory);
	return true;
}

static INT_PTR CALLBACK UninstallDialogProc(HWND window, UINT message, WPARAM wParam, LPARAM)
{
	if (message == WM_INITDIALOG)
	{
		SetText(window, IDC_PAGE_TITLE, InstallerString::UninstallOptions);
		SetText(window, IDC_UNINSTALL_RAINMETER, InstallerString::UninstallRainmeter);
		SetText(window, IDC_DELETE_SETTINGS, InstallerString::UninstallSettings);
		SetText(window, IDC_DELETE_DESCRIPTION, InstallerString::UninstallSettingsDescription);
		CheckDlgButton(window, IDC_UNINSTALL_RAINMETER, BST_CHECKED);
		return TRUE;
	}
	if (message == WM_COMMAND && LOWORD(wParam) == IDOK)
	{
		g_settings.deleteSettings = IsDlgButtonChecked(window, IDC_DELETE_SETTINGS) == BST_CHECKED;
		EndDialog(window, IDOK);
		return TRUE;
	}
	if (message == WM_CLOSE || (message == WM_COMMAND && LOWORD(wParam) == IDCANCEL))
	{
		EndDialog(window, IDCANCEL);
		return TRUE;
	}
	return FALSE;
}

static int RunInstaller()
{
	if (!IsSupportedWindows())
	{
		WCHAR error[1024];
		GetText(InstallerString::UnsupportedWindowsError, error, _countof(error));
		MessageBoxW(nullptr, error, L"Rainmeter Setup", MB_OK | MB_ICONERROR);
		return 3;
	}
	if (g_settings.elevated) return Install(nullptr) ? 0 : 1;
	if (!g_settings.restartSpecified && FindWindowW(L"DummyRainWClass", L"Rainmeter control window"))
		g_settings.restart = true;
	SetProcessDefaultLayout(g_language->rtl ? LAYOUT_RTL : 0);
	if (!g_settings.silent && g_settings.showLanguageDialog &&
	    DialogBoxParamW(g_instance, MAKEINTRESOURCEW(IDD_LANGUAGE), nullptr, LanguageDialogProc, 0) != IDOK)
		return 1;
	if (g_settings.silent)
	{
		GetDefaultDirectory();
		bool installed = g_settings.portable || IsAdministrator() ? Install(nullptr) : RunElevated(false);
		if (!installed) return 1;
		UpdateCurrentUser();
		g_installed = true;
	}
	else if (DialogBoxParamW(g_instance, MAKEINTRESOURCEW(IDD_WIZARD), nullptr, WizardDialogProc, 0) != IDOK)
		return 1;
	if (g_wizardImage)
	{
		g_wizardImage->Release();
		g_wizardImage = nullptr;
	}
	if (g_installed && g_settings.restart)
	{
		WCHAR executable[MAX_PATH];
		FormatString(executable, _countof(executable), L"%1!s!\\Rainmeter.exe", g_settings.directory);
		ShellExecuteW(nullptr, nullptr, executable, nullptr, g_settings.directory, SW_SHOWNORMAL);
	}
	return 0;
}

static int RunUninstaller()
{
	if (!g_settings.directory[0])
	{
		WCHAR executable[MAX_PATH];
		GetModuleFileNameW(nullptr, executable, _countof(executable));
		CopyString(g_settings.directory, _countof(g_settings.directory), executable);
		WCHAR* slash = wcsrchr(g_settings.directory, L'\\');
		if (slash)
			*slash = 0;
	}
	unsigned short language = 0;
	WCHAR value[16];
	if (ReadRegistryString(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Rainmeter", L"Language", value, _countof(value)))
		language = static_cast<unsigned short>(wcstoul(value, nullptr, 10));
	if (language)
		g_language = FindLanguage(language);
	SetProcessDefaultLayout(g_language->rtl ? LAYOUT_RTL : 0);
	if (g_settings.elevated) return UninstallElevated() ? 0 : 1;
	if (!g_settings.silent && DialogBoxParamW(g_instance, MAKEINTRESOURCEW(IDD_UNINSTALL), nullptr, UninstallDialogProc, 0) != IDOK) return 1;
	if (!RunElevated(true)) return 1;
	RemoveCurrentUserData();
	return 0;
}

extern "C" void __cdecl wWinMainCRTStartup()
{
	g_instance = GetModuleHandleW(nullptr);
	INITCOMMONCONTROLSEX controls = {static_cast<DWORD>(sizeof(controls)), ICC_PROGRESS_CLASS};
	InitCommonControlsEx(&controls);
	CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
	ParseCommandLine();
#ifdef RM_UNINSTALLER
	int result = RunUninstaller();
#else
	int result = g_settings.verifyArchive ? (ExtractArchive(nullptr, true) ? 0 : 1) : RunInstaller();
#endif
	CoUninitialize();
	ExitProcess(result);
}
