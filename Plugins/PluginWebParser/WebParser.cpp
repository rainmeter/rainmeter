/*
  Copyright (C) 2005 Kimmo Pekkola

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

#include <windows.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <Wininet.h>
#include <shlwapi.h>
#include <process.h>
#include "../../Library/pcre-8.10/config.h"
#include "../../Library/pcre-8.10/pcre.h"
#include "../../Common/StringUtil.h"
#include "../API/RainmeterAPI.h"

void ShowError(int lineNumber, WCHAR* errorMsg = nullptr);

class ProxyCachePool
{
public:
	ProxyCachePool(LPCWSTR globalProxyName = nullptr) :
		m_GlobalProxyName((globalProxyName && *globalProxyName) ? globalProxyName : L"/auto")
	{
		m_GlobalProxyCache = new ProxyCache(CreateProxy(m_GlobalProxyName.c_str()), true);

		_wcslwr(&m_GlobalProxyName[0]);
		m_CacheMap[m_GlobalProxyName] = m_GlobalProxyCache;
		//DebugLog(L"* ADD-GLOBAL: key=%s, handle=0x%p, ref=new", m_GlobalProxyName.c_str(), m_GlobalProxyCache->GetCache());
	}

	~ProxyCachePool()
	{
		for (auto iter = m_CacheMap.begin(); iter != m_CacheMap.end(); ++iter)
		{
			ProxyCache* cache = (*iter).second;
			//DebugLog(L"* FORCE-REMOVE: key=%s, global=%i, ref=%i", (*iter).first.c_str(), cache->IsGlobal(), cache->GetRef());
			delete cache;
		}
	}

	HINTERNET GetCache(const std::wstring& proxyName)
	{
		ProxyCache* cache = nullptr;

		if (proxyName.empty())
		{
			// Use global proxy setting
			cache = m_GlobalProxyCache;
		}
		else
		{
			std::wstring key = proxyName;
			_wcslwr(&key[0]);

			auto iter = m_CacheMap.find(key);
			if (iter != m_CacheMap.end())
			{
				cache = (*iter).second;
			}
			else  // cache not found
			{
				// Create new proxy
				ProxyCache* cache = new ProxyCache(CreateProxy(proxyName.c_str()));
				m_CacheMap[key] = cache;
				//DebugLog(L"* ADD: key=%s, handle=0x%p, ref=new", key.c_str(), cache->GetCache());
				return cache->GetCache();
			}
		}

		// Use proxy cache
		cache->AddRef();
		//DebugLog(L"* ADD-REF: key=%s, handle=0x%p, global=%i, ref=%i",
		//	cache->IsGlobal() ? m_GlobalProxyName.c_str() : proxyName.c_str(), cache->GetCache(), cache->IsGlobal(), cache->GetRef());
		return cache->GetCache();
	}

	void RemoveCache(const std::wstring& proxyName)
	{
		std::wstring key = proxyName.empty() ? m_GlobalProxyName : proxyName;

		if (!proxyName.empty())
		{
			_wcslwr(&key[0]);
		}

		auto iter = m_CacheMap.find(key);
		if (iter != m_CacheMap.end())
		{
			ProxyCache* cache = (*iter).second;
			cache->Release();
			//DebugLog(L"* REMOVE: key=%s, global=%i, ref=%i", key.c_str(), cache->IsGlobal(), cache->GetRef());

			if (cache->IsInvalid())
			{
				//DebugLog(L"* EMPTY-ERASE: key=%s", key.c_str());
				m_CacheMap.erase(iter);
				delete cache;
			}
		}
	}

private:
	HINTERNET CreateProxy(LPCWSTR proxyName)
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

		HINTERNET handle = InternetOpen(L"Rainmeter WebParser plugin",
			proxyType,
			proxyServer,
			nullptr,
			0);

		if (handle)
		{
			WCHAR buffer[256];
			_snwprintf_s(buffer, _TRUNCATE, L"WebParser: ProxyServer=\"%s\" (type=%s, handle=0x%p)",
				proxyName,
				proxyType == INTERNET_OPEN_TYPE_PRECONFIG ? L"PRECONFIG" : proxyType == INTERNET_OPEN_TYPE_DIRECT ? L"DIRECT" : L"PROXY",
				handle);
			RmLog(LOG_DEBUG, buffer);
		}
		else
		{
			ShowError(__LINE__);
		}

		return handle;
	}

	class ProxyCache
	{
	public:
		ProxyCache(HINTERNET handle, bool isGlobal = false) : m_Handle(handle), m_IsGlobal(isGlobal), m_Ref(1) {}
		~ProxyCache() { Dispose(); }

		void AddRef() { if (!IsInvalid()) { ++m_Ref; } }
		void Release() { if (m_Ref > 0) { --m_Ref; } if (IsInvalid()) { Dispose(); } }

		bool IsGlobal() { return m_IsGlobal; }
		bool IsInvalid() { return (m_Ref <= 0 && !IsGlobal()); }
		//int GetRef() { return m_Ref; }
		HINTERNET GetCache() { return m_Handle; }

	private:
		ProxyCache() {}
		ProxyCache(const ProxyCache& cache) {}

		void Dispose() { if (m_Handle) { InternetCloseHandle(m_Handle); m_Handle = nullptr; } }

		HINTERNET m_Handle;
		bool m_IsGlobal;
		int m_Ref;
	};

	std::unordered_map<std::wstring, ProxyCache*> m_CacheMap;
	ProxyCache* m_GlobalProxyCache;
	std::wstring m_GlobalProxyName;
};

struct ProxySetting
{
	std::wstring server;
	HINTERNET handle;

	ProxySetting() : handle() {}
};

struct MeasureData
{
	std::wstring url;
	std::wstring regExp;
	std::wstring resultString;
	std::wstring errorString;
	std::wstring finishAction;
	std::wstring downloadFolder;
	std::wstring downloadFile;
	std::wstring downloadedFile;
	std::wstring debugFileLocation;
	LPCWSTR section;
	void* skin;
	ProxySetting proxy;
	HANDLE threadHandle;
	HANDLE dlThreadHandle;
	int codepage;
	int stringIndex;
	int stringIndex2;
	int decodeCharacterReference;
	int debug;
	UINT updateRate;
	UINT updateCounter;
	bool download;
	bool forceReload;

	MeasureData() :
		skin(),
		threadHandle(),
		dlThreadHandle(),
		codepage(),
		stringIndex(),
		stringIndex2(),
		decodeCharacterReference(),
		debug(),
		updateRate(),
		updateCounter(),
		download(),
		forceReload()
	{
	}
};

BYTE* DownloadUrl(HINTERNET handle, std::wstring& url, DWORD* dataSize, bool forceReload);
unsigned __stdcall NetworkThreadProc(void* pParam);
unsigned __stdcall NetworkDownloadThreadProc(void* pParam);
void ParseData(MeasureData* measure, LPCSTR parseData, DWORD dwSize);

CRITICAL_SECTION g_CriticalSection;
ProxyCachePool* g_ProxyCachePool = nullptr;
UINT g_InstanceCount = 0;

static std::vector<MeasureData*> g_Measures;
static bool g_Debug = false;

static std::unordered_map<std::wstring, WCHAR> g_CERs;

#define OVECCOUNT 300    // should be a multiple of 3

std::string ConvertAsciiToUTF8(LPCSTR str, int strLen, int codepage)
{
	std::string szUTF8;

	if (str && *str)
	{
		std::wstring wide = StringUtil::Widen(str, strLen, codepage);
		if (!wide.empty())
		{
			szUTF8.swap(StringUtil::NarrowUTF8(wide));
		}
	}
	return szUTF8;
}

void DecodeReferences(std::wstring& str, int opt)
{
	// (opt <= 0 || opt > 3) : Do nothing.
	// (opt == 1)            : Decode both numeric character references and character entity references.
	// (opt == 2)            : Decode only numeric character references.
	// (opt == 3)            : Decode only character entity references.

	if (opt >= 1 && opt <= 3)
	{
		std::wstring::size_type start = 0;

		while ((start = str.find(L'&', start)) != std::wstring::npos)
		{
			std::wstring::size_type end, pos;

			if ((end = str.find(L';', start)) == std::wstring::npos) break;
			pos = start + 1;

			if (pos == end)  // &; - skip
			{
				start = end + 1;
				continue;
			}
			else if ((end - pos) > 10)  // name (or number) is too long
			{
				++start;
				continue;
			}

			if (str[pos] == L'#')  // Numeric character reference
			{
				if (opt == 3 ||    // Decode only character entity references,
					++pos == end)  // &#; - skip
				{
					start = end + 1;
					continue;
				}

				int base;
				if (str[pos] == L'x' || str[pos] == L'X')
				{
					if (++pos == end)  // &#x; or &#X; - skip
					{
						start = end + 1;
						continue;
					}
					base = 16;
				}
				else
				{
					base = 10;
				}

				std::wstring num(str, pos, end - pos);
				WCHAR* pch = nullptr;
				errno = 0;
				long ch = wcstol(num.c_str(), &pch, base);
				if (pch == nullptr || *pch != L'\0' || errno == ERANGE || ch <= 0 || ch >= 0xFFFE)  // invalid character
				{
					start = pos;
					continue;
				}
				str.replace(start, end - start + 1, 1, (WCHAR)ch);
				++start;
			}
			else  // Character entity reference
			{
				if (opt == 2)  // Decode only numeric character references - skip
				{
					start = end + 1;
					continue;
				}

				std::wstring name(str, pos, end - pos);

				std::unordered_map<std::wstring, WCHAR>::const_iterator iter = g_CERs.find(name);
				if (iter != g_CERs.end())
				{
					str.replace(start, end - start + 1, 1, (*iter).second);
				}
				++start;
			}
		}
	}
}

void FillCharacterEntityReferences()
{
	struct CER
	{
		WCHAR* name;
		WCHAR ch;
	};

	// List from:
	// http://www.w3.org/TR/html4/sgml/entities.html
	// http://www.w3.org/TR/xhtml1/#C_16
	static CER entities[] =
	{
		// for markup-significant and internationalization characters
		{ L"quot",		(WCHAR)34 },
		{ L"amp",		(WCHAR)38 },
		{ L"apos",		(WCHAR)39 },
		{ L"lt",		(WCHAR)60 },
		{ L"gt",		(WCHAR)62 },
		{ L"OElig",		(WCHAR)338 },
		{ L"oelig",		(WCHAR)339 },
		{ L"Scaron",	(WCHAR)352 },
		{ L"scaron",	(WCHAR)353 },
		{ L"Yuml",		(WCHAR)376 },
		{ L"circ",		(WCHAR)710 },
		{ L"tilde",		(WCHAR)732 },
		{ L"ensp",		(WCHAR)8194 },
		{ L"emsp",		(WCHAR)8195 },
		{ L"thinsp",	(WCHAR)8201 },
		{ L"zwnj",		(WCHAR)8204 },
		{ L"zwj",		(WCHAR)8205 },
		{ L"lrm",		(WCHAR)8206 },
		{ L"rlm",		(WCHAR)8207 },
		{ L"ndash",		(WCHAR)8211 },
		{ L"mdash",		(WCHAR)8212 },
		{ L"lsquo",		(WCHAR)8216 },
		{ L"rsquo",		(WCHAR)8217 },
		{ L"sbquo",		(WCHAR)8218 },
		{ L"ldquo",		(WCHAR)8220 },
		{ L"rdquo",		(WCHAR)8221 },
		{ L"bdquo",		(WCHAR)8222 },
		{ L"dagger",	(WCHAR)8224 },
		{ L"Dagger",	(WCHAR)8225 },
		{ L"permil",	(WCHAR)8240 },
		{ L"lsaquo",	(WCHAR)8249 },
		{ L"rsaquo",	(WCHAR)8250 },
		{ L"euro",		(WCHAR)8364 },

		// for ISO 8859-1 characters
		{ L"nbsp",		(WCHAR)160 },
		{ L"iexcl",		(WCHAR)161 },
		{ L"cent",		(WCHAR)162 },
		{ L"pound",		(WCHAR)163 },
		{ L"curren",	(WCHAR)164 },
		{ L"yen",		(WCHAR)165 },
		{ L"brvbar",	(WCHAR)166 },
		{ L"sect",		(WCHAR)167 },
		{ L"uml",		(WCHAR)168 },
		{ L"copy",		(WCHAR)169 },
		{ L"ordf",		(WCHAR)170 },
		{ L"laquo",		(WCHAR)171 },
		{ L"not",		(WCHAR)172 },
		{ L"shy",		(WCHAR)173 },
		{ L"reg",		(WCHAR)174 },
		{ L"macr",		(WCHAR)175 },
		{ L"deg",		(WCHAR)176 },
		{ L"plusmn",	(WCHAR)177 },
		{ L"sup2",		(WCHAR)178 },
		{ L"sup3",		(WCHAR)179 },
		{ L"acute",		(WCHAR)180 },
		{ L"micro",		(WCHAR)181 },
		{ L"para",		(WCHAR)182 },
		{ L"middot",	(WCHAR)183 },
		{ L"cedil",		(WCHAR)184 },
		{ L"sup1",		(WCHAR)185 },
		{ L"ordm",		(WCHAR)186 },
		{ L"raquo",		(WCHAR)187 },
		{ L"frac14",	(WCHAR)188 },
		{ L"frac12",	(WCHAR)189 },
		{ L"frac34",	(WCHAR)190 },
		{ L"iquest",	(WCHAR)191 },
		{ L"Agrave",	(WCHAR)192 },
		{ L"Aacute",	(WCHAR)193 },
		{ L"Acirc",		(WCHAR)194 },
		{ L"Atilde",	(WCHAR)195 },
		{ L"Auml",		(WCHAR)196 },
		{ L"Aring",		(WCHAR)197 },
		{ L"AElig",		(WCHAR)198 },
		{ L"Ccedil",	(WCHAR)199 },
		{ L"Egrave",	(WCHAR)200 },
		{ L"Eacute",	(WCHAR)201 },
		{ L"Ecirc",		(WCHAR)202 },
		{ L"Euml",		(WCHAR)203 },
		{ L"Igrave",	(WCHAR)204 },
		{ L"Iacute",	(WCHAR)205 },
		{ L"Icirc",		(WCHAR)206 },
		{ L"Iuml",		(WCHAR)207 },
		{ L"ETH",		(WCHAR)208 },
		{ L"Ntilde",	(WCHAR)209 },
		{ L"Ograve",	(WCHAR)210 },
		{ L"Oacute",	(WCHAR)211 },
		{ L"Ocirc",		(WCHAR)212 },
		{ L"Otilde",	(WCHAR)213 },
		{ L"Ouml",		(WCHAR)214 },
		{ L"times",		(WCHAR)215 },
		{ L"Oslash",	(WCHAR)216 },
		{ L"Ugrave",	(WCHAR)217 },
		{ L"Uacute",	(WCHAR)218 },
		{ L"Ucirc",		(WCHAR)219 },
		{ L"Uuml",		(WCHAR)220 },
		{ L"Yacute",	(WCHAR)221 },
		{ L"THORN",		(WCHAR)222 },
		{ L"szlig",		(WCHAR)223 },
		{ L"agrave",	(WCHAR)224 },
		{ L"aacute",	(WCHAR)225 },
		{ L"acirc",		(WCHAR)226 },
		{ L"atilde",	(WCHAR)227 },
		{ L"auml",		(WCHAR)228 },
		{ L"aring",		(WCHAR)229 },
		{ L"aelig",		(WCHAR)230 },
		{ L"ccedil",	(WCHAR)231 },
		{ L"egrave",	(WCHAR)232 },
		{ L"eacute",	(WCHAR)233 },
		{ L"ecirc",		(WCHAR)234 },
		{ L"euml",		(WCHAR)235 },
		{ L"igrave",	(WCHAR)236 },
		{ L"iacute",	(WCHAR)237 },
		{ L"icirc",		(WCHAR)238 },
		{ L"iuml",		(WCHAR)239 },
		{ L"eth",		(WCHAR)240 },
		{ L"ntilde",	(WCHAR)241 },
		{ L"ograve",	(WCHAR)242 },
		{ L"oacute",	(WCHAR)243 },
		{ L"ocirc",		(WCHAR)244 },
		{ L"otilde",	(WCHAR)245 },
		{ L"ouml",		(WCHAR)246 },
		{ L"divide",	(WCHAR)247 },
		{ L"oslash",	(WCHAR)248 },
		{ L"ugrave",	(WCHAR)249 },
		{ L"uacute",	(WCHAR)250 },
		{ L"ucirc",		(WCHAR)251 },
		{ L"uuml",		(WCHAR)252 },
		{ L"yacute",	(WCHAR)253 },
		{ L"thorn",		(WCHAR)254 },
		{ L"yuml",		(WCHAR)255 },

		// for symbols, mathematical symbols, and Greek letters
		{ L"fnof",		(WCHAR)402 },
		{ L"Alpha",		(WCHAR)913 },
		{ L"Beta",		(WCHAR)914 },
		{ L"Gamma",		(WCHAR)915 },
		{ L"Delta",		(WCHAR)916 },
		{ L"Epsilon",	(WCHAR)917 },
		{ L"Zeta",		(WCHAR)918 },
		{ L"Eta",		(WCHAR)919 },
		{ L"Theta",		(WCHAR)920 },
		{ L"Iota",		(WCHAR)921 },
		{ L"Kappa",		(WCHAR)922 },
		{ L"Lambda",	(WCHAR)923 },
		{ L"Mu",		(WCHAR)924 },
		{ L"Nu",		(WCHAR)925 },
		{ L"Xi",		(WCHAR)926 },
		{ L"Omicron",	(WCHAR)927 },
		{ L"Pi",		(WCHAR)928 },
		{ L"Rho",		(WCHAR)929 },
		{ L"Sigma",		(WCHAR)931 },
		{ L"Tau",		(WCHAR)932 },
		{ L"Upsilon",	(WCHAR)933 },
		{ L"Phi",		(WCHAR)934 },
		{ L"Chi",		(WCHAR)935 },
		{ L"Psi",		(WCHAR)936 },
		{ L"Omega",		(WCHAR)937 },
		{ L"alpha",		(WCHAR)945 },
		{ L"beta",		(WCHAR)946 },
		{ L"gamma",		(WCHAR)947 },
		{ L"delta",		(WCHAR)948 },
		{ L"epsilon",	(WCHAR)949 },
		{ L"zeta",		(WCHAR)950 },
		{ L"eta",		(WCHAR)951 },
		{ L"theta",		(WCHAR)952 },
		{ L"iota",		(WCHAR)953 },
		{ L"kappa",		(WCHAR)954 },
		{ L"lambda",	(WCHAR)955 },
		{ L"mu",		(WCHAR)956 },
		{ L"nu",		(WCHAR)957 },
		{ L"xi",		(WCHAR)958 },
		{ L"omicron",	(WCHAR)959 },
		{ L"pi",		(WCHAR)960 },
		{ L"rho",		(WCHAR)961 },
		{ L"sigmaf",	(WCHAR)962 },
		{ L"sigma",		(WCHAR)963 },
		{ L"tau",		(WCHAR)964 },
		{ L"upsilon",	(WCHAR)965 },
		{ L"phi",		(WCHAR)966 },
		{ L"chi",		(WCHAR)967 },
		{ L"psi",		(WCHAR)968 },
		{ L"omega",		(WCHAR)969 },
		{ L"thetasym",	(WCHAR)977 },
		{ L"upsih",		(WCHAR)978 },
		{ L"piv",		(WCHAR)982 },
		{ L"bull",		(WCHAR)8226 },
		{ L"hellip",	(WCHAR)8230 },
		{ L"prime",		(WCHAR)8242 },
		{ L"Prime",		(WCHAR)8243 },
		{ L"oline",		(WCHAR)8254 },
		{ L"frasl",		(WCHAR)8260 },
		{ L"weierp",	(WCHAR)8472 },
		{ L"image",		(WCHAR)8465 },
		{ L"real",		(WCHAR)8476 },
		{ L"trade",		(WCHAR)8482 },
		{ L"alefsym",	(WCHAR)8501 },
		{ L"larr",		(WCHAR)8592 },
		{ L"uarr",		(WCHAR)8593 },
		{ L"rarr",		(WCHAR)8594 },
		{ L"darr",		(WCHAR)8595 },
		{ L"harr",		(WCHAR)8596 },
		{ L"crarr",		(WCHAR)8629 },
		{ L"lArr",		(WCHAR)8656 },
		{ L"uArr",		(WCHAR)8657 },
		{ L"rArr",		(WCHAR)8658 },
		{ L"dArr",		(WCHAR)8659 },
		{ L"hArr",		(WCHAR)8660 },
		{ L"forall",	(WCHAR)8704 },
		{ L"part",		(WCHAR)8706 },
		{ L"exist",		(WCHAR)8707 },
		{ L"empty",		(WCHAR)8709 },
		{ L"nabla",		(WCHAR)8711 },
		{ L"isin",		(WCHAR)8712 },
		{ L"notin",		(WCHAR)8713 },
		{ L"ni",		(WCHAR)8715 },
		{ L"prod",		(WCHAR)8719 },
		{ L"sum",		(WCHAR)8721 },
		{ L"minus",		(WCHAR)8722 },
		{ L"lowast",	(WCHAR)8727 },
		{ L"radic",		(WCHAR)8730 },
		{ L"prop",		(WCHAR)8733 },
		{ L"infin",		(WCHAR)8734 },
		{ L"ang",		(WCHAR)8736 },
		{ L"and",		(WCHAR)8743 },
		{ L"or",		(WCHAR)8744 },
		{ L"cap",		(WCHAR)8745 },
		{ L"cup",		(WCHAR)8746 },
		{ L"int",		(WCHAR)8747 },
		{ L"there4",	(WCHAR)8756 },
		{ L"sim",		(WCHAR)8764 },
		{ L"cong",		(WCHAR)8773 },
		{ L"asymp",		(WCHAR)8776 },
		{ L"ne",		(WCHAR)8800 },
		{ L"equiv",		(WCHAR)8801 },
		{ L"le",		(WCHAR)8804 },
		{ L"ge",		(WCHAR)8805 },
		{ L"sub",		(WCHAR)8834 },
		{ L"sup",		(WCHAR)8835 },
		{ L"nsub",		(WCHAR)8836 },
		{ L"sube",		(WCHAR)8838 },
		{ L"supe",		(WCHAR)8839 },
		{ L"oplus",		(WCHAR)8853 },
		{ L"otimes",	(WCHAR)8855 },
		{ L"perp",		(WCHAR)8869 },
		{ L"sdot",		(WCHAR)8901 },
		{ L"lceil",		(WCHAR)8968 },
		{ L"rceil",		(WCHAR)8969 },
		{ L"lfloor",	(WCHAR)8970 },
		{ L"rfloor",	(WCHAR)8971 },
		{ L"lang",		(WCHAR)9001 },
		{ L"rang",		(WCHAR)9002 },
		{ L"loz",		(WCHAR)9674 },
		{ L"spades",	(WCHAR)9824 },
		{ L"clubs",		(WCHAR)9827 },
		{ L"hearts",	(WCHAR)9829 },
		{ L"diams",		(WCHAR)9830 }
	};

	const int entityCount = _countof(entities);
	g_CERs.rehash(entityCount);
	for (int i = 0; i < entityCount; ++i)
	{
		g_CERs.insert(std::pair<std::wstring, WCHAR>(entities[i].name, entities[i].ch));
	}

	// for DEBUG
	//std::map<std::wstring, WCHAR>::const_iterator iter = g_CERs.begin();
	//for ( ; iter != g_CERs.end(); ++iter)
	//{
	//	WCHAR buffer[64];
	//	wsprintf(buffer, L"%s - %c", (*iter).first.c_str(), (*iter).second);
	//	RmLog(LOG_DEBUG, buffer);
	//}
}

void SetupGlobalProxySetting()
{
	if (!g_ProxyCachePool)
	{
		WCHAR buffer[MAX_PATH] = {0};
		LPCWSTR file = RmGetSettingsFile();

		GetPrivateProfileString(L"WebParser.dll", L"ProxyServer", nullptr, buffer, MAX_PATH, file);
		g_ProxyCachePool = new ProxyCachePool(buffer);
	}
}

void ClearGlobalProxySetting()
{
	delete g_ProxyCachePool;
	g_ProxyCachePool = nullptr;
}

void SetupProxySetting(ProxySetting& setting, void* rm)
{
	if (g_ProxyCachePool)
	{
		setting.server = RmReadString(rm, L"ProxyServer", L"");
		setting.handle = g_ProxyCachePool->GetCache(setting.server);
	}
}

void ClearProxySetting(ProxySetting& setting)
{
	if (g_ProxyCachePool)
	{
		g_ProxyCachePool->RemoveCache(setting.server);
	}

	setting.handle = nullptr;
	setting.server.clear();
}

PLUGIN_EXPORT void Initialize(void** data, void* rm)
{
	MeasureData* measure = new MeasureData;
	*data = measure;
	g_Measures.push_back(measure);

	measure->skin = RmGetSkin(rm);
	measure->section = RmGetMeasureName(rm);

	if (g_InstanceCount == 0)
	{
		InitializeCriticalSection(&g_CriticalSection);
		FillCharacterEntityReferences();

		SetupGlobalProxySetting();
	}

	SetupProxySetting(measure->proxy, rm);  // No support for DynamicVariables

	++g_InstanceCount;
}

PLUGIN_EXPORT void Reload(void* data, void* rm, double* maxValue)
{
	MeasureData* measure = (MeasureData*)data;

	EnterCriticalSection(&g_CriticalSection);

	/* Read our own settings from the ini-file */

	std::wstring url = RmReadString(rm, L"Url", L"", FALSE);

	std::wstring::size_type start = 0;
	while ((start = url.find(L"[&", start)) != std::wstring::npos)
	{
		std::wstring::size_type si = start + 1;
		std::wstring::size_type end = url.find(L']', si);
		if (end == std::wstring::npos) break;

		std::wstring var = L"[";
		var += url.substr(si + 1, end - si);

		std::wstring result = RmReplaceVariables(rm, var.c_str());
		if (result != var)
		{
			url.replace(start, end - start + 1, result);
		}

		start = end;
	}

	measure->url = url;

	measure->regExp = RmReadString(rm, L"RegExp", L"");
	measure->finishAction = RmReadString(rm, L"FinishAction", L"", FALSE);
	measure->errorString = RmReadString(rm, L"ErrorString", L"");

	int index = RmReadInt(rm, L"StringIndex", 0);
	measure->stringIndex = index < 0 ? 0 : index;

	index = RmReadInt(rm, L"StringIndex2", 0);
	measure->stringIndex2 = index < 0 ? 0 : index;

	measure->decodeCharacterReference = RmReadInt(rm, L"DecodeCharacterReference", 0);
	measure->updateRate = RmReadInt(rm, L"UpdateRate", 600);
	measure->forceReload = 0!=RmReadInt(rm, L"ForceReload", 0);
	measure->codepage = RmReadInt(rm, L"CodePage", 0);

	measure->download = 0!=RmReadInt(rm, L"Download", 0);
	if (measure->download)
	{
		measure->downloadFolder = RmPathToAbsolute(rm, L"DownloadFile\\");
		measure->downloadFile = RmReadString(rm, L"DownloadFile", L"");
	}
	else
	{
		measure->downloadFile.clear();
	}

	measure->debug = RmReadInt(rm, L"Debug", 0);
	if (measure->debug == 2)
	{
		measure->debugFileLocation = RmReadPath(rm, L"Debug2File", L"WebParserDump.txt");
		RmLog(LOG_DEBUG, measure->debugFileLocation.c_str());
	}

	LeaveCriticalSection(&g_CriticalSection);
}

