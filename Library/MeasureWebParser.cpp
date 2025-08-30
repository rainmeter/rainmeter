/* Copyright (C) 2005 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "MeasureWebParser.h"
#include "Rainmeter.h"
#include "System.h"
#include "pcre/config.h"
#include "pcre/pcre.h"
#include "../Common/CharacterEntityReference.h"
#include "../Common/StringUtil.h"
#include "../Common/FileUtil.h"

#include "json/json.hpp"

using nlohmann::json;

void ShowError(MeasureWebParser *measure, WCHAR *description);

template <typename Iterator, typename Pred, typename Operation>
void for_each_if(Iterator begin, Iterator end, Pred p, Operation op)
{
	for (; begin != end; begin++)
	{
		if (p(*begin))
		{
			op(*begin);
		}
	}
}

class ProxyCachePool
{
public:
	ProxyCachePool(LPCWSTR globalProxyName = nullptr, LPCWSTR globalUserAgent = nullptr) :
		m_GlobalProxyName((globalProxyName && *globalProxyName) ? globalProxyName : L"/auto"),
		m_GlobalUserAgent((globalUserAgent && *globalUserAgent) ? globalUserAgent : L"Rainmeter WebParser plugin")
	{
		m_GlobalProxyCache = new ProxyCache(
			CreateProxy(m_GlobalProxyName.c_str(), m_GlobalUserAgent.c_str()),
			m_GlobalUserAgent,
			true);

		_wcslwr(&m_GlobalProxyName[0]);
		m_CacheMap.emplace(m_GlobalProxyName, m_GlobalProxyCache);
		//LogDebugF(L"* ADD-GLOBAL: key=%s, handle=0x%p, ref=new, agent=%s", m_GlobalProxyName.c_str(),
		//	m_GlobalProxyCache->GetCache(), m_GlobalUserAgent.c_str());
	}

	~ProxyCachePool()
	{
		for (auto iter = m_CacheMap.begin(); iter != m_CacheMap.end(); ++iter)
		{
			ProxyCache* cache = (*iter).second;
			//LogDebugF(L"* FORCE-REMOVE: key=%s, global=%i, ref=%i, agent=%s", (*iter).first.c_str(),
			//	cache->IsGlobal(), cache->GetRef(), (*iter).second->GetAgent().c_str());
			delete cache;
			cache = nullptr;
		}
	}

	HINTERNET GetCache(const std::wstring& proxyName, const std::wstring& userAgent)
	{
		ProxyCache* cache = nullptr;

		std::wstring key = proxyName.empty() ? m_GlobalProxyName : proxyName;
		std::wstring agent = userAgent.empty() ? m_GlobalUserAgent : userAgent;
		_wcslwr(&key[0]);

		bool found = false;
		auto iters = m_CacheMap.equal_range(key);
		for (auto& it = iters.first; it != iters.second; ++it)
		{
			if (StringUtil::CaseInsensitiveFind(it->second->GetAgent(), agent) != std::wstring::npos)
			{
				found = true;
				cache = it->second;
				break;
			}
		}

		if (!found)
		{
			// Create new proxy
			cache = new ProxyCache(CreateProxy(key.c_str(), agent.c_str()), agent);
			m_CacheMap.emplace(key, cache);
			//LogDebugF(L"* ADD: key=%s, handle=0x%p, ref=new, agent=%s", key.c_str(), cache->GetCache(), agent.c_str());
			return cache->GetCache();
		}

		// Use proxy cache
		cache->AddRef();
		//LogDebugF(L"* ADD-REF: key=%s, handle=0x%p, global=%i, ref=%i, agent=%s",
		//	cache->IsGlobal() ? m_GlobalProxyName.c_str() : proxyName.c_str(), cache->GetCache(),
		//	cache->IsGlobal(), cache->GetRef(), agent.c_str());
		return cache->GetCache();
	}

	void RemoveCache(const std::wstring& proxyName, const std::wstring& userAgent)
	{
		std::wstring key = proxyName.empty() ? m_GlobalProxyName : proxyName;
		std::wstring agent = userAgent.empty() ? m_GlobalUserAgent : userAgent;

		if (!proxyName.empty())
		{
			_wcslwr(&key[0]);
		}

		auto iters = m_CacheMap.equal_range(key);
		for (auto& it = iters.first; it != iters.second; ++it)
		{
			if (StringUtil::CaseInsensitiveFind(it->second->GetAgent(), agent) != std::wstring::npos)
			{
				ProxyCache* cache = it->second;
				cache->Release();
				//LogDebugF(L"* REMOVE: key=%s, global=%i, ref=%i, agent=%s",
				//	key.c_str(), cache->IsGlobal(), cache->GetRef(), agent.c_str());

				if (cache->IsInvalid())
				{
					//LogDebugF(L"* EMPTY-ERASE: key=%s, agent=%s", key.c_str(), agent.c_str());
					m_CacheMap.erase(it);
					delete cache;
					cache = nullptr;
				}

				break;
			}
		}
	}

private:
	HINTERNET CreateProxy(LPCWSTR proxyName, LPCWSTR userAgent)
	{
		DWORD proxyType;
		LPCWSTR proxyServer;

		if (_wcsicmp(proxyName, L"/auto") == 0)
		{
			proxyType = INTERNET_OPEN_TYPE_PRECONFIG;
			proxyServer = nullptr;
		}
		else if (_wcsicmp(proxyName, L"/none") == 0)
		{
			proxyType = INTERNET_OPEN_TYPE_DIRECT;
			proxyServer = nullptr;
		}
		else
		{
			proxyType = INTERNET_OPEN_TYPE_PROXY;
			proxyServer = proxyName;
		}

		HINTERNET handle = InternetOpen(userAgent, proxyType, proxyServer, nullptr, 0);
		if (handle)
		{
			if (GetRainmeter().GetDebug())
			{
				LogDebugF(
					L"ProxyServer=\"%s\" (type=%s, handle=0x%p)",
					proxyName,
					proxyType == INTERNET_OPEN_TYPE_PRECONFIG ? L"PRECONFIG" : proxyType == INTERNET_OPEN_TYPE_DIRECT ? L"DIRECT" : L"PROXY",
					handle);
				LogDebugF(L"Useragent=\"%s\"", userAgent);
			}
		}
		else
		{
			ShowError(nullptr, L"InternetOpen error");
		}

		return handle;
	}

	class ProxyCache
	{
	public:
		ProxyCache(HINTERNET handle, std::wstring agent, bool isGlobal = false) :
			m_Handle(handle), m_Agent(agent), m_IsGlobal(isGlobal), m_Ref(1) {}
		~ProxyCache() { Dispose(); }

		void AddRef() { if (!IsInvalid()) { ++m_Ref; } }
		void Release() { if (m_Ref > 0) { --m_Ref; } if (IsInvalid()) { Dispose(); } }

		bool IsGlobal() { return m_IsGlobal; }
		bool IsInvalid() { return (m_Ref <= 0 && !IsGlobal()); }
		//int GetRef() { return m_Ref; }
		HINTERNET GetCache() { return m_Handle; }
		std::wstring& GetAgent() { return m_Agent; }

	private:
		ProxyCache() = delete;
		ProxyCache(const ProxyCache& cache) = delete;

		void Dispose() { if (m_Handle) { InternetCloseHandle(m_Handle); m_Handle = nullptr; } }

		HINTERNET m_Handle;
		std::wstring m_Agent;
		bool m_IsGlobal;
		int m_Ref;
	};

	std::unordered_multimap<std::wstring, ProxyCache*> m_CacheMap;
	ProxyCache* m_GlobalProxyCache;
	std::wstring m_GlobalProxyName;
	std::wstring m_GlobalUserAgent;
};

BYTE* DownloadUrl(HINTERNET handle, std::wstring& url, std::wstring& headers, DWORD* dataSize, DWORD flags);

CRITICAL_SECTION g_CriticalSection;
ProxyCachePool* g_ProxyCachePool = nullptr;
UINT g_InstanceCount = 0U;

static std::vector<MeasureWebParser*> g_Measures;

#define OVECCOUNT 300    // should be a multiple of 3

void SetupGlobalProxySetting()
{
	if (!g_ProxyCachePool)
	{
		WCHAR server[MAX_PATH] = { 0 };
		WCHAR agent[MAX_PATH] = { 0 };
		LPCWSTR file = GetRainmeter().GetDataFile().c_str();

		if (GetPrivateProfileString(L"WebParser", L"ProxyServer", nullptr, server, MAX_PATH, file) == 0)
		{
			GetPrivateProfileString(L"WebParser.dll", L"ProxyServer", nullptr, server, MAX_PATH, file);  // For backwards compatibility
		}
		if (GetPrivateProfileString(L"WebParser", L"UserAgent", nullptr, agent, MAX_PATH, file) == 0)
		{
			GetPrivateProfileString(L"WebParser.dll", L"UserAgent", nullptr, agent, MAX_PATH, file);  // For backwards compatibility
		}
		g_ProxyCachePool = new ProxyCachePool(server, agent);
	}
}

void ClearGlobalProxySetting()
{
	delete g_ProxyCachePool;
	g_ProxyCachePool = nullptr;
}

void SetupProxySetting(ProxySetting& setting, const std::wstring proxyServer, const std::wstring userAgent)
{
	if (g_ProxyCachePool)
	{
		setting.server = proxyServer;
		setting.agent = userAgent;
		setting.handle = g_ProxyCachePool->GetCache(setting.server, setting.agent);
	}
}

void ClearProxySetting(ProxySetting& setting)
{
	if (g_ProxyCachePool)
	{
		g_ProxyCachePool->RemoveCache(setting.server, setting.agent);
	}

	setting.handle = nullptr;
	setting.server.clear();
	setting.agent.clear();
}

MeasureWebParser::MeasureWebParser(Skin* skin, const WCHAR* name) : Measure(skin, name),
	m_ThreadHandle(),
	m_DlThreadHandle(),
	m_Codepage(),
	m_StringIndex(),
	m_StringIndex2(),
	m_DecodeCharacterReference(),
	m_DecodeCodePoints(false),
	m_Debug(),
	m_LogSubstringErrors(),
	m_UpdateRate(),
	m_UpdateCounter(),
	m_Download(),
	m_ForceReload(),
	m_InternetOpenUrlFlags(INTERNET_FLAG_RESYNCHRONIZE)
{
	g_Measures.push_back(this);

	if (g_InstanceCount == 0)
	{
		System::InitializeCriticalSection(&g_CriticalSection);
		SetupGlobalProxySetting();
	}

	// No DynamicVariables support for ProxyServer or UserAgent
	SetupProxySetting(
		m_Proxy,
		GetSkin()->GetParser().ReadString(name, L"ProxyServer", L""),
		GetSkin()->GetParser().ReadString(name, L"UserAgent", L""));

	++g_InstanceCount;
}

MeasureWebParser::~MeasureWebParser()
{
	if (m_ThreadHandle)
	{
		// Thread is killed inside critical section so that itself is not inside one when it is terminated
		EnterCriticalSection(&g_CriticalSection);

		TerminateThread(m_ThreadHandle, 0);
		m_ThreadHandle = nullptr;

		LeaveCriticalSection(&g_CriticalSection);
	}

	if (m_DlThreadHandle)
	{
		// Thread is killed inside critical section so that itself is not inside one when it is terminated
		EnterCriticalSection(&g_CriticalSection);

		TerminateThread(m_DlThreadHandle, 0);
		m_DlThreadHandle = nullptr;

		LeaveCriticalSection(&g_CriticalSection);
	}

	if (m_DownloadFile.empty())  // cache mode
	{
		if (!m_DownloadedFile.empty())
		{
			// Delete the file
			DeleteFile(m_DownloadedFile.c_str());
		}
	}

	ClearProxySetting(m_Proxy);

	auto iter = std::find(g_Measures.begin(), g_Measures.end(), this);
	g_Measures.erase(iter);

	--g_InstanceCount;
	if (g_InstanceCount == 0)
	{
		// Last one, close all handles
		ClearGlobalProxySetting();

		// Last instance deletes the critical section
		DeleteCriticalSection(&g_CriticalSection);
	}
}

void MeasureWebParser::ReadOptions(ConfigParser& parser, const WCHAR* section)
{
	EnterCriticalSection(&g_CriticalSection);

	Measure::ReadOptions(parser, section);

	std::wstring url = parser.ReadString(section, L"Url", L"", false);

	// Parse new-style variables without parsing old-style section variables
	if (parser.ContainsNewStyleVariable(url))
	{
		parser.ParseVariables(url, ConfigParser::VariableType::Section);
	}

	m_Url = url;

	m_Headers.clear();
	size_t hNum = 1ULL;
	std::wstring hOption = L"Header";
	std::wstring hValue = parser.ReadString(section, hOption.c_str(), L"");
	while (!hValue.empty())
	{
		m_Headers += hValue + L"\r\n";
		hOption = L"Header" + std::to_wstring(++hNum);
		hValue = parser.ReadString(section, hOption.c_str(), L"");
	}

	if (!m_Headers.empty())
	{
		m_Headers += L"\r\n";  // Append "\r\n" to last header to denote end of header section
	}

	std::wstring parseModeConfig = parser.ReadString(section, L"ParseMode", L"Regex");
	if (parseModeConfig == L"Json")
	{
		m_ParseMode = ParseMode::Json;

		m_JsonValueSpecs.clear();
		for (int i = 1; true; ++i)
		{
			auto configName = L"JsonValuePath" + std::to_wstring(i);
			auto jsonPathSpec = parser.ReadString(section, configName.c_str(), L"N/A");
			if (jsonPathSpec == L"N/A")
				break;
			LogNoticeF(this, L"%s = %s", configName.c_str(), jsonPathSpec.c_str());
			m_JsonValueSpecs.push_back(jsonPathSpec);
		}
	}
	else
	{
		m_ParseMode = ParseMode::Regex;
	}

	m_RegExp = parser.ReadString(section, L"RegExp", L"");
	m_FinishAction = parser.ReadString(section, L"FinishAction", L"", false);
	m_OnParseErrAction = parser.ReadString(section, L"OnRegExpErrorAction", L"", false);
	m_OnConnectErrAction = parser.ReadString(section, L"OnConnectErrorAction", L"", false);
	m_OnDownloadErrAction = parser.ReadString(section, L"OnDownloadErrorAction", L"", false);
	m_ErrorString = parser.ReadString(section, L"ErrorString", L"");
	m_LogSubstringErrors = parser.ReadBool(section, L"LogSubstringErrors", true);

	int index = parser.ReadInt(section, L"StringIndex", 0);
	m_StringIndex = index < 0 ? 0 : index;

	index = parser.ReadInt(section, L"StringIndex2", 0);
	m_StringIndex2 = index < 0 ? 0 : index;

	m_DecodeCharacterReference = parser.ReadInt(section, L"DecodeCharacterReference", 0);
	m_DecodeCodePoints = parser.ReadBool(section, L"DecodeCodePoints", false);

	m_UpdateRate = parser.ReadInt(section, L"UpdateRate", 600);
	m_Codepage = parser.ReadInt(section, L"CodePage", 0);
	if (m_Codepage == 0)
	{
		m_Codepage = CP_UTF8;
	}

	m_Download = parser.ReadBool(section, L"Download", false);
	if (m_Download)
	{
		m_DownloadFolder = L"DownloadFile\\";
		GetSkin()->MakePathAbsolute(m_DownloadFolder);
		m_DownloadFile = parser.ReadString(section, L"DownloadFile", L"");
	}
	else
	{
		m_DownloadFile.clear();
	}

	m_Debug = parser.ReadInt(section, L"Debug", 0);
	if (m_Debug == 2)
	{
		std::wstring oldDebugFileLocation = m_DebugFileLocation;
		m_DebugFileLocation = parser.ReadString(section, L"Debug2File", L"WebParserDump.txt");
		GetSkin()->MakePathAbsolute(m_DebugFileLocation);

		if (_wcsicmp(oldDebugFileLocation.c_str(), m_DebugFileLocation.c_str()) != 0)
		{
			LogNoticeF(this, L"Debug file: %s", m_DebugFileLocation.c_str());
		}
	}

	{
		m_ForceReload = parser.ReadBool(section, L"ForceReload", false);  // Deprecated

		m_InternetOpenUrlFlags = m_ForceReload ? INTERNET_FLAG_RELOAD : INTERNET_FLAG_RESYNCHRONIZE;
		std::wstring szFlags = parser.ReadString(section, L"Flags", L"");
		if (!szFlags.empty())
		{
			// Flags: https://docs.microsoft.com/en-us/windows/win32/api/wininet/nf-wininet-internetopenurlw#parameters
			std::vector<std::wstring> tokens = ConfigParser::Tokenize(szFlags, L"|");
			for (const auto& token : tokens)
			{
				const WCHAR* flag = token.c_str();
				if (_wcsicmp(flag, L"ForceReload") == 0)
				{
					m_InternetOpenUrlFlags |= INTERNET_FLAG_RELOAD;
				}
				else if (_wcsicmp(flag, L"Resync") == 0)
				{
					m_InternetOpenUrlFlags |= INTERNET_FLAG_RESYNCHRONIZE;
				}
				else if (_wcsicmp(flag, L"NoCookies") == 0)
				{
					m_InternetOpenUrlFlags |= INTERNET_FLAG_NO_COOKIES;
				}
				else if (_wcsicmp(flag, L"Hyperlink") == 0)
				{
					m_InternetOpenUrlFlags |= INTERNET_FLAG_HYPERLINK;
				}
				else if (_wcsicmp(flag, L"TempFile") == 0)
				{
					m_InternetOpenUrlFlags |= INTERNET_FLAG_NEED_FILE;
				}
				else if (_wcsicmp(flag, L"NoAuth") == 0)
				{
					m_InternetOpenUrlFlags |= INTERNET_FLAG_NO_AUTH;
				}
				else if (_wcsicmp(flag, L"NoCacheWrite") == 0)
				{
					m_InternetOpenUrlFlags |= INTERNET_FLAG_NO_CACHE_WRITE;
				}
				else if (_wcsicmp(flag, L"PragmaNoCache") == 0)
				{
					m_InternetOpenUrlFlags |= INTERNET_FLAG_PRAGMA_NOCACHE;
				}
				else if (_wcsicmp(flag, L"Secure") == 0)
				{
					m_InternetOpenUrlFlags |= INTERNET_FLAG_SECURE;
				}
				else if (_wcsicmp(flag, L"IgnoreCertName") == 0)
				{
					m_InternetOpenUrlFlags |= INTERNET_FLAG_IGNORE_CERT_CN_INVALID;
				}
				else if (_wcsicmp(flag, L"IgnoreCertDate") == 0)
				{
					m_InternetOpenUrlFlags |= INTERNET_FLAG_IGNORE_CERT_DATE_INVALID;
				}
				else if (_wcsicmp(flag, L"IgnoreHTTPRedirect") == 0)
				{
					m_InternetOpenUrlFlags |= INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTP;
				}
				else if (_wcsicmp(flag, L"IgnoreHTTPSRedirect") == 0)
				{
					m_InternetOpenUrlFlags |= INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTPS;
				}
				else
				{
					LogErrorF(this, L"Invalid flag: %s", flag);
				}
			}
		}
	}

	LeaveCriticalSection(&g_CriticalSection);
}

void MeasureWebParser::UpdateValue()
{
	m_Value = 0.0;  // Default "number" value to 0

	if (m_Download && !IsParsingConfigured() && m_Url.find(L'[') == std::wstring::npos)
	{
		// If no RegExp/JsonSpec is configured and the Url is not a reference to another measure
		// then download the file that is pointed by the Url

		// Run the download thread only if not running already
		if (m_DlThreadHandle == nullptr)
		{
			if (m_UpdateCounter == 0U)
			{
				StartDownloadThread();
			}

			++m_UpdateCounter;
			if (m_UpdateCounter >= m_UpdateRate)
			{
				m_UpdateCounter = 0U;
			}
		}

		// Else download the file pointed by the result string (this is done later)
	}
	else
	{
		// Otherwise assign the result from a previous parsing step
		// or fetch the URL and parse it according to the configuration

		// Make sure that the thread is not writing to the result at the same time
		EnterCriticalSection(&g_CriticalSection);

		if (!m_ResultString.empty())
		{
			m_Value = wcstod(m_ResultString.c_str(), nullptr);
		}

		LeaveCriticalSection(&g_CriticalSection);

		if (m_Url.size() > 0ULL && m_Url.find(L'[') == std::wstring::npos)
		{
			// This is not a reference; need to update.
			if (m_ThreadHandle == nullptr && m_DlThreadHandle == nullptr)
			{
				if (m_UpdateCounter == 0U)
				{
					// Launch a new thread to fetch the web data and parse the content
					unsigned int id = 0U;
					HANDLE threadHandle = (HANDLE)_beginthreadex(nullptr, 0U, NetworkThreadProc, this, 0U, &id);
					if (threadHandle)
					{
						m_ThreadHandle = threadHandle;
					}
				}

				++m_UpdateCounter;
				if (m_UpdateCounter >= m_UpdateRate)
				{
					m_UpdateCounter = 0U;
				}
			}
		}
	}
}

const WCHAR* MeasureWebParser::GetStringValue()
{
	static std::wstring s_ResultString;

	EnterCriticalSection(&g_CriticalSection);
	if (m_Download)
	{
		s_ResultString = m_DownloadedFile;
	}
	else
	{
		s_ResultString = m_ResultString;
	}
	LeaveCriticalSection(&g_CriticalSection);

	return CheckSubstitute(s_ResultString.c_str());
}

// Fetches the data from the net and parses the page
unsigned __stdcall MeasureWebParser::NetworkThreadProc(void* pParam)
{
	auto* measure = (MeasureWebParser*)pParam;
	DWORD dwSize = 0UL;

	bool isDebugging = GetRainmeter().GetDebug();
	if (isDebugging) LogDebugF(measure, L"Fetching: %s", measure->m_Url.c_str());

	BYTE* rawByteData = DownloadUrl(measure->m_Proxy.handle, measure->m_Url, measure->m_Headers, &dwSize, measure->m_InternetOpenUrlFlags);
	if (!rawByteData)
	{
		ShowError(measure, L"Fetch error");

		if (!measure->m_OnConnectErrAction.empty())
		{
			GetRainmeter().DelayedExecuteCommand(measure->m_OnConnectErrAction.c_str(), measure->GetSkin());
		}
	}
	else
	{
		if (isDebugging) LogDebugF(measure, L"Fetching: Success!");

		if (measure->m_Debug == 2)
		{
			// Dump to a file

			FILE* file = _wfopen(measure->m_DebugFileLocation.c_str(), L"wb");
			if (file)
			{
				fwrite(rawByteData, sizeof(BYTE), dwSize, file);
				fclose(file);
			}
			else
			{
				LogErrorF(measure, L"Failed to dump debug data");
			}
		}

		// Convert data to UTF16 string data for parsing
		std::wstring buffer;
		auto data = (const WCHAR *)rawByteData;
		DWORD dataLength = dwSize / 2;

		const int UTF16_CODEPAGE = 1200;
		if (measure->m_Codepage != UTF16_CODEPAGE)
		{
			buffer = StringUtil::Widen((LPCSTR)data, dwSize, measure->m_Codepage);
			data = buffer.c_str();
			dataLength = (DWORD)buffer.length();
		}

		if (isDebugging) LogDebugF(measure, L"Parsing data...");
		measure->ParseData(data, dataLength);
		if (isDebugging) LogDebugF(measure, L"Parsing data...done!");

		free(rawByteData);
		rawByteData = nullptr;
	}

	EnterCriticalSection(&g_CriticalSection);
	CloseHandle(measure->m_ThreadHandle);
	measure->m_ThreadHandle = 0;
	LeaveCriticalSection(&g_CriticalSection);

	return 0;   // thread completed successfully
}

void MeasureWebParser::ClearResult()
{
	EnterCriticalSection(&g_CriticalSection);
	m_ResultString.clear();
	if (m_Download)
	{
		if (m_DownloadFile.empty()) // cache mode
		{
			if (!m_DownloadedFile.empty())
			{
				// Delete old downloaded file
				DeleteFile(m_DownloadedFile.c_str());
			}
		}
		m_DownloadedFile.clear();
	}
	LeaveCriticalSection(&g_CriticalSection);
}

void MeasureWebParser::ParseData(const WCHAR *data, DWORD dataLength)
{
	bool error = false;
	if (m_ParseMode == ParseMode::Regex)
	{
		error = ParseDataRegex(data, dataLength);
	}
	else if (m_ParseMode == ParseMode::Json)
	{
		error = ParseDataJson(data, dataLength);
	}
	else
	{
		LogErrorF(this, L"Unknown parse mode: %d", m_ParseMode);
	}

	if (m_Download)
	{
		StartDownloadThread();
	}

	if (error && !m_OnParseErrAction.empty())
	{
		GetRainmeter().DelayedExecuteCommand(m_OnParseErrAction.c_str(), GetSkin());
	}
	else if (!m_Download && !m_FinishAction.empty())
	{
		GetRainmeter().DelayedExecuteCommand(m_FinishAction.c_str(), GetSkin());
	}
}

bool MeasureWebParser::ParseDataJson(const WCHAR *data, DWORD dataLength)
{
	bool error = false;

	const auto numJsonSpecs = static_cast<int>(m_JsonValueSpecs.size());
	std::vector<std::wstring> matches;
	matches.reserve(numJsonSpecs+1); // index 0 == raw data, others are matches
	matches.emplace_back(std::wstring{data, dataLength});

	// Parse the json and perform the matches
	const auto json = json::parse(std::wstring(data, dataLength), /*cb=*/nullptr, /*allow_exceptions=*/false);
	if(!json.is_discarded()) {
		// Got valid JSON, parse all specs accordingly
		// Config name options are starting with index 1 (index 0 is implied and is the entire source string)
		int jsonSpecIndex = 1;
		for (const auto &specUtf16 : m_JsonValueSpecs)
		{
			const auto specUtf8 = StringUtil::NarrowUTF8(specUtf16);
			std::wstring matchUtf16;
			const auto jsonPointer = json::json_pointer(specUtf8);
			if (json.contains(jsonPointer)) {
				const auto match = json[jsonPointer];
				const auto matchStr = match.dump();
				matchUtf16 = StringUtil::Widen(matchStr);
			} else {
				matchUtf16 = L"N/A";
			}
			matches.push_back(matchUtf16);

			if (GetRainmeter().GetDebug() && m_Debug != 0)
			{
				const auto jsonSpecConfigName = L"JsonValuePath" + std::to_wstring(jsonSpecIndex);
				LogNoticeF(this, L"Json match %s: %s = %s", jsonSpecConfigName.c_str(), specUtf16.c_str(), matchUtf16.c_str());
			}
			jsonSpecIndex++;
		}
	}

	const auto numMatches = static_cast<int>(matches.size());

	// First determine the value for the match according to the StringIndex defined on this measure
	if (m_StringIndex < numMatches)
	{
		EnterCriticalSection(&g_CriticalSection);
		m_ResultString.assign(matches.at(m_StringIndex));
		CharacterEntityReference::Decode(m_ResultString, m_DecodeCharacterReference, m_DecodeCodePoints);
		LeaveCriticalSection(&g_CriticalSection);
	}
	else
	{
		if (m_LogSubstringErrors)
			LogWarningF(this, L"Not enough JSON matches");

		error = true;

		ClearResult();
	}

	// Then update the references to this measure, e.g. inject the parsed matches into the other measures
	// that reference this one according to their StringIndex property
	const std::wstring thisMeasureReference = L"[" + GetOriginalName() + L"]";
	auto referencesThisMeasurePredicate = [&thisMeasureReference, this](MeasureWebParser *parser)
	{
		return GetSkin() == parser->GetSkin() &&
			   StringUtil::CaseInsensitiveFind(parser->m_Url, thisMeasureReference) != std::wstring::npos;
	};
	auto updateOtherMeasure = [&matches, &thisMeasureReference, this](MeasureWebParser *otherParser)
	{
		if (static_cast<size_t>(otherParser->m_StringIndex) < matches.size())
		{
			auto match = matches.at(otherParser->m_StringIndex);
			otherParser->ProcessMatch(match, thisMeasureReference);
		}
		else
		{
			if (m_LogSubstringErrors)
				LogWarningF(otherParser, L"Not enough substrings");

			otherParser->ClearResult();
		}
	};

	for_each_if(g_Measures.begin(), g_Measures.end(), referencesThisMeasurePredicate, updateOtherMeasure);

	return error;
}

