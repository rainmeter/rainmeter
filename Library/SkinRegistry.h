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
	struct Folder 
	{
		std::wstring name;
		std::vector<std::wstring> files;
		UINT baseID;

		int16_t active;
		int16_t level;

		Folder() {}
		~Folder() {}

		Folder(Folder&& r) :
			name(std::move(r.name)),
			files(std::move(r.files)),
			baseID(r.baseID),
			active(r.active),
			level(r.level)
		{
		}

		Folder& operator=(Folder&& r)
		{
			name = std::move(r.name);
			files = std::move(r.files);
			baseID = r.baseID;
			active = r.active;
			level = r.level;
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

	int FindFolderIndex(const std::wstring& folderPath) const;
	Folder* FindFolder(const std::wstring& folderPath);

	Indexes FindIndexes(const std::wstring& folderPath, const std::wstring& file);
	Indexes FindIndexesForID(UINT id);

	std::wstring GetFolderPath(int folderIndex) const;

	Folder& GetFolder(int index) { return m_Folders[index]; }
	int GetFolderCount() const { return (int)m_Folders.size(); }
	bool IsEmpty() const { return m_Folders.empty(); }

	void Populate(const std::wstring& path);

private:
	int PopulateRecursive(const std::wstring& path, std::wstring base, int index, UINT level);

	// Contains a sequential list of Folders. The folders are arranged as follows:
	//   A         (index: 0, level: 1)
	//     B       (index: 1, level: 2)
	//       C     (index: 2, level: 3)
	//     D       (index: 3, level: 2)
	std::vector<Folder> m_Folders;
};

#endif
