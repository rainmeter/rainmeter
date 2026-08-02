// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "../Common/DirectoryWatcher.h"
#include "../Common/PathUtil.h"
#include "SkinRegistry.h"
#include "resource.h"

static bool IsIgnoredRootFolder(const WCHAR* folder)
{
	static constexpr std::array<LPCWSTR, 3> ignoredFolders = { L"@Backup", L"Backup", L"@Vault" };
	return std::find_if(ignoredFolders.begin(), ignoredFolders.end(), [folder](LPCWSTR ignoredFolder) { return _wcsicmp(folder, ignoredFolder) == 0; }) != ignoredFolders.end();
}

SkinRegistry::SkinRegistry() = default;

SkinRegistry::~SkinRegistry() = default;

std::wstring SkinRegistry::GetFolderPath(int folderIndex) const
{
	// Traverse |m_Folders| backwards until level 1 is reached.
	const auto& skinFolder = m_Folders[folderIndex];
	std::wstring path = skinFolder.name;
	for (int i = skinFolder.level - 1, index = folderIndex; i >= 1; --i)
	{
		while (m_Folders[index].level != i)
		{
			--index;
		}

		path.insert(0, L"\\");
		path.insert(0, m_Folders[index].name);
	}
	return path;
}

int SkinRegistry::FindFolderIndex(std::wstring folderPath) const
{
	// Remove any leading and trailing slashes
	PathUtil::RemoveLeadingAndTrailingBackslash(folderPath);

	if (folderPath.empty()) return -1;

	const WCHAR* path = folderPath.c_str();
	int len = 0;
	while (path[len] && path[len] != L'\\') ++len;

	int level = 1;
	for (int i = 0, isize = (int)m_Folders.size(); i < isize; ++i)
	{
		const auto& skinFolder = m_Folders[i];
		if (skinFolder.level == level)
		{
			if (skinFolder.name.length() == len && _wcsnicmp(skinFolder.name.c_str(), path, len) == 0)
			{
				path += len;
				if (*path)
				{
					++path;	// Skip backslash
					len = 0;
					while (path[len] && path[len] != L'\\') ++len;
				}
				else
				{
					// Match found
					return i;
				}

				++level;
			}
		}
		else if (skinFolder.level < level)
		{
			break;
		}
	}

	return -1;
}

SkinRegistry::Folder* SkinRegistry::FindFolder(const std::wstring& folderPath)
{
	const int folderIndex = FindFolderIndex(folderPath);
	return (folderIndex != -1) ? &m_Folders[folderIndex] : nullptr;
}

SkinRegistry::Indexes SkinRegistry::FindIndexes(const std::wstring& folderPath, const std::wstring& file)
{
	const int folderIndex = FindFolderIndex(folderPath);
	if (folderIndex != -1)
	{
		const Folder& skinFolder = m_Folders[folderIndex];
		const WCHAR* fileSz = file.c_str();
		for (size_t i = 0, isize = skinFolder.files.size(); i < isize; ++i)
		{
			if (_wcsicmp(skinFolder.files[i].filename.c_str(), fileSz) == 0)
			{
				return Indexes(folderIndex, (int)i);
			}
		}
	}

	return Indexes::Invalid();  // Not found.
}

SkinRegistry::Indexes SkinRegistry::FindIndexesForID(UINT id)
{
	if (id >= ID_CONFIG_FIRST && id <= ID_CONFIG_LAST)
	{
		// Check which skin was selected
		for (size_t i = 0, isize = m_Folders.size(); i < isize; ++i)
		{
			const Folder& skinFolder = m_Folders[i];
			if (id >= skinFolder.baseID &&
				id < (skinFolder.baseID + skinFolder.files.size()))
			{
				return Indexes((int)i, (int)(id - skinFolder.baseID));
			}
		}
	}

	return Indexes::Invalid();  // Not found.
}

void SkinRegistry::Populate(const std::wstring& path, std::vector<std::wstring>& favorites)
{
	{
		CriticalSectionLock lock(m_ChangesLock);
		m_ChangedRootFolders.clear();
	}

	m_Folders.clear();
	PopulateRecursive(path, favorites, L"", 0, 0);
}

