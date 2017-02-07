/* Copyright (C) 2013 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_LIBRARY_SKINDIRECTORY_H_
#define RM_LIBRARY_SKINDIRECTORY_H_

#include <Windows.h>
#include <string>
#include <vector>
#include <cstdint>

// Reprsents a hierarchy of skin folders (reprsented by the Folder struct) and the names of their
// respective files.
class SkinRegistry
{
public:
	SkinRegistry() = default;
	SkinRegistry(const SkinRegistry& other) = delete;
	SkinRegistry& operator=(SkinRegistry other) = delete;

	struct File
	{
		std::wstring filename;
		bool isFavorite;

		File() {}
		~File() {}
		File(std::wstring name) : filename(name), isFavorite(false) {}

		const bool operator==(const File& rhs) const
		{
			return (rhs.filename == filename) && (rhs.isFavorite == isFavorite);
		}
	};

	struct Folder 
	{
		std::wstring name;
		std::vector<SkinRegistry::File> files;
		UINT baseID;

		int16_t active;
		int16_t level;

		bool hasFavorite;

		Folder() {}
		~Folder() {}

		Folder(Folder&& r) :
			name(std::move(r.name)),
			files(std::move(r.files)),
			baseID(r.baseID),
			active(r.active),
			level(r.level),
			hasFavorite(r.hasFavorite)
		{
		}

		Folder& operator=(Folder&& r)
		{
			name = std::move(r.name);
			files = std::move(r.files);
			baseID = r.baseID;
			active = r.active;
			level = r.level;
			hasFavorite = r.hasFavorite;
			return *this;
		}
	};

	struct Indexes
	{
		int folder;
		int file;

		Indexes(int folderIndex = 0, int fileIndex = 0) : folder(folderIndex), file(fileIndex) {}

		bool IsValid() const { return folder != -1; }

		static Indexes Invalid() { return Indexes(-1, 0); }
	};

	int FindFolderIndex(std::wstring folderPath) const;
	Folder* FindFolder(const std::wstring& folderPath);

	Indexes FindIndexes(const std::wstring& folderPath, const std::wstring& file);
	Indexes FindIndexesForID(UINT id);

	std::wstring GetFolderPath(int folderIndex) const;

	Folder& GetFolder(int index) { return m_Folders[index]; }
	int GetFolderCount() const { return (int)m_Folders.size(); }
	bool IsEmpty() const { return m_Folders.empty(); }

	void Populate(const std::wstring& path, std::vector<std::wstring>& favorites);

	std::vector<std::wstring> UpdateFavorite(const std::wstring& config, const std::wstring& filename, bool favorite);

private:
	int PopulateRecursive(const std::wstring& path, std::vector<std::wstring>& favorites, std::wstring base, int index, UINT level);

	std::vector<std::wstring> ValidateFavorites();

	// Contains a sequential list of Folders. The folders are arranged as follows:
	//   A         (index: 0, level: 1)
	//     B       (index: 1, level: 2)
	//       C     (index: 2, level: 3)
	//     D       (index: 3, level: 2)
	std::vector<Folder> m_Folders;
};

#endif