/**
 * Takes the string data, applies the regex, evalutes the matches
 * and updates the values of any measures that refence this one
 * according to their defined StringIndex.
 */
bool MeasureWebParser::ParseDataRegex(const WCHAR *data, DWORD dataLength)
{
	const char *error = nullptr;
	int erroffset = 0;
	int ovector[OVECCOUNT] = {0};
	int rc = 0;
	bool doErrorAction = false;

	// Compile the regular expression in the first argument
	pcre16 *re = pcre16_compile(
		(PCRE_SPTR16)m_RegExp.c_str(),
		PCRE_UTF16, &error, &erroffset, nullptr);
	if (re != nullptr)
	{
		// Compilation succeeded: match the subject in the second argument
		rc = pcre16_exec(re, nullptr, (PCRE_SPTR16)data, dataLength, 0, 0, ovector, OVECCOUNT);
		if (rc >= 0)
		{
			if (rc == 0)
			{
				// The output vector wasn't big enough
				LogErrorF(this, L"Too many substrings");
			}
			else
			{
				// First determine the value for the match according to the StringIndex defined on this measure
				if (m_StringIndex < rc)
				{
					if (GetRainmeter().GetDebug() && m_Debug != 0)
					{
						for (int i = 0; i < rc; ++i)
						{
							const WCHAR* match = data + ovector[2 * i];
							const int matchLen = min(ovector[2 * i + 1] - ovector[2 * i], 256);
							LogDebugF(this, L"Index %2d: %.*s", i, matchLen, match);
						}
					}

					const WCHAR* match = data + ovector[2 * m_StringIndex];
					int matchLen = ovector[2 * m_StringIndex + 1] - ovector[2 * m_StringIndex];
					EnterCriticalSection(&g_CriticalSection);
					m_ResultString.assign(match, matchLen);
					CharacterEntityReference::Decode(m_ResultString, m_DecodeCharacterReference, m_DecodeCodePoints);
					LeaveCriticalSection(&g_CriticalSection);
				}
				else
				{
					if (m_LogSubstringErrors) LogWarningF(this, L"Not enough substrings");

					// Clear the old result
					ClearResult();
				}

				// Then update the references to this measure, e.g. inject the parsed matches into the other measures
				// that reference this one according to their StringIndex property
				const std::wstring thisMeasureReference = L"[" + GetOriginalName() + L"]";
				for (auto &measure : g_Measures)
				{
					if (GetSkin() == measure->GetSkin() &&
						StringUtil::CaseInsensitiveFind(measure->m_Url, thisMeasureReference) != std::wstring::npos)
					{
						if (measure->m_StringIndex < rc)
						{
							const WCHAR *match = data + ovector[2 * measure->m_StringIndex];
							int matchLen = ovector[2 * measure->m_StringIndex + 1] - ovector[2 * measure->m_StringIndex];
							std::wstring matchStr(match, matchLen);
							measure->ProcessMatch(matchStr, thisMeasureReference);
						}
						else
						{
							if (m_LogSubstringErrors) LogWarningF(measure, L"Not enough substrings");

							// Clear the old result
							measure->ClearResult();
						}
					}
				}
			}
		}
		else
		{
			// Matching failed: handle error cases
			LogErrorF(this, L"RegExp matching error (%d)", rc);
			doErrorAction = true;

			EnterCriticalSection(&g_CriticalSection);
			m_ResultString = m_ErrorString;

			// Update the references
			const std::wstring thisMeasureReference = L"[" + GetOriginalName() + L"]";
			for (auto &measure : g_Measures)
			{
				if ((StringUtil::CaseInsensitiveFind(measure->m_Url, thisMeasureReference) != std::wstring::npos) &&
					(GetSkin() == measure->GetSkin()))
				{
					measure->m_ResultString = measure->m_ErrorString;
				}
			}
			LeaveCriticalSection(&g_CriticalSection);
		}

		// Release memory used for the compiled pattern
		pcre16_free(re);
	}
	else
	{
		// Compilation failed.
		LogErrorF(this, L"RegExp error at offset %d: %S", erroffset, error);
		doErrorAction = true;
	}

	return doErrorAction;
}

