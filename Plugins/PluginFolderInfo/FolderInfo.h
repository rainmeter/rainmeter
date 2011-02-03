#pragma once

#include <string>
#include <vector>
#include <windows.h>
#include "../../Library/pcre-8.10/pcre.h"

namespace PluginFolderInfo {
	
struct FileInfo
{
	std::wstring Name;
	bool IsFolder;
	UINT64 Size;

	FileInfo()
	{
		IsFolder = false;
		Size = 0;
	}
};

class FolderInfo
{
private:
	bool mySubFolderFlag;
	bool myHiddenFileFlag;
	bool mySystemFileFlag;
	std::wstring myPath;
	UINT64 mySize;
	unsigned int myFileCount;
	unsigned int myFolderCount;
	pcre* myRegExpFilter;
	pcre_extra* myRegExpFilterExtra;
	DWORD myLastUpdateTime;

private:
	void Clear();
	void CalculateSize();
	void SetPath(const wchar_t* aPath, const wchar_t* aIniPath);

public:
	DWORD GetLastUpdateTime()
	{
		return myLastUpdateTime;
	}

	void SetRegExpFilter(const wchar_t* aFilter);

	void IncludeSubFolders(bool aFlag)
	{
		mySubFolderFlag = aFlag;
	}

	void IncludeHiddenFiles(bool aFlag)
	{
		myHiddenFileFlag = aFlag;
	}

	void IncludeSystemFiles(bool aFlag)
	{
		mySystemFileFlag = aFlag;
	}

	UINT64 GetSize()
	{
		return mySize;
	}

	int GetFileCount()
	{
		return myFileCount;
	}
	
	int GetFolderCount()
	{
		return myFolderCount;
	}

	FolderInfo(const wchar_t* aPath, const wchar_t* aIniPath);
	void Update();
}; // class FolderInfo

} // namespace PluginFolderInfo
