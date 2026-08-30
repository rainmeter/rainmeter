// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include <Windows.h>
#include <ShObjIdl.h>
#include <optional>
#include <span>
#include <string>

// Wrappers around the Vista+ common item dialog (IFileDialog). All functions return the selected
// path, or an empty optional if the dialog could not be created or the user cancelled it.
namespace ShellDialog {

struct Options
{
	HWND parent = nullptr;
	const WCHAR* title = nullptr;

	// Name/pattern pairs, e.g. { { L"Bitmap files", L"*.bmp" } }. Ignored by SelectFolder.
	std::span<const COMDLG_FILTERSPEC> filters;

	// Optional extension (without a dot) appended to file names typed without one.
	const WCHAR* defaultExtension = nullptr;

	// Optional full path, bare file name, or directory (with a trailing separator). Preselects the
	// folder and/or the file name.
	const WCHAR* initialPath = nullptr;
};

std::optional<std::wstring> SelectFolder(const Options& options);
std::optional<std::wstring> SelectFile(const Options& options);
std::optional<std::wstring> SaveFile(const Options& options);

}  // namespace ShellDialog