PLUGIN_EXPORT double Update(void* data)
{
	MeasureData* measure = (MeasureData*)data;
	double value = 0;

	if (measure->download && measure->regExp.empty() && measure->url.find(L'[') == std::wstring::npos)
	{
		// If RegExp is empty download the file that is pointed by the Url
		if (measure->dlThreadHandle == 0)
		{
			if (measure->updateCounter == 0)
			{
				// Launch a new thread to fetch the web data
				unsigned int id;
				HANDLE threadHandle = (HANDLE)_beginthreadex(nullptr, 0, NetworkDownloadThreadProc, measure, 0, &id);
				if (threadHandle)
				{
					measure->dlThreadHandle = threadHandle;
				}
				else  // error
				{
					std::wstring log = L"WebParser.dll: [";
					log += measure->section;
					log += L"] Failed to begin download thread";
					RmLog(LOG_ERROR, log.c_str());
				}
			}

			measure->updateCounter++;
			if (measure->updateCounter >= measure->updateRate)
			{
				measure->updateCounter = 0;
			}
		}

		// Else download the file pointed by the result string (this is done later)
	}
	else
	{
		// Make sure that the thread is not writing to the result at the same time
		EnterCriticalSection(&g_CriticalSection);

		if (!measure->resultString.empty())
		{
			value = wcstod(measure->resultString.c_str(), nullptr);
		}

		LeaveCriticalSection(&g_CriticalSection);

		if (measure->url.size() > 0 && measure->url.find(L'[') == std::wstring::npos)
		{
			// This is not a reference; need to update.
			if (measure->threadHandle == 0 && measure->dlThreadHandle == 0)
			{
				if (measure->updateCounter == 0)
				{
					// Launch a new thread to fetch the web data
					unsigned int id;
					HANDLE threadHandle = (HANDLE)_beginthreadex(nullptr, 0, NetworkThreadProc, measure, 0, &id);
					if (threadHandle)
					{
						measure->threadHandle = threadHandle;
					}
					else  // error
					{
						std::wstring log = L"WebParser.dll: [";
						log += measure->section;
						log += L"] Failed to begin thread";
						RmLog(LOG_ERROR, log.c_str());
					}
				}

				measure->updateCounter++;
				if (measure->updateCounter >= measure->updateRate)
				{
					measure->updateCounter = 0;
				}
			}
		}
	}

	return value;
}