void SkinRegistry::PopulateChanged(const std::wstring& path, std::vector<std::wstring>& favorites)
{
	ankerl::unordered_dense::set<std::wstring> directories;
	{
		CriticalSectionLock lock(m_ChangesLock);
		directories.swap(m_ChangedRootFolders);
	}

	for (const auto& directory : directories)
	{
		auto first = std::find_if(m_Folders.begin(), m_Folders.end(), [&](const Folder& folder) { return folder.level == 1 && _wcsicmp(folder.name.c_str(), directory.c_str()) == 0; });
		if (first != m_Folders.end())
		{
			auto last = std::find_if(first + 1, m_Folders.end(), [](const Folder& folder) { return folder.level == 1; });
			m_Folders.erase(first, last);
		}

		SkinRegistry registry;
		registry.PopulateRecursive(path, favorites, directory, 0, 1);
		if (registry.m_Folders.empty()) continue;

		// Insert the rebuilt subtree in root folder order.
		auto position = std::find_if(m_Folders.begin(), m_Folders.end(), [&](const Folder& folder) { return folder.level == 1 && _wcsicmp(folder.name.c_str(), directory.c_str()) > 0; });
		m_Folders.insert(position, std::make_move_iterator(registry.m_Folders.begin()), std::make_move_iterator(registry.m_Folders.end()));
	}

	UpdateBaseIDs();
}

