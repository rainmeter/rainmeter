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
#include "../../Library/RawString.h"
#include "../../Library/pcre-8.10/config.h"
#include "../../Library/pcre-8.10/pcre.h"

class CFolderInfo
{
public:
	CFolderInfo(LPCWSTR path);
	~CFolderInfo();

	void AddInstance();
	void RemoveInstance();

	DWORD GetLastUpdateTime() { return m_LastUpdateTime; }

	void SetRegExpFilter(LPCWSTR filter);

	void IncludeSubFolders(bool flag) { m_IncludeSubFolders = flag; }
	void IncludeHiddenFiles(bool flag) { m_IncludeHiddenFiles = flag; }
	void IncludeSystemFiles(bool flag) { m_IncludeSystemFiles = flag; }

	LPCWSTR GetPath() { return m_Path.c_str(); }

	UINT64 GetSize() { return m_Size; }
	int GetFileCount() { return m_FileCount; }
	int GetFolderCount() { return m_FolderCount; }

	void Update();

private:
	void Clear();
	void FreePcre();
	void CalculateSize();

	UINT m_InstanceCount;

	CRawString m_Path;
	bool m_IncludeSubFolders;
	bool m_IncludeHiddenFiles;
	bool m_IncludeSystemFiles;
	UINT64 m_Size;
	UINT m_FileCount;
	UINT m_FolderCount;
	pcre* m_RegExpFilter;
	DWORD m_LastUpdateTime;
};
