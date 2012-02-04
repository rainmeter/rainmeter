#include "FolderInfo.h"
#include <windows.h>
#include <list>

namespace PluginFolderInfo {
	
FolderInfo::FolderInfo(const wchar_t* aPath, const wchar_t* aIniPath)
{
	mySubFolderFlag = false;
	myHiddenFileFlag = false;
	mySystemFileFlag = false;
	myRegExpFilter = NULL;
	myRegExpFilterExtra = NULL;
	myLastUpdateTime = 0;
	Clear();
	SetPath(aPath, aIniPath);
}

FolderInfo::~FolderInfo()
{
	FreePcre();
}

void FolderInfo::Clear()
{
	mySize = 0;
	myFileCount = 0;
	myFolderCount = 0;
}

void FolderInfo::FreePcre()
{
	if (myRegExpFilter) {
		pcre_free(myRegExpFilter);
		myRegExpFilter = NULL;
	}

	if (myRegExpFilterExtra) {
		pcre_free(myRegExpFilterExtra);
		myRegExpFilterExtra = NULL;
	}
}

void FolderInfo::SetPath(const wchar_t* aPath, const wchar_t* aIniPath)
{
	if (!aPath || 0 == aPath[0]) {
		myPath = L"";
		return;
	}

	myPath = aPath;
	if (wcsncmp(aPath, L".\\", 2) == 0 || wcsncmp(aPath, L"..\\", 3) == 0) {
		wchar_t* buf = new wchar_t[wcslen(aIniPath) + 1];
		wcscpy(buf, aIniPath);
		wchar_t* iniFileName = wcsrchr(buf, '\\');
		if (iniFileName) {
			iniFileName[1] = 0;
			myPath = buf;
			myPath += aPath;
		}
		delete[] buf;
	}

	if (myPath[myPath.size() - 1] != L'\\') {
		myPath += L"\\";
	}
}

void FolderInfo::Update()
{
	Clear();

	if (myPath.length() == 0) {
		return;
	}

	CalculateSize();
	myLastUpdateTime = GetTickCount();
}

void FolderInfo::CalculateSize()
{
	std::list<std::wstring> folderQueue;
	folderQueue.push_back(myPath.c_str());

	wchar_t searchPattern[MAX_PATH + 10];
	wchar_t buffer[MAX_PATH];
	char utf8Buf[MAX_PATH * 3];
	WIN32_FIND_DATA findData;
	HANDLE findHandle;
	while (!folderQueue.empty()) {
		std::list<std::wstring>::reference ref = folderQueue.front();
		wsprintf(searchPattern, L"%s%s", ref.c_str(), L"\\*.*");
		findHandle = ::FindFirstFile(searchPattern, &findData);
		if (INVALID_HANDLE_VALUE == findHandle) {
			folderQueue.pop_front();
			continue;
		}

		do {
			// special case for "." and ".."
			if (wcscmp(findData.cFileName, L".") == 0 ||
				wcscmp(findData.cFileName, L"..") == 0) {
				continue;
			}

			bool isFolder = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) > 0;

			if (!myHiddenFileFlag && (findData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)) {
				continue;
			}
			else if (!mySystemFileFlag && (findData.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM)) {
				continue;
			}
			else if (!isFolder && myRegExpFilter) {
				int utf8BufLen = WideCharToMultiByte(CP_UTF8, 0, findData.cFileName, wcslen(findData.cFileName) + 1, utf8Buf, MAX_PATH * 3, NULL, NULL);
				if (0 != pcre_exec(myRegExpFilter, myRegExpFilterExtra, utf8Buf, utf8BufLen, 0, 0, NULL, 0)) {
					continue;
				}
			}

			if (isFolder) {
				myFolderCount++;
				if (mySubFolderFlag) {
					wsprintf(buffer, L"%s\\%s", ref.c_str(), findData.cFileName);
					folderQueue.push_back(buffer);
				}
			}
			else {
				myFileCount++;
				mySize += ((UINT64)findData.nFileSizeHigh << 32) + findData.nFileSizeLow;
			}
		}
		while (::FindNextFile(findHandle, &findData));
		FindClose(findHandle);

		folderQueue.pop_front();
	}
}

void FolderInfo::SetRegExpFilter(const wchar_t* aFilter)
{
	FreePcre();

	if (aFilter == NULL) {
		return;
	}

	int filterLen = wcslen(aFilter) + 1;
	int bufLen = WideCharToMultiByte(CP_UTF8, 0, aFilter, filterLen, NULL, 0, NULL, NULL);

	char* buf = new char[bufLen];
	WideCharToMultiByte(CP_UTF8, 0, aFilter, filterLen, buf, bufLen, NULL, NULL);

	const char* error;
	int erroffset;
	myRegExpFilter = pcre_compile(buf, PCRE_UTF8, &error, &erroffset, NULL);
	if (myRegExpFilter) {
		myRegExpFilterExtra = pcre_study(myRegExpFilter, 0, &error);
	}

	delete [] buf;
}

} // namespace PluginFolderInfo