void MeasureWebParser::ProcessMatch(const std::wstring& match, const std::wstring& parentMeasureReference)
{
	if (IsParsingConfigured())
	{
		// Change the index and parse the substring
		int index = m_StringIndex;
		m_StringIndex = m_StringIndex2;
		ParseData(match.c_str(), match.length());
		m_StringIndex = index;
	}
	else
	{
		// Set the result
		EnterCriticalSection(&g_CriticalSection);

		// Substitude the [measure] with result
		m_ResultString = m_Url;
		m_ResultString.replace(
			StringUtil::CaseInsensitiveFind(m_ResultString, parentMeasureReference),
			parentMeasureReference.size(), match);
		CharacterEntityReference::Decode(m_ResultString, m_DecodeCharacterReference, m_DecodeCodePoints);

		// Start download threads for the references in case a child measure wants to download
		// an Url that is based on the match of the parent
		if (m_Download)
		{
			StartDownloadThread();
		}

		LeaveCriticalSection(&g_CriticalSection);
	}
}

void MeasureWebParser::StartDownloadThread()
{
	// Run the thread only if it is not running already
	EnterCriticalSection(&g_CriticalSection);
	if (m_DlThreadHandle == nullptr)
	{
		unsigned int id = 0U;
		HANDLE threadHandle = (HANDLE)_beginthreadex(nullptr, 0U, NetworkDownloadThreadProc, this, 0U, &id);
		if (threadHandle)
		{
			m_DlThreadHandle = threadHandle;
		}
	}
	LeaveCriticalSection(&g_CriticalSection);
}

