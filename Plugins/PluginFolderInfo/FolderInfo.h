/*
  Copyright (C) 2010 Elestel

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

#pragma once

#include <string>
#include <windows.h>
#include "../../Common/RawString.h"
#include "../../Library/pcre-8.10/config.h"
#include "../../Library/pcre-8.10/pcre.h"

class CFolderInfo
{
public:
	CFolderInfo(void* ownerSkin);
	~CFolderInfo();

	void AddInstance();
	void RemoveInstance();

	void* GetSkin() { return m_Skin; }
	DWORD GetLastUpdateTime() { return m_LastUpdateTime; }

	void SetPath(LPCWSTR path);
	void SetRegExpFilter(LPCWSTR filter);
	void SetSubFolders(bool flag) { m_IncludeSubFolders = flag; }
	void SetHiddenFiles(bool flag) { m_IncludeHiddenFiles = flag; }
	void SetSystemFiles(bool flag) { m_IncludeSystemFiles = flag; }

	UINT64 GetSize() { return m_Size; }
	int GetFileCount() { return m_FileCount; }
	int GetFolderCount() { return m_FolderCount; }

	void Update();

private:
	void Clear();
	void FreePcre();
	void CalculateSize();

	UINT m_InstanceCount;
	void* m_Skin;

	RawString m_Path;
	bool m_IncludeSubFolders;
	bool m_IncludeHiddenFiles;
	bool m_IncludeSystemFiles;
	UINT64 m_Size;
	UINT m_FileCount;
	UINT m_FolderCount;
	pcre* m_RegExpFilter;
	DWORD m_LastUpdateTime;
};
