/* Copyright (C) 2010 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once

#include <string>
#include <windows.h>
#include "../../Common/RawString.h"
#include "../../Library/pcre/config.h"
#include "../../Library/pcre/pcre.h"

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
	pcre16* m_RegExpFilter;
	DWORD m_LastUpdateTime;
};