// Fetches the data from the net and parses the page
unsigned __stdcall NetworkThreadProc(void* pParam)
{
	MeasureData* measure = (MeasureData*)pParam;
	DWORD dwSize = 0;

	BYTE* data = DownloadUrl(measure->proxy.handle, measure->url, &dwSize, measure->forceReload);

	if (data)
	{
		if (measure->debug == 2)
		{
			// Dump to a file

			FILE* file = _wfopen(measure->debugFileLocation.c_str(), L"wb");
			if (file)
			{
				fwrite(data, sizeof(BYTE), dwSize, file);
				fclose(file);
			}
			else
			{
				std::wstring log = L"WebParser.dll: [";
				log += measure->section;
				log += L"] Failed to dump debug data: ";
				log += measure->debugFileLocation;
				RmLog(LOG_ERROR, log.c_str());
			}
		}

		ParseData(measure, (LPCSTR)data, dwSize);

		free(data);
	}

	EnterCriticalSection(&g_CriticalSection);
	CloseHandle(measure->threadHandle);
	measure->threadHandle = 0;
	LeaveCriticalSection(&g_CriticalSection);

	return 0;   // thread completed successfully
}

void ParseData(MeasureData* measure, LPCSTR parseData, DWORD dwSize)
{
	// Parse the value from the data
	pcre* re;
	const char* error;
	int erroffset;
	int ovector[OVECCOUNT];
	int rc;
	int flags = PCRE_UTF8;

	if (measure->codepage == 0)
	{
		flags = 0;
	}

	// Compile the regular expression in the first argument
	re = pcre_compile(
		StringUtil::NarrowUTF8(measure->regExp).c_str(),	// the pattern
		flags,												// default options
		&error,												// for error message
		&erroffset,											// for error offset
		nullptr);											// use default character tables

	if (re != nullptr)
	{
		// Compilation succeeded: match the subject in the second argument
		std::string utf8Data;

		if (measure->codepage == 1200)		// 1200 = UTF-16LE
		{
			// Must convert the data to utf8
			utf8Data = StringUtil::NarrowUTF8((LPCWSTR)parseData, dwSize / 2);
			parseData = utf8Data.c_str();
			dwSize = utf8Data.length();
		}
		else if (measure->codepage != CP_UTF8 && measure->codepage != 0)		// 0 = CP_ACP
		{
			// Must convert the data to utf8
			utf8Data = ConvertAsciiToUTF8(parseData, dwSize, measure->codepage);
			parseData = utf8Data.c_str();
			dwSize = utf8Data.length();
		}

		rc = pcre_exec(
			re,						// the compiled pattern
			nullptr,				// no extra data - we didn't study the pattern
			parseData,				// the subject string
			dwSize,					// the length of the subject
			0,						// start at offset 0 in the subject
			0,						// default options
			ovector,				// output vector for substring information
			OVECCOUNT);				// number of elements in the output vector

		if (rc >= 0)
		{
			if (rc == 0)
			{
				// The output vector wasn't big enough
				std::wstring log = L"WebParser.dll: [";
				log += measure->section;
				log += L"] Too many substrings!";
				RmLog(LOG_ERROR, log.c_str());
			}
			else
			{
				if (measure->stringIndex < rc)
				{
					if (measure->debug != 0)
					{
						for (int i = 0; i < rc; ++i)
						{
							const char* substring_start = parseData + ovector[2 * i];
							int substring_length = ovector[2 * i + 1] - ovector[2 * i];
							substring_length = min(substring_length, 256);

							WCHAR buffer[32];
							wsprintf(buffer, L"%2d", i);

							std::wstring log = L"WebParser.dll: [";
							log += measure->section;
							log += L"] (Index ";
							log += buffer;
							log += L") ";
							log += StringUtil::WidenUTF8(substring_start, substring_length);
							RmLog(LOG_DEBUG, log.c_str());
						}
					}

					const char* substring_start = parseData + ovector[2 * measure->stringIndex];
					int substring_length = ovector[2 * measure->stringIndex + 1] - ovector[2 * measure->stringIndex];

					EnterCriticalSection(&g_CriticalSection);
					measure->resultString = StringUtil::WidenUTF8(substring_start, substring_length);
					DecodeReferences(measure->resultString, measure->decodeCharacterReference);
					LeaveCriticalSection(&g_CriticalSection);
				}
				else
				{
					std::wstring log = L"WebParser.dll: [";
					log += measure->section;
					log += L"] Not enough substrings!";
					RmLog(LOG_WARNING, log.c_str());

					// Clear the old result
					EnterCriticalSection(&g_CriticalSection);
					measure->resultString.clear();
					if (measure->download)
					{
						if (measure->downloadFile.empty())  // cache mode
						{
							if (!measure->downloadedFile.empty())
							{
								// Delete old downloaded file
								DeleteFile(measure->downloadedFile.c_str());
							}
						}
						measure->downloadedFile.clear();
					}
					LeaveCriticalSection(&g_CriticalSection);
				}

				// Update the references
				std::vector<MeasureData*>::iterator i = g_Measures.begin();
				std::wstring compareStr = L"[";
				compareStr += measure->section;
				compareStr += L"]";
				for ( ; i != g_Measures.end(); ++i)
				{
					if (measure->skin == (*i)->skin &&
						(*i)->url.find(compareStr) != std::wstring::npos)
					{
						if ((*i)->stringIndex < rc)
						{
							const char* substring_start = parseData + ovector[2 * (*i)->stringIndex];
							int substring_length = ovector[2 * (*i)->stringIndex + 1] - ovector[2 * (*i)->stringIndex];

							if (!(*i)->regExp.empty())
							{
								// Change the index and parse the substring
								int index = (*i)->stringIndex;
								(*i)->stringIndex = (*i)->stringIndex2;
								ParseData((*i), substring_start, substring_length);
								(*i)->stringIndex = index;
							}
							else
							{
								// Set the result
								EnterCriticalSection(&g_CriticalSection);

								// Substitude the [measure] with result
								std::wstring result = StringUtil::WidenUTF8(substring_start, substring_length);
								(*i)->resultString = (*i)->url;
								(*i)->resultString.replace((*i)->resultString.find(compareStr), compareStr.size(), result);
								DecodeReferences((*i)->resultString, (*i)->decodeCharacterReference);

								// Start download threads for the references
								if ((*i)->download)
								{
									// Start the download thread
									unsigned int id;
									HANDLE threadHandle = (HANDLE)_beginthreadex(nullptr, 0, NetworkDownloadThreadProc, (*i), 0, &id);
									if (threadHandle)
									{
										(*i)->dlThreadHandle = threadHandle;
									}
									else  // error
									{
										std::wstring log = L"WebParser.dll: [";
										log += (*i)->section;
										log += L"] Failed to begin download thread";
										RmLog(LOG_ERROR, log.c_str());
									}
								}

								LeaveCriticalSection(&g_CriticalSection);
							}
						}
						else
						{
							std::wstring log = L"WebParser.dll: [";
							log += (*i)->section;
							log += L"] Not enough substrings!";
							RmLog(LOG_WARNING, log.c_str());

							// Clear the old result
							EnterCriticalSection(&g_CriticalSection);
							(*i)->resultString.clear();
							if ((*i)->download)
							{
								if ((*i)->downloadFile.empty())  // cache mode
								{
									if (!(*i)->downloadedFile.empty())
									{
										// Delete old downloaded file
										DeleteFile((*i)->downloadedFile.c_str());
									}
								}
								(*i)->downloadedFile.clear();
							}
							LeaveCriticalSection(&g_CriticalSection);
						}
					}
				}
			}
		}
		else
		{
			// Matching failed: handle error cases
			WCHAR buffer[32];
			wsprintf(buffer, L"%d", rc);

			std::wstring log = L"WebParser.dll: [";
			log += measure->section;
			log += L"] Matching error! (";
			log += buffer;
			log += L')';
			RmLog(LOG_ERROR, log.c_str());

			EnterCriticalSection(&g_CriticalSection);
			measure->resultString = measure->errorString;

			// Update the references
			std::vector<MeasureData*>::iterator i = g_Measures.begin();
			std::wstring compareStr = L"[";
			compareStr += measure->section;
			compareStr += L"]";
			for ( ; i != g_Measures.end(); ++i)
			{
				if (((*i)->url.find(compareStr) != std::wstring::npos) && (measure->skin == (*i)->skin))
				{
					(*i)->resultString = (*i)->errorString;
				}
			}
			LeaveCriticalSection(&g_CriticalSection);
		}

		// Release memory used for the compiled pattern
		pcre_free(re);
	}
	else
	{
		// Compilation failed: print the error message and exit
		WCHAR buffer[32];
		wsprintf(buffer, L"%d", erroffset);

		std::wstring log = L"WebParser.dll: [";
		log += measure->section;
		log += L"] PCRE compilation failed at offset ";
		log += buffer;
		log += L": ";
		log += StringUtil::Widen(error);
		RmLog(LOG_ERROR, log.c_str());
	}

	if (measure->download)
	{
		// Start the download thread
		unsigned int id;
		HANDLE threadHandle = (HANDLE)_beginthreadex(nullptr, 0, NetworkDownloadThreadProc, measure, 0, &id);
		if (threadHandle)
		{
			measure->dlThreadHandle = threadHandle;
		}
		else  // error
		{
			std::wstring log = L"WebParser.dll: [";
			log += measure->section;
			log += L"] Failed to begin download thread";
			RmLog(LOG_ERROR, log.c_str());
		}
	}
	else
	{
		if (!measure->finishAction.empty())
		{
			RmExecute(measure->skin, measure->finishAction.c_str());
		}
	}
}

