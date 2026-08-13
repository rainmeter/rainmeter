// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "Measure.h"
#include "Net.h"

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

	void ResetCounter();
	void ResetValue();

protected:
	void ReadOptions(ConfigParser& parser, const WCHAR* section) override;
	void UpdateValue() override;
	void Command(const std::wstring& command) override;

private:
	enum class ParseType : BYTE
	{
		RegExp,
		JsonPointer
	};

	void HandleFetchResult(BYTE* data, DWORD dataSize, DWORD errorCode);

	void StartDownloadTask();
	void HandleDownloadResult(const std::wstring&, HRESULT result);

	void ParseData(const BYTE* rawData, DWORD rawSize, bool utf16Data = false);
	bool ParseRegExp(std::wstring_view data);
	bool ParseJsonPointer(std::wstring_view data);

	struct ReferenceMatch
	{
		MeasureWebParser* measure;
		size_t position;
		size_t length;
	};

	ReferenceMatch FindMeasureUrlReference(Measure* measure) const;

	LocaleUtil::NumberFormat m_NumberFormat;
	std::wstring m_Url;
	std::wstring m_Expression;
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
	ParseType m_ParseType;
	int m_Codepage;
	int m_StringIndex;
	int m_StringIndex2;
	int m_DecodeCharacterReference;
	bool m_DecodeCodePoints;
	int m_Debug;
	UINT m_UpdateRate;
	UINT m_UpdateCounter;
	bool m_Download;
	bool m_ForceReload;
	bool m_LogSubstringErrors;
	DWORD m_InternetOpenUrlFlags;
	Net::FetchTask* m_FetchTask;
	Net::DownloadTask* m_DownloadTask;
};