void SkinRegistry::HandleDirectoryChange(const WCHAR* path, DWORD action, DWORD attributes)
{
	if (action == FILE_ACTION_MODIFIED) return;

	const WCHAR* relativePath = path + m_ChangeWatcher->GetPath().length();
	const WCHAR* ext = wcsrchr(relativePath, L'.');
	const bool iniChanged = ext && _wcsicmp(ext, L".ini") == 0;
	const bool directoryChanged = (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
	if (!iniChanged && !directoryChanged) return;

	const WCHAR* separator = wcschr(relativePath, L'\\');
	const std::wstring directory(relativePath, separator ? separator - relativePath : wcslen(relativePath));
	if (directory.empty() || (!separator && !directoryChanged) || IsIgnoredRootFolder(directory.c_str())) return;

	CriticalSectionLock lock(m_ChangesLock);
	m_ChangedRootFolders.emplace(directory);
}

bool SkinRegistry::HasChanges(std::wstring folderPath)
{
	CriticalSectionLock lock(m_ChangesLock);
	if (folderPath.empty()) return !m_ChangedRootFolders.empty();

	PathUtil::RemoveLeadingAndTrailingBackslash(folderPath);
	const size_t separator = folderPath.find(L'\\');
	if (separator != std::wstring::npos) folderPath.resize(separator);
	return std::find_if(m_ChangedRootFolders.begin(), m_ChangedRootFolders.end(), [&](const std::wstring& folder) { return _wcsicmp(folder.c_str(), folderPath.c_str()) == 0; }) != m_ChangedRootFolders.end();
}

void SkinRegistry::StartWatching(const std::wstring& path)
{
	if (!m_ChangeWatcher) m_ChangeWatcher = std::make_unique<DirectoryWatcher>();

	auto callback = [](const WCHAR* path, DWORD action, DWORD attributes, void* context)
	{
		auto* registry = (SkinRegistry*)context;
		registry->HandleDirectoryChange(path, action, attributes);
	};
	const DWORD flags = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME;
	m_ChangeWatcher->Start(path, true, callback, this, flags);
}

void SkinRegistry::UpdateBaseIDs()
{
	UINT id = ID_CONFIG_FIRST;
	for (auto& folder : m_Folders)
	{
		folder.baseID = id;
		id += (UINT)folder.files.size();
	}
}

int SkinRegistry::PopulateRecursive(const std::wstring& path, std::vector<std::wstring>& favorites, std::wstring base, int index, UINT level)
{
	WIN32_FIND_DATA fileData = { 0 };			// Data structure describes the file found
	HANDLE hSearch = nullptr;					// Search handle returned by FindFirstFile
	std::list<std::wstring> subfolders;

	// Find all .ini files and subfolders
	std::wstring filter = path + base;
	filter += L"\\*";

	hSearch = FindFirstFileEx(
		filter.c_str(),
		FindExInfoBasic,
		&fileData,
		FindExSearchNameMatch,
		nullptr,
		0);

	bool foundFiles = false;
	if (hSearch != INVALID_HANDLE_VALUE)
	{
		Folder folder;
		folder.baseID = ID_CONFIG_FIRST + index;
		folder.active = 0;
		folder.level = level;
		folder.hasFavorite = false;

		do
		{
			const std::wstring filename = fileData.cFileName;

			if (fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				if (!PathUtil::IsDotOrDotDot(fileData.cFileName) &&
					!(level == 0 && IsIgnoredRootFolder(fileData.cFileName)) &&
					!(level == 1 && wcscmp(L"@Resources", fileData.cFileName) == 0))
				{
					subfolders.push_back(filename);
				}
			}
			else if (level != 0)
			{
				// Check whether the extension is ".ini"
				size_t filenameLen = filename.size();
				if (filenameLen >= 4 && _wcsicmp(fileData.cFileName + (filenameLen - 4), L".ini") == 0)
				{
					File file(filename);
					std::wstring temp = base + L'\\';
					temp += filename;

					for (const auto& favorite : favorites)
					{
						if (temp == favorite)
						{
							file.isFavorite = true;
							folder.hasFavorite = true;	// current folder

							// Tell parent folder(s) it has a favorite
							temp = base;
							std::wstring::size_type pos = 0;
							std::wstring prevConfig;
							Folder* prevFolder = nullptr;

							for (int i = level; i > 1; --i)
							{
								pos = temp.rfind(L'\\');
								prevConfig.assign(temp, 0, pos);
								prevFolder = FindFolder(prevConfig);

								if (prevFolder)
								{
									prevFolder->hasFavorite = true;
								}

								temp = prevConfig;
							}

							break;
						}
					}

					folder.files.push_back(file);
					foundFiles = true;
					++index;
				}
			}
		}
		while (FindNextFile(hSearch, &fileData));

		FindClose(hSearch);

		if (level > 0 && (foundFiles || !subfolders.empty()))
		{
			if (level == 1)
			{
				folder.name = base;
			}
			else
			{
				std::wstring::size_type pos = base.rfind(L'\\') + 1;
				folder.name.assign(base, pos, base.length() - pos);
			}

			m_Folders.push_back(std::move(folder));
		}
	}

	if (level != 0)
	{
		base += L'\\';
	}

	if (!subfolders.empty())
	{
		bool popFolder = !foundFiles;

		std::list<std::wstring>::const_iterator iter = subfolders.begin();
		for ( ; iter != subfolders.end(); ++iter)
		{
			int newIndex = PopulateRecursive(path, favorites, base + (*iter), index, level + 1);
			if (newIndex != index)
			{
				popFolder = false;
			}

			index = newIndex;
		}

		if (popFolder)
		{
			m_Folders.pop_back();
		}
	}

	return index;
}

std::vector<std::wstring> SkinRegistry::UpdateFavorite(const std::wstring& config, const std::wstring& filename, bool favorite)
{
	Folder* folder = FindFolder(config);
	for (auto& file : folder->files)
	{
		if (file.filename == filename)
		{
			file.isFavorite = favorite;
			break;
		}
	}

	return ValidateFavorites();
}

std::vector<std::wstring> SkinRegistry::ValidateFavorites()
{
	// Files should be marked correctly at this point, Folders are not
	int16_t count = 0;
	int16_t foundOnLevel = 0;
	std::vector<std::wstring> newFavorites;
	std::wstring favorite;
	Indexes indexes;

	auto folder = m_Folders.rbegin();
	for (; folder != m_Folders.rend(); ++folder)
	{
		(*folder).hasFavorite = false;

		auto file = (*folder).files.rbegin();
		for (; file != (*folder).files.rend(); ++file)
		{
			if ((*file).isFavorite)
			{
				++count;
				foundOnLevel = (*folder).level;
				(*folder).hasFavorite = true;	// to mark current folder

				indexes = FindIndexesForID((*folder).baseID);
				favorite = GetFolderPath(indexes.folder);
				favorite += L'\\';
				favorite += (*file).filename;
				newFavorites.emplace(newFavorites.begin(), favorite);
			}
		}

		if (foundOnLevel > (*folder).level && count > 0)
		{
			(*folder).hasFavorite = true;
			--foundOnLevel;
		}

		if ((*folder).level == 1)
		{
			count = 0;
			foundOnLevel = 0;
		}
	}

	return newFavorites;
}