bool MeasureWebParser::IsParsingConfigured() const
{
	return !m_RegExp.empty() || !m_JsonValueSpecs.empty();
}

// Downloads file from the net
unsigned __stdcall MeasureWebParser::NetworkDownloadThreadProc(void* pParam)
{
	auto* measure = (MeasureWebParser*)pParam;
	const bool download = !measure->m_DownloadFile.empty();
	bool ready = false;

	std::wstring url;

	// If there is no parsing configured and the Url is not a reference
	// then use the Url for download
	if (!measure->IsParsingConfigured() && measure->m_ResultString.empty())
	{
		if (!measure->m_Url.empty() && measure->m_Url[0] != L'[')
		{
			url = measure->m_Url;
		}
	}
	// Otherwise take the result string from the measure and treat it as the Url,
	// e.g. this measure references another one and the match should be treated
	// as an Url for download
	else
	{
		EnterCriticalSection(&g_CriticalSection);
		url = measure->m_ResultString;
		LeaveCriticalSection(&g_CriticalSection);

		std::wstring::size_type pos = url.find(L':');
		if (pos == std::wstring::npos && !url.empty())	// No protocol
		{
			// Add the base url to the string
			if (url[0] == L'/')
			{
				// Absolute path
				pos = measure->m_Url.find(L'/', 7ULL);	// Assume "http://" (=7)
				if (pos != std::wstring::npos)
				{
					std::wstring path(measure->m_Url.substr(0ULL, pos));
					url = path + url;
				}
			}
			else
			{
				// Relative path

				pos = measure->m_Url.rfind(L'/');
				if (pos != std::wstring::npos)
				{
					std::wstring path(measure->m_Url.substr(0ULL, pos + 1ULL));
					url = path + url;
				}
			}
		}
	}

	if (!url.empty())
	{
		// Create the filename
		WCHAR buffer[MAX_PATH] = { 0 };
		std::wstring fullpath, directory;

		if (download)  // download mode
		{
			PathCanonicalize(buffer, measure->m_DownloadFile.c_str());

			std::wstring path = buffer;
			std::wstring::size_type pos = path.find_first_not_of(L'\\');
			if (pos != std::wstring::npos)
			{
				path.erase(0UL, pos);
			}

			PathCanonicalize(buffer, measure->m_DownloadFolder.c_str());
			CreateDirectory(buffer, nullptr);	// Make sure that the folder exists

			wcscat(buffer, path.c_str());

			if (buffer[wcslen(buffer) - 1ULL] != L'\\')  // path is a file
			{
				fullpath = buffer;
				PathRemoveFileSpec(buffer);
			}
			PathAddBackslash(buffer);
		}
		else  // cache mode
		{
			GetTempPath(_countof(buffer), buffer);
			wcscat(buffer, L"Rainmeter-Cache\\");  // "%TEMP%\Rainmeter-Cache\"
		}
		CreateDirectory(buffer, nullptr);	// Make sure that the folder exists
		directory = buffer;

		if (fullpath.empty())
		{
			fullpath = directory;

			std::wstring::size_type pos2 = url.find_first_of(L"?#");
			std::wstring::size_type pos1 = url.find_last_of(L'/', pos2);
			pos1 = (pos1 != std::wstring::npos) ? pos1 + 1ULL : 0ULL;

			std::wstring name;
			if (pos2 != std::wstring::npos)
			{
				name.assign(url, pos1, pos2 - pos1);
			}
			else
			{
				name.assign(url, pos1, url.length() - pos1);
			}

			if (!name.empty())
			{
				// Replace reserved characters to "_"
				pos1 = 0ULL;
				while ((pos1 = name.find_first_of(L"\\/:*?\"<>|", pos1)) != std::wstring::npos)
				{
					name[pos1] = L'_';
				}
				fullpath += name;
			}
			else
			{
				fullpath += L"index";
			}
		}

		ready = true;

		if (download)  // download mode
		{
			if (!PathFileExists(directory.c_str()) || !PathIsDirectory(directory.c_str()))
			{
				ready = false;
				LogErrorF(measure, L"Directory does not exist: %s", directory.c_str());
			}
			else if (PathIsDirectory(fullpath.c_str()))
			{
				ready = false;
				LogErrorF(measure, L"Path is a directory, not a file: %s", fullpath.c_str());
			}
			else if (PathFileExists(fullpath.c_str()))
			{
				DWORD attr = GetFileAttributes(fullpath.c_str());
				if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_READONLY))
				{
					ready = false;
					LogErrorF(measure, L"File is read-only: %s", fullpath.c_str());
				}
			}
		}
		else  // cache mode
		{
			EnterCriticalSection(&g_CriticalSection);

			if (PathFileExists(fullpath.c_str()))
			{
				std::wstring::size_type pos = fullpath.find_last_of(L'.');

				std::wstring path, ext;
				if (pos != std::wstring::npos)
				{
					path.assign(fullpath, 0, pos);
					ext.assign(fullpath, pos, fullpath.length() - pos);
				}
				else
				{
					path = fullpath;
				}

				// Assign a serial number
				int i = 1;
				do
				{
					_snwprintf_s(buffer, _countof(buffer), L"_%d", i++);

					fullpath = path;
					fullpath += buffer;
					if (!ext.empty())
					{
						fullpath += ext;
					}
				} while (PathFileExists(fullpath.c_str()));
			}

			// Create empty file
			HANDLE hFile = CreateFile(fullpath.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
			if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);

			LeaveCriticalSection(&g_CriticalSection);
		}

		if (ready)
		{
			// Delete IE cache before download if "SyncMode5" is not 3 (every visit to the page)
			{
				// Check "Temporary Internet Files" sync mode (SyncMode5)
				// Values:
				//   Every visit to the page                 3
				//   Every time you start Internet Explorer  2
				//   Automatically (default)                 4
				//   Never                                   0
				// http://support.microsoft.com/kb/263070/en

				HKEY hKey = nullptr;
				LONG ret = 0L;
				DWORD mode = 0UL;

				ret = RegOpenKeyEx(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings", 0, KEY_QUERY_VALUE, &hKey);
				if (ret == ERROR_SUCCESS)
				{
					DWORD size = sizeof(mode);
					ret = RegQueryValueEx(hKey, L"SyncMode5", nullptr, nullptr, (LPBYTE)&mode, &size);
					RegCloseKey(hKey);
					hKey = nullptr;
				}

				if (ret != ERROR_SUCCESS || mode != 3)
				{
					std::wstring::size_type pos = url.find_first_of(L'#');

					if (pos != std::wstring::npos)
					{
						DeleteUrlCacheEntry(url.substr(0, pos).c_str());
					}
					else
					{
						DeleteUrlCacheEntry(url.c_str());
					}
				}
			}

			if (GetRainmeter().GetDebug())
			{
				LogDebugF(measure, L"Downloading url '%s' to: %s", url.c_str(), fullpath.c_str());
			}

			HRESULT resultCoInitialize = CoInitialize(nullptr);  // requires before calling URLDownloadToFile function

			// Download the file
			HRESULT result = URLDownloadToFile(nullptr, url.c_str(), fullpath.c_str(), 0, nullptr);
			if (result == S_OK)
			{
				EnterCriticalSection(&g_CriticalSection);

				if (!download)  // cache mode
				{
					if (!measure->m_DownloadedFile.empty())
					{
						// Delete old downloaded file
						DeleteFile(measure->m_DownloadedFile.c_str());
					}
				}

				// Convert LFN to 8.3 filename if the path contains blank character
				if (fullpath.find_first_of(L' ') != std::wstring::npos)
				{
					DWORD size = GetShortPathName(fullpath.c_str(), buffer, _countof(buffer));
					if (size > 0UL && size <= _countof(buffer))
					{
						fullpath = buffer;
					}
				}
				measure->m_DownloadedFile = fullpath;

				LeaveCriticalSection(&g_CriticalSection);

				if (!measure->m_FinishAction.empty())
				{
					GetRainmeter().DelayedExecuteCommand(measure->m_FinishAction.c_str(), measure->GetSkin());
				}
			}
			else
			{
				ready = false;

				if (!download)  // cache mode
				{
					// Delete empty file
					DeleteFile(fullpath.c_str());
				}

				LogErrorF(
					measure,
					L"Download failed (res=0x%08X, COM=0x%08X): %s",
					result, resultCoInitialize, url.c_str());

				if (!measure->m_OnDownloadErrAction.empty())
				{
					GetRainmeter().DelayedExecuteCommand(measure->m_OnDownloadErrAction.c_str(), measure->GetSkin());
				}
			}

			if (SUCCEEDED(resultCoInitialize))
			{
				CoUninitialize();
			}
		}
		else
		{
			LogErrorF(measure, L"Download failed: %s", url.c_str());

			if (!measure->m_OnDownloadErrAction.empty())
			{
				GetRainmeter().DelayedExecuteCommand(measure->m_OnDownloadErrAction.c_str(), measure->GetSkin());
			}
		}
	}
	else
	{
		LogErrorF(measure, L"Url is empty");
	}

	if (!ready) // download failed
	{
		EnterCriticalSection(&g_CriticalSection);

		if (!download) // cache mode
		{
			if (!measure->m_DownloadedFile.empty())
			{
				// Delete old downloaded file
				DeleteFile(measure->m_DownloadedFile.c_str());
			}
		}

		// Clear old downloaded filename
		measure->m_DownloadedFile.clear();

		LeaveCriticalSection(&g_CriticalSection);
	}

	EnterCriticalSection(&g_CriticalSection);
	CloseHandle(measure->m_DlThreadHandle);
	measure->m_DlThreadHandle = 0;
	LeaveCriticalSection(&g_CriticalSection);

	return 0U;   // thread completed successfully
}