// Downloads file from the net
unsigned __stdcall NetworkDownloadThreadProc(void* pParam)
{
	MeasureData* measure = (MeasureData*)pParam;
	const bool download = !measure->downloadFile.empty();
	bool ready = false;

	std::wstring url;

	if (measure->regExp.empty() && measure->resultString.empty())
	{
		if (!measure->url.empty() && measure->url[0] != L'[')
		{
			url = measure->url;
		}
	}
	else
	{
		EnterCriticalSection(&g_CriticalSection);
		url = measure->resultString;
		LeaveCriticalSection(&g_CriticalSection);

		std::wstring::size_type pos = url.find(L':');
		if (pos == std::wstring::npos && !url.empty())	// No protocol
		{
			// Add the base url to the string
			if (url[0] == L'/')
			{
				// Absolute path
				pos = measure->url.find(L'/', 7);	// Assume "http://" (=7)
				if (pos != std::wstring::npos)
				{
					std::wstring path(measure->url.substr(0, pos));
					url = path + url;
				}
			}
			else
			{
				// Relative path

				pos = measure->url.rfind(L'/');
				if (pos != std::wstring::npos)
				{
					std::wstring path(measure->url.substr(0, pos + 1));
					url = path + url;
				}
			}
		}
	}

	if (!url.empty())
	{
		// Create the filename
		WCHAR buffer[MAX_PATH] = {0};
		std::wstring fullpath, directory;

		if (download)  // download mode
		{
			PathCanonicalize(buffer, measure->downloadFile.c_str());

			std::wstring path = buffer;
			std::wstring::size_type pos = path.find_first_not_of(L'\\');
			if (pos != std::wstring::npos)
			{
				path.erase(0, pos);
			}

			PathCanonicalize(buffer, measure->downloadFolder.c_str());
			CreateDirectory(buffer, nullptr);	// Make sure that the folder exists

			wcscat(buffer, path.c_str());

			if (buffer[wcslen(buffer)-1] != L'\\')  // path is a file
			{
				fullpath = buffer;
				PathRemoveFileSpec(buffer);
			}
			PathAddBackslash(buffer);
		}
		else  // cache mode
		{
			GetTempPath(MAX_PATH, buffer);
			wcscat(buffer, L"Rainmeter-Cache\\");  // "%TEMP%\Rainmeter-Cache\"
		}
		CreateDirectory(buffer, nullptr);	// Make sure that the folder exists
		directory = buffer;

		if (fullpath.empty())
		{
			fullpath = directory;

			std::wstring::size_type pos2 = url.find_first_of(L"?#");
			std::wstring::size_type pos1 = url.find_last_of(L'/', pos2);
			pos1 = (pos1 != std::wstring::npos) ? pos1 + 1 : 0;

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
				pos1 = 0;
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
			std::wstring log;

			if (!PathFileExists(directory.c_str()) || !PathIsDirectory(directory.c_str()))
			{
				ready = false;

				log = L"WebParser.dll: [";
				log += measure->section;
				log += L"] Directory does not exist: ";
				log += directory;
				RmLog(LOG_ERROR, log.c_str());
			}
			else if (PathIsDirectory(fullpath.c_str()))
			{
				ready = false;

				log = L"WebParser.dll: [";
				log += measure->section;
				log += L"] Path is a directory, not a file: ";
				log += fullpath;
				RmLog(LOG_ERROR, log.c_str());
			}
			else if (PathFileExists(fullpath.c_str()))
			{
				DWORD attr = GetFileAttributes(fullpath.c_str());
				if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_READONLY))
				{
					ready = false;

					log = L"WebParser.dll: [";
					log += measure->section;
					log += L"] File is READ-ONLY: ";
					log += fullpath;
					RmLog(LOG_ERROR, log.c_str());
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
					wsprintf(buffer, L"_%i", i++);

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

				HKEY hKey;
				LONG ret;
				DWORD mode;

				ret = RegOpenKeyEx(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings", 0, KEY_QUERY_VALUE, &hKey);
				if (ret == ERROR_SUCCESS)
				{
					DWORD size = sizeof(mode);
					ret = RegQueryValueEx(hKey, L"SyncMode5", nullptr, nullptr, (LPBYTE)&mode, &size);
					RegCloseKey(hKey);
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

			// Write some log info
			std::wstring log = L"WebParser.dll: [";
			log += measure->section;
			log += L"] Downloading url ";
			log += url;
			log += L" to ";
			log += fullpath;
			RmLog(LOG_DEBUG, log.c_str());

			HRESULT resultCoInitialize = CoInitialize(nullptr);  // requires before calling URLDownloadToFile function

			// Download the file
			HRESULT result = URLDownloadToFile(nullptr, url.c_str(), fullpath.c_str(), 0, nullptr);
			if (result == S_OK)
			{
				EnterCriticalSection(&g_CriticalSection);

				if (!download)  // cache mode
				{
					if (!measure->downloadedFile.empty())
					{
						// Delete old downloaded file
						DeleteFile(measure->downloadedFile.c_str());
					}
				}

				// Convert LFN to 8.3 filename if the path contains blank character
				if (fullpath.find_first_of(L' ') != std::wstring::npos)
				{
					DWORD size = GetShortPathName(fullpath.c_str(), buffer, MAX_PATH);
					if (size > 0 && size <= MAX_PATH)
					{
						fullpath = buffer;
					}
				}
				measure->downloadedFile = fullpath;

				LeaveCriticalSection(&g_CriticalSection);

				if (!measure->finishAction.empty())
				{
					RmExecute(measure->skin, measure->finishAction.c_str());
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

				wsprintf(buffer, L"result=0x%08X, COM=0x%08X", result, resultCoInitialize);

				std::wstring log = L"WebParser.dll: [";
				log += measure->section;
				log += L"] Download failed (";
				log += buffer;
				log += L"): ";
				log += url;
				RmLog(LOG_ERROR, log.c_str());
			}

			if (SUCCEEDED(resultCoInitialize))
			{
				CoUninitialize();
			}
		}
		else
		{
			std::wstring log = L"WebParser.dll: [";
			log += measure->section;
			log += L"] Download failed: ";
			log += url;
			RmLog(LOG_ERROR, log.c_str());
		}
	}
	else
	{
		std::wstring log = L"WebParser.dll: [";
		log += measure->section;
		log += L"] Url is empty";
		RmLog(LOG_ERROR, log.c_str());
	}

	if (!ready) // download failed
	{
		EnterCriticalSection(&g_CriticalSection);

		if (!download) // cache mode
		{
			if (!measure->downloadedFile.empty())
			{
				// Delete old downloaded file
				DeleteFile(measure->downloadedFile.c_str());
			}
		}

		// Clear old downloaded filename
		measure->downloadedFile.clear();

		LeaveCriticalSection(&g_CriticalSection);
	}

	EnterCriticalSection(&g_CriticalSection);
	CloseHandle(measure->dlThreadHandle);
	measure->dlThreadHandle = 0;
	LeaveCriticalSection(&g_CriticalSection);

	return 0;   // thread completed successfully
}

PLUGIN_EXPORT LPCWSTR GetString(void* data)
{
	MeasureData* measure = (MeasureData*)data;
	static std::wstring resultString;

	EnterCriticalSection(&g_CriticalSection);
	if (measure->download)
	{
		resultString = measure->downloadedFile;
	}
	else
	{
		resultString = measure->resultString;
	}
	LeaveCriticalSection(&g_CriticalSection);

	return resultString.c_str();
}

PLUGIN_EXPORT void Finalize(void* data)
{
	MeasureData* measure = (MeasureData*)data;

	if (measure->threadHandle)
	{
		// Thread is killed inside critical section so that itself is not inside one when it is terminated
		EnterCriticalSection(&g_CriticalSection);

		TerminateThread(measure->threadHandle, 0);
		measure->threadHandle = nullptr;

		LeaveCriticalSection(&g_CriticalSection);
	}

	if (measure->dlThreadHandle)
	{
		// Thread is killed inside critical section so that itself is not inside one when it is terminated
		EnterCriticalSection(&g_CriticalSection);

		TerminateThread(measure->dlThreadHandle, 0);
		measure->dlThreadHandle = nullptr;

		LeaveCriticalSection(&g_CriticalSection);
	}

	if (measure->downloadFile.empty())  // cache mode
	{
		if (!measure->downloadedFile.empty())
		{
			// Delete the file
			DeleteFile(measure->downloadedFile.c_str());
		}
	}

	ClearProxySetting(measure->proxy);

	delete measure;
	std::vector<MeasureData*>::iterator iter = std::find(g_Measures.begin(), g_Measures.end(), measure);
	g_Measures.erase(iter);

	--g_InstanceCount;
	if (g_InstanceCount == 0)
	{
		// Last one, close all handles
		ClearGlobalProxySetting();

		g_CERs.clear();

		// Last instance deletes the critical section
		DeleteCriticalSection(&g_CriticalSection);
	}
}

/*
	Downloads the given url and returns the webpage as dynamically allocated string.
	You need to free the returned string after use!
*/
BYTE* DownloadUrl(HINTERNET handle, std::wstring& url, DWORD* dataSize, bool forceReload)
{
	std::wstring err = L"WebParser.dll: Fetching: " + url;
	RmLog(LOG_DEBUG, err.c_str());

	DWORD flags = INTERNET_FLAG_RESYNCHRONIZE;
	if (forceReload)
	{
		flags = INTERNET_FLAG_RELOAD;
	}

	HINTERNET hUrlDump = InternetOpenUrl(handle, url.c_str(), nullptr, 0, flags, 0);
	if (!hUrlDump)
	{
		if (_wcsnicmp(url.c_str(), L"file://", 7) == 0)  // file scheme
		{
			const std::string urlACP = StringUtil::Narrow(url);
			hUrlDump = InternetOpenUrlA(handle, urlACP.c_str(), nullptr, 0, flags, 0);
		}

		if (!hUrlDump)
		{
			ShowError(__LINE__);
			return nullptr;
		}
	}

	// Allocate buffer with 3 extra bytes for triple null termination in case the string is
	// invalid (e.g. when incorrectly using the UTF-16LE codepage for the data).
	const int CHUNK_SIZE = 8192;
	DWORD bufferSize = CHUNK_SIZE;
	BYTE* buffer = (BYTE*)malloc(bufferSize + 3);
	*dataSize = 0;

	// Read the data.
	do
	{
		DWORD readSize;
		if (!InternetReadFile(hUrlDump, buffer + *dataSize, bufferSize - *dataSize, &readSize))
		{
			ShowError(__LINE__);
			break;
		}
		else if (readSize == 0)
		{
			// All data read.
			break;
		}

		*dataSize += readSize;

		bufferSize += CHUNK_SIZE;
		buffer = (BYTE*)realloc(buffer, bufferSize + 3);
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
void ShowError(int lineNumber, WCHAR* errorMsg)
{
	DWORD dwErr = GetLastError();

	WCHAR buffer[16];
	wsprintf(buffer, L"%i", lineNumber);

	std::wstring err = L"WebParser.dll: (";
	err += buffer;
	err += L") ";

	if (errorMsg == nullptr)
	{
		if (dwErr == ERROR_INTERNET_EXTENDED_ERROR)
		{
			WCHAR szBuffer[1024];
			DWORD dwError, dwLen = 1024;
			if (InternetGetLastResponseInfo(&dwError, szBuffer, &dwLen))
			{
				err += szBuffer;
				wsprintf(buffer, L"%i", dwError);
			}
			else
			{
				err += L"Unknown error";
				wsprintf(buffer, L"%i", dwErr);
			}

			err += L" (ErrorCode=";
			err += buffer;
			err += L')';
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

			if (lpMsgBuf == nullptr)
			{
				err += L"Unknown error";
			}
			else
			{
				err += (LPTSTR)lpMsgBuf;
				LocalFree(lpMsgBuf);
			}

			wsprintf(buffer, L"%i", dwErr);
			err += L" (ErrorCode=";
			err += buffer;
			err += L')';
		}
	}
	else
	{
		err += errorMsg;
	}

	RmLog(LOG_ERROR, err.c_str());
}

PLUGIN_EXPORT void ExecuteBang(void* data, LPCWSTR args)
{
	MeasureData* measure = (MeasureData*)data;

	// Kill the threads (if any) and reset the update counter
	if (_wcsicmp(args, L"UPDATE") == 0)
	{
		if (measure->threadHandle)
		{
			// Thread is killed inside critical section so that itself is not inside one when it is terminated
			EnterCriticalSection(&g_CriticalSection);

			TerminateThread(measure->threadHandle, 0);
			measure->threadHandle = nullptr;

			LeaveCriticalSection(&g_CriticalSection);
		}

		if (measure->dlThreadHandle)
		{
			// Thread is killed inside critical section so that itself is not inside one when it is terminated
			EnterCriticalSection(&g_CriticalSection);

			TerminateThread(measure->dlThreadHandle, 0);
			measure->dlThreadHandle = nullptr;

			LeaveCriticalSection(&g_CriticalSection);
		}

		measure->updateCounter = 0;
	}
}
