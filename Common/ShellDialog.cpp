// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "ShellDialog.h"
#include "PathUtil.h"

namespace {

void SetInitialPath(IFileDialog* dialog, const WCHAR* initialPath)
{
	if (!initialPath || !*initialPath) return;

	const std::wstring path = initialPath;
	const std::wstring::size_type pos = path.find_last_of(L"\\/");
	if (pos != std::wstring::npos)
	{
		// Keep the separator for root paths like "C:\", but drop it everywhere else.
		std::wstring folder = path.substr(0, (pos > 0 && path[pos - 1] != L':') ? pos : pos + 1);

		Microsoft::WRL::ComPtr<IShellItem> item;
		if (SUCCEEDED(SHCreateItemFromParsingName(folder.c_str(), nullptr, IID_PPV_ARGS(&item))))
		{
			dialog->SetFolder(item.Get());
		}
	}

	const std::wstring name = (pos == std::wstring::npos) ? path : path.substr(pos + 1);
	if (!name.empty()) dialog->SetFileName(name.c_str());
}

std::optional<std::wstring> ShowDialog(REFCLSID clsid, FILEOPENDIALOGOPTIONS flags, const ShellDialog::Options& options)
{
	Microsoft::WRL::ComPtr<IFileDialog> dialog;
	HRESULT hr = CoCreateInstance(clsid, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog));
	if (FAILED(hr)) return std::nullopt;

	FILEOPENDIALOGOPTIONS dialogOptions = 0;
	dialog->GetOptions(&dialogOptions);
	dialog->SetOptions(dialogOptions | FOS_FORCEFILESYSTEM | flags);

	if (options.title) dialog->SetTitle(options.title);
	if (!options.filters.empty()) dialog->SetFileTypes((UINT)options.filters.size(), options.filters.data());
	if (options.defaultExtension) dialog->SetDefaultExtension(options.defaultExtension);
	SetInitialPath(dialog.Get(), options.initialPath);

	// Also fails if the user cancelled the dialog.
	if (FAILED(dialog->Show(options.parent))) return std::nullopt;

	Microsoft::WRL::ComPtr<IShellItem> item;
	if (FAILED(dialog->GetResult(&item))) return std::nullopt;

	WCHAR* buffer = nullptr;
	if (FAILED(item->GetDisplayName(SIGDN_FILESYSPATH, &buffer))) return std::nullopt;

	std::wstring path = buffer;
	CoTaskMemFree(buffer);
	return path;
}

}  // namespace

namespace ShellDialog {

std::optional<std::wstring> SelectFolder(const Options& options)
{
	// A folder is preselected only if it ends with a separator.
	std::wstring folder;
	if (options.initialPath && *options.initialPath)
	{
		folder = options.initialPath;
		PathUtil::AppendBackslashIfMissing(folder);
	}

	Options folderOptions = options;
	folderOptions.filters = {};
	folderOptions.defaultExtension = nullptr;
	folderOptions.initialPath = folder.c_str();

	return ShowDialog(CLSID_FileOpenDialog, FOS_PICKFOLDERS | FOS_PATHMUSTEXIST, folderOptions);
}

std::optional<std::wstring> SelectFile(const Options& options)
{
	return ShowDialog(CLSID_FileOpenDialog, FOS_FILEMUSTEXIST | FOS_PATHMUSTEXIST, options);
}

std::optional<std::wstring> SaveFile(const Options& options)
{
	return ShowDialog(CLSID_FileSaveDialog, FOS_OVERWRITEPROMPT, options);
}

}  // namespace ShellDialog