/*
	Downloads the given url and returns the webpage as dynamically allocated string.
	You need to free the returned string after use!
*/
BYTE* DownloadUrl(HINTERNET handle, std::wstring& url, std::wstring& headers, DWORD* dataSize, DWORD flags)
{
	if (_wcsnicmp(url.c_str(), L"file://", 7ULL) == 0)  // Local file
	{
		WCHAR path[MAX_PATH] = { 0 };
		DWORD pathLength = _countof(path);
		HRESULT hr = PathCreateFromUrl(url.c_str(), path, &pathLength, 0);
		if (FAILED(hr))
		{
			return nullptr;
		}

		size_t fileSize = 0ULL;
		BYTE* buffer = FileUtil::ReadFullFile(path, &fileSize).release();
		*dataSize = (DWORD)fileSize;

		return buffer;
	}

	{
		URL_COMPONENTS components = { 0 };
		components.dwStructSize = sizeof(components);
		components.dwExtraInfoLength = ULONG_MAX;
		if (InternetCrackUrl(url.c_str(), static_cast<DWORD>(url.size()), 0, &components))
		{
			if (components.lpszExtraInfo && components.dwExtraInfoLength > 0ULL)
			{
				size_t position = url.find(components.lpszExtraInfo);  // Only percent encode characters in the query or fragment part of the URL
				if (position != std::wstring::npos)
				{
					std::wstring extra = url.substr(position);
					StringUtil::EncodeUrl(extra, false);  // Only percent encode spaces, control characters, and non-ascii characters (HEX: 80-255)

					url.erase(position);
					url.append(extra);
				}
			}
		}
	}

	HINTERNET hUrlDump = InternetOpenUrl(handle, url.c_str(), headers.c_str(), -1L, flags, 0);
	if (!hUrlDump)
	{
		return nullptr;
	}

	// Allocate buffer with 3 extra bytes for triple null termination in case the string is
	// invalid (e.g. when incorrectly using the UTF-16LE codepage for the data).
	const DWORD CHUNK_SIZE = 8192UL;
	DWORD bufferSize = CHUNK_SIZE;
	BYTE* buffer = (BYTE*)malloc(bufferSize + 3UL);
	*dataSize = 0UL;

	// Read the data.
	do
	{
		DWORD readSize = 0UL;
		if (!InternetReadFile(hUrlDump, buffer + *dataSize, bufferSize - *dataSize, &readSize))
		{
			free(buffer);
			buffer = nullptr;
			InternetCloseHandle(hUrlDump);
			return nullptr;
		}
		else if (readSize == 0UL)
		{
			// All data read.
			break;
		}

		*dataSize += readSize;

		bufferSize += CHUNK_SIZE;

		BYTE* oldBuffer = buffer;
		if ((buffer = (BYTE*)realloc(buffer, bufferSize + 3UL)) == nullptr)
		{
			free(oldBuffer);  // In case realloc fails
			oldBuffer = nullptr;
			InternetCloseHandle(hUrlDump);
			return nullptr;
		}
	}
	while (true);

	InternetCloseHandle(hUrlDump);

	// Triple null terminate the buffer.
	buffer[*dataSize] = 0;
	buffer[*dataSize + 1] = 0;
	buffer[*dataSize + 2] = 0;

	return buffer;
}

