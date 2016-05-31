/* Copyright (C) 2015 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_LIBRARY_MEASUREWEBPARSER_H_
#define RM_LIBRARY_MEASUREWEBPARSER_H_

#include "Measure.h"

struct ProxySetting
{
	std::wstring agent;
	std::wstring server;
	HINTERNET handle;

	ProxySetting() : handle() {}
};

class MeasureWebParser : public Measure
{
public:
	MeasureWebParser(Skin* skin, const WCHAR* name);
	virtual ~MeasureWebParser();

	MeasureWebParser(const MeasureWebParser& other) = delete;
	MeasureWebParser& operator=(MeasureWebParser other) = delete;

	UINT GetTypeID() override { return TypeID<MeasureWebParser>(); }

	const WCHAR* GetStringValue() override;

protected:
	void ReadOptions(ConfigParser& parser, const WCHAR* section) override;
	void UpdateValue() override;
	void Command(const std::wstring& command) override;

private:
	static unsigned __stdcall NetworkThreadProc(void* pParam);
	static unsigned __stdcall NetworkDownloadThreadProc(void* pParam);
	void ParseData(const BYTE* rawData, DWORD rawSize, bool utf16Data = false);

	std::wstring m_Url;
	std::wstring m_RegExp;
	std::wstring m_ResultString;
	std::wstring m_ErrorString;
	std::wstring m_FinishAction;
	std::wstring m_OnRegExpErrAction;
	std::wstring m_OnConnectErrAction;
	std::wstring m_OnDownloadErrAction;
	std::wstring m_DownloadFolder;
	std::wstring m_DownloadFile;
	std::wstring m_DownloadedFile;
	std::wstring m_DebugFileLocation;
	std::wstring m_Headers;
	ProxySetting m_Proxy;
	HANDLE m_ThreadHandle;
	HANDLE m_DlThreadHandle;
	int m_Codepage;
	int m_StringIndex;
	int m_StringIndex2;
	int m_DecodeCharacterReference;
	int m_Debug;
	UINT m_UpdateRate;
	UINT m_UpdateCounter;
	bool m_Download;
	bool m_ForceReload;
	bool m_LogSubstringErrors;
};

#endif