/*
  Writes the last error to log.
*/
void ShowError(MeasureWebParser* measure, WCHAR* description)
{
	DWORD dwErr = GetLastError();
	if (dwErr == ERROR_INTERNET_EXTENDED_ERROR)
	{
		WCHAR szBuffer[1024];
		DWORD dwError, dwLen = 1024;
		const WCHAR* error = L"Unknown error";
		if (InternetGetLastResponseInfo(&dwError, szBuffer, &dwLen))
		{
			error = szBuffer;
			dwErr = dwError;
		}

		LogErrorF(measure, L"(%s) %s (ErrorCode=%i)", description, error, dwErr);
	}
	else
	{
		LPVOID lpMsgBuf = nullptr;

		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_HMODULE |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS |
			FORMAT_MESSAGE_MAX_WIDTH_MASK,
			GetModuleHandle(L"wininet"),
			dwErr,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			nullptr
		);

		const WCHAR* error = lpMsgBuf ? (WCHAR*)lpMsgBuf : L"Unknown error";
		LogErrorF(measure, L"(%s) %s (ErrorCode=%i)", description, error, dwErr);

		if (lpMsgBuf) LocalFree(lpMsgBuf);
	}
}

void MeasureWebParser::Command(const std::wstring& command)
{
	const WCHAR* args = command.c_str();

	// Kill the threads (if any) and reset the update counter
	if (_wcsicmp(args, L"UPDATE") == 0)
	{
		if (m_ThreadHandle)
		{
			// Thread is killed inside critical section so that itself is not inside one when it is terminated
			EnterCriticalSection(&g_CriticalSection);

			TerminateThread(m_ThreadHandle, 0UL);
			m_ThreadHandle = nullptr;

			LeaveCriticalSection(&g_CriticalSection);
		}

		if (m_DlThreadHandle)
		{
			// Thread is killed inside critical section so that itself is not inside one when it is terminated
			EnterCriticalSection(&g_CriticalSection);

			TerminateThread(m_DlThreadHandle, 0UL);
			m_DlThreadHandle = nullptr;

			LeaveCriticalSection(&g_CriticalSection);
		}

		m_UpdateCounter = 0U;
	}
	else if (_wcsicmp(args, L"RESET") == 0)
	{
		m_ResultString.clear();
		m_DownloadedFile.clear();

		EnterCriticalSection(&g_CriticalSection);

		// Update the references
		auto i = g_Measures.begin();
		std::wstring compareStr = L"[";
		compareStr += GetOriginalName();
		compareStr += L']';
		for (; i != g_Measures.end(); ++i)
		{
			if ((StringUtil::CaseInsensitiveFind((*i)->m_Url, compareStr) != std::wstring::npos) &&
				(GetSkin() == (*i)->GetSkin()))
			{
				(*i)->m_ResultString.clear();
				(*i)->m_DownloadedFile.clear();
			}
		}
		LeaveCriticalSection(&g_CriticalSection);
	}
}
