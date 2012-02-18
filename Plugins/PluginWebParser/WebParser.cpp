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

// Note: To compile this you need the PCRE library (http://www.pcre.org/).
// See: http://www.perldoc.com/perl5.8.0/pod/perlre.html

#include <windows.h>
#include <math.h>
#include <string>
#include <map>
#include <vector>
#include <Wininet.h>
#include <shlwapi.h>
#include <process.h>
#include "../../Library/pcre-8.10/config.h"
#include "../../Library/pcre-8.10/pcre.h"
#include "../../Library/Export.h"	// Rainmeter's exported functions

#include "../../Library/DisableThreadLibraryCalls.h"	// contains DllMain entry point

/* The exported functions */
extern "C"
{
__declspec( dllexport ) UINT Initialize(HMODULE instance, LPCTSTR iniFile, LPCTSTR section, UINT id);
__declspec( dllexport ) double Update2(UINT id);
__declspec( dllexport ) LPCTSTR GetString(UINT id, UINT flags);
__declspec( dllexport ) void Finalize(HMODULE instance, UINT id);
__declspec( dllexport ) UINT GetPluginVersion();
__declspec( dllexport ) LPCTSTR GetPluginAuthor();
}

struct UrlData
{
	std::wstring url;
	std::wstring regExp;
	std::wstring resultString;
	std::wstring errorString;
	std::wstring proxy;
	int codepage;
	int stringIndex;
	int stringIndex2;
	int decodeCharacterReference;
	UINT updateRate;
	UINT updateCounter;
	std::wstring section;
	std::wstring finishAction;
	bool download;
	bool forceReload;
	std::wstring downloadFile;
	std::wstring downloadedFile;
	std::wstring iniFile;
	int debug;
	std::wstring debugFileLocation;
	HANDLE threadHandle;
	HANDLE dlThreadHandle;
};

BYTE* DownloadUrl(std::wstring& url, DWORD* dwSize, bool forceReload);
void ShowError(int lineNumber, WCHAR* errorMsg = NULL);
unsigned __stdcall NetworkThreadProc(void* pParam);
unsigned __stdcall NetworkDownloadThreadProc(void* pParam);
void Log(int level, const WCHAR* string);
void ParseData(UrlData* urlData, LPCSTR parseData, DWORD dwSize);

CRITICAL_SECTION g_CriticalSection;
bool g_Initialized = false;

static std::map<UINT, UrlData*> g_UrlData;
static bool g_Debug = false;
static HINTERNET hRootHandle = NULL;

static std::map<std::wstring, WCHAR> g_CERs;

#define OVECCOUNT 300    // should be a multiple of 3

std::wstring ConvertToWide(LPCSTR str, int codepage)
{
	std::wstring szWide;

	if (str && *str)
	{
		int strLen = (int)strlen(str);
		int bufLen = MultiByteToWideChar(codepage, 0, str, strLen, NULL, 0);
		if (bufLen > 0)
		{
			szWide.resize(bufLen);
			MultiByteToWideChar(codepage, 0, str, strLen, &szWide[0], bufLen);
		}
	}
	return szWide;
}

std::string ConvertToAscii(LPCWSTR str, int codepage)
{
	std::string szAscii;

	if (str && *str)
	{
		int strLen = (int)wcslen(str);
		int bufLen = WideCharToMultiByte(codepage, 0, str, strLen, NULL, 0, NULL, NULL);
		if (bufLen > 0)
		{
			szAscii.resize(bufLen);
			WideCharToMultiByte(codepage, 0, str, strLen, &szAscii[0], bufLen, NULL, NULL);
		}
	}
	return szAscii;
}

std::string ConvertWideToAscii(LPCWSTR str)
{
	return ConvertToAscii(str, CP_ACP);
}

std::wstring ConvertAsciiToWide(LPCSTR str)
{
	return ConvertToWide(str, CP_ACP);
}

std::wstring ConvertUTF8ToWide(LPCSTR str)
{
	return ConvertToWide(str, CP_UTF8);
}

std::string ConvertWideToUTF8(LPCWSTR str)
{
	return ConvertToAscii(str, CP_UTF8);
}

std::string ConvertAsciiToUTF8(LPCSTR str, int codepage)
{
	std::string szUTF8;

	if (str && *str)
	{
		std::wstring wide = ConvertToWide(str, codepage);
		if (!wide.empty())
		{
			szUTF8.swap(ConvertWideToUTF8(wide.c_str()));
		}
	}
	return szUTF8;
}

std::wstring& DecodeReferences(std::wstring& str, int opt)
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
				WCHAR* pch = NULL;
				errno = 0;
				long ch = wcstol(num.c_str(), &pch, base);
				if (pch == NULL || *pch != L'\0' || errno == ERANGE || ch <= 0 || ch >= 0xFFFE)  // invalid character
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

				std::map<std::wstring, WCHAR>::const_iterator iter = g_CERs.find(name);
				if (iter != g_CERs.end())
				{
					str.replace(start, end - start + 1, 1, (*iter).second);
				}
				++start;
			}
		}
	}

	return str;
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
	//	Log(buffer);
	//}
}

bool BelongToSameProcess(HWND wnd)
{
	DWORD procId = 0;
	GetWindowThreadProcessId(wnd, &procId);

	return (procId == GetCurrentProcessId());
}

HWND FindMeterWindow(HWND parent)
{
	HWND wnd = NULL;

	while (wnd = FindWindowEx(parent, wnd, L"RainmeterMeterWindow", NULL))
	{
		if (BelongToSameProcess(wnd))
		{
			return wnd;
		}
	}

	return NULL;
}

HWND FindMeterWindow(const std::wstring& iniFile)
{
	std::wstring str = PluginBridge(L"getconfig", iniFile.c_str());
	if (!str.empty())
	{
		str = PluginBridge(L"getwindow", str.c_str());
		if (str != L"error")
		{
			return (HWND)UlongToPtr(wcstoul(str.c_str(), NULL, 10));
		}
	}

	return FindMeterWindow(NULL);  // Use old way to find
}

/*
  This function is called when the measure is initialized.
  The function must return the maximum value that can be measured.
  The return value can also be 0, which means that Rainmeter will
  track the maximum value automatically. The parameters for this
  function are:

  instance  The instance of this DLL
  iniFile   The name of the ini-file (usually Rainmeter.ini)
  section   The name of the section in the ini-file for this measure
  id        The identifier for the measure. This is used to identify the measures that use the same plugin.
*/
UINT Initialize(HMODULE instance, LPCTSTR iniFile, LPCTSTR section, UINT id)
{
	LPCTSTR tmpSz;

	if (!g_Initialized)
	{
		InitializeCriticalSection(&g_CriticalSection);
		FillCharacterEntityReferences();
		g_Initialized = true;
	}

	UrlData* data = new UrlData;
	data->section = section;
	data->decodeCharacterReference = 0;
	data->updateRate = 600;
	data->updateCounter = 0;
	data->iniFile = iniFile;

	/* Read our own settings from the ini-file */

	data->url = ReadConfigString(section, L"Url", L"");
	data->regExp = ReadConfigString(section, L"RegExp", L"");
	data->finishAction = ReadConfigString(section, L"FinishAction", L"");
	data->errorString = ReadConfigString(section, L"ErrorString", L"");
	data->proxy = ReadConfigString(section, L"ProxyServer", L"");
	data->downloadFile = ReadConfigString(section, L"DownloadFile", L"");
	data->debugFileLocation = ReadConfigString(section, L"Debug2File", L"c:\\WebParserDump.txt");

	if (data->debugFileLocation.find(L"\\") == std::wstring::npos)
	{
		std::wstring str = data->iniFile.substr(0,data->iniFile.find_last_of(L"\\")+1);
		str += data->debugFileLocation;
		Log(LOG_DEBUG, str.c_str());
		data->debugFileLocation = str;
	}

	tmpSz = ReadConfigString(section, L"StringIndex", L"0");
	if (tmpSz)
	{
		data->stringIndex = _wtoi(tmpSz);
	}

	tmpSz = ReadConfigString(section, L"StringIndex2", L"0");
	if (tmpSz)
	{
		data->stringIndex2 = _wtoi(tmpSz);
	}

	tmpSz = ReadConfigString(section, L"DecodeCharacterReference", L"0");
	if (tmpSz)
	{
		data->decodeCharacterReference = _wtoi(tmpSz);
	}

	tmpSz = ReadConfigString(section, L"UpdateRate", L"600");
	if (tmpSz && *tmpSz)
	{
		data->updateRate = _wtoi(tmpSz);
	}

	tmpSz = ReadConfigString(section, L"Download", L"0");
	if (tmpSz)
	{
		data->download = 1 == _wtoi(tmpSz);
	}

	tmpSz = ReadConfigString(section, L"ForceReload", L"0");
	if (tmpSz)
	{
		data->forceReload = 1 == _wtoi(tmpSz);
	}

	tmpSz = ReadConfigString(section, L"Debug", L"0");
	if (tmpSz)
	{
		data->debug = _wtoi(tmpSz);
	}

	tmpSz = ReadConfigString(section, L"CodePage", L"0");
	if (tmpSz)
	{
		data->codepage = _wtoi(tmpSz);
	}

	if (hRootHandle == NULL)
	{
		if (data->proxy.empty())
		{
			hRootHandle = InternetOpen(L"Rainmeter WebParser plugin",
										INTERNET_OPEN_TYPE_PRECONFIG,
										NULL,
										NULL,
										0);
		}
		else
		{
			hRootHandle = InternetOpen(L"Rainmeter WebParser plugin",
										INTERNET_OPEN_TYPE_PROXY,
										data->proxy.c_str(),
										NULL,
										0);
		}

		if (hRootHandle == NULL)
		{
			ShowError(__LINE__);
			delete data;
			return 0;
		}
	}

	data->threadHandle = 0;
	data->dlThreadHandle = 0;

	// During initialization there is no threads yet so no need to do any locking
	g_UrlData[id] = data;

	return 0;
}

/*
  This function is called when new value should be measured.
  The function returns the new value.
*/
double Update2(UINT id)
{
	double value = 0;
	UrlData* urlData = NULL;

	// Find the data for this instance (the data structure is not changed by anyone so this should be safe)
	std::map<UINT, UrlData*>::iterator urlIter = g_UrlData.find(id);
	if (urlIter != g_UrlData.end())
	{
		urlData = (*urlIter).second;
	}

	if (urlData)
	{
		if (urlData->download && urlData->regExp.empty() && urlData->url.find(L'[') == std::wstring::npos)
		{
			// If RegExp is empty download the file that is pointed by the Url
			if (urlData->dlThreadHandle == 0)
			{
				if (urlData->updateCounter == 0)
				{
					// Launch a new thread to fetch the web data
					unsigned int id;
					HANDLE threadHandle = (HANDLE)_beginthreadex(NULL, 0, NetworkDownloadThreadProc, urlData, 0, &id);
					if (threadHandle)
					{
						urlData->dlThreadHandle = threadHandle;
					}
					else  // error
					{
						std::wstring log = L"WebParser.dll: [";
						log += urlData->section;
						log += L"] Failed to begin download thread.";
						Log(LOG_ERROR, log.c_str());
					}
				}

				urlData->updateCounter++;
				if (urlData->updateCounter >= urlData->updateRate)
				{
					urlData->updateCounter = 0;
				}
			}

			// Else download the file pointed by the result string (this is done later)
		}
		else
		{
			// Make sure that the thread is not writing to the result at the same time
			EnterCriticalSection(&g_CriticalSection);

			if (!urlData->resultString.empty())
			{
				value = wcstod(urlData->resultString.c_str(), NULL);
			}

			LeaveCriticalSection(&g_CriticalSection);

			if (urlData->url.size() > 0 && urlData->url.find(L'[') == std::wstring::npos)
			{
				// This is not a reference; need to update.
				if (urlData->threadHandle == 0 && urlData->dlThreadHandle == 0)
				{
					if (urlData->updateCounter == 0)
					{
						// Launch a new thread to fetch the web data
						unsigned int id;
						HANDLE threadHandle = (HANDLE)_beginthreadex(NULL, 0, NetworkThreadProc, urlData, 0, &id);
						if (threadHandle)
						{
							urlData->threadHandle = threadHandle;
						}
						else  // error
						{
							std::wstring log = L"WebParser.dll: [";
							log += urlData->section;
							log += L"] Failed to begin thread.";
							Log(LOG_ERROR, log.c_str());
						}
					}

					urlData->updateCounter++;
					if (urlData->updateCounter >= urlData->updateRate)
					{
						urlData->updateCounter = 0;
					}
				}
			}
		}
	}

	return value;
}



/*
  Thread that fetches the data from the net and parses the page.
*/
unsigned __stdcall NetworkThreadProc(void* pParam)
{
	UrlData* urlData = (UrlData*)pParam;
	DWORD dwSize = 0;

	BYTE* data = DownloadUrl(urlData->url, &dwSize, urlData->forceReload);

	if (data)
	{
		if (urlData->debug == 2)
		{
			// Dump to a file

			FILE* file = _wfopen(urlData->debugFileLocation.c_str(), L"wb");
			if (file)
			{
				fwrite(data, sizeof(BYTE), dwSize, file);
				fclose(file);
			}
			else
			{
				std::wstring log = L"WebParser.dll: [";
				log += urlData->section;
				log += L"] Failed to dump debug data: ";
				log += urlData->debugFileLocation;
				Log(LOG_ERROR, log.c_str());
			}
		}

		ParseData(urlData, (LPCSTR)data, dwSize);

		delete [] data;
	}

	EnterCriticalSection(&g_CriticalSection);
	CloseHandle(urlData->threadHandle);
	urlData->threadHandle = 0;
	LeaveCriticalSection(&g_CriticalSection);

	return 0;   // thread completed successfully
}

void ParseData(UrlData* urlData, LPCSTR parseData, DWORD dwSize)
{
	// Parse the value from the data
	pcre* re;
	const char* error;
	int erroffset;
	int ovector[OVECCOUNT];
	int rc;
	int flags = PCRE_UTF8;

	if (urlData->codepage == 0)
	{
		flags = 0;
	}

	// Compile the regular expression in the first argument
	re = pcre_compile(
		ConvertWideToUTF8(urlData->regExp.c_str()).c_str(),   // the pattern
		flags,					  // default options
		&error,               // for error message
		&erroffset,           // for error offset
		NULL);                // use default character tables

	if (re != NULL)
	{
		// Compilation succeeded: match the subject in the second argument
		std::string utf8Data;

		if (urlData->codepage == 1200)		// 1200 = UTF-16LE
		{
			// Must convert the data to utf8
			utf8Data = ConvertWideToUTF8((LPCWSTR)parseData);
			parseData = utf8Data.c_str();
			dwSize = utf8Data.length();
		}
		else if (urlData->codepage != CP_UTF8 && urlData->codepage != 0)		// 0 = CP_ACP
		{
			// Must convert the data to utf8
			utf8Data = ConvertAsciiToUTF8(parseData, urlData->codepage);
			parseData = utf8Data.c_str();
			dwSize = utf8Data.length();
		}

		rc = pcre_exec(
			re,                   // the compiled pattern
			NULL,                 // no extra data - we didn't study the pattern
			parseData,			  // the subject string
			dwSize,			      // the length of the subject
			0,                    // start at offset 0 in the subject
			0,					  // default options
			ovector,              // output vector for substring information
			OVECCOUNT);           // number of elements in the output vector


		if (rc >= 0)
		{
			if (rc == 0)
			{
				// The output vector wasn't big enough
				std::wstring log = L"WebParser.dll: [";
				log += urlData->section;
				log += L"] Too many substrings!";
				Log(LOG_ERROR, log.c_str());
			}
			else
			{
				if (urlData->stringIndex < rc)
				{
					if (urlData->debug != 0)
					{
						for (int i = 0; i < rc; ++i)
						{
							const char* substring_start = parseData + ovector[2 * i];
							int substring_length = ovector[2 * i + 1] - ovector[2 * i];
							substring_length = min(substring_length, 256);
							std::string tmpStr(substring_start, substring_length);

							WCHAR buffer[32];
							wsprintf(buffer, L"%2d", i);

							std::wstring log = L"WebParser.dll: [";
							log += urlData->section;
							log += L"] (Index ";
							log += buffer;
							log += L") ";
							log += ConvertUTF8ToWide(tmpStr.c_str());
							Log(LOG_DEBUG, log.c_str());
						}
					}

					const char* substring_start = parseData + ovector[2 * urlData->stringIndex];
					int substring_length = ovector[2 * urlData->stringIndex + 1] - ovector[2 * urlData->stringIndex];

					EnterCriticalSection(&g_CriticalSection);
					std::string szResult(substring_start, substring_length);
					urlData->resultString = DecodeReferences(ConvertUTF8ToWide(szResult.c_str()), urlData->decodeCharacterReference);
					LeaveCriticalSection(&g_CriticalSection);
				}
				else
				{
					std::wstring log = L"WebParser.dll: [";
					log += urlData->section;
					log += L"] Not enough substrings!";
					Log(LOG_WARNING, log.c_str());

					// Clear the old result
					EnterCriticalSection(&g_CriticalSection);
					urlData->resultString.clear();
					if (urlData->download)
					{
						if (urlData->downloadFile.empty())  // cache mode
						{
							if (!urlData->downloadedFile.empty())
							{
								// Delete old downloaded file
								DeleteFile(urlData->downloadedFile.c_str());
							}
						}
						urlData->downloadedFile.clear();
					}
					LeaveCriticalSection(&g_CriticalSection);
				}

				// Update the references
				std::map<UINT, UrlData*>::iterator i = g_UrlData.begin();
				std::wstring compareStr = L"[";
				compareStr += urlData->section;
				compareStr += L"]";
				for ( ; i != g_UrlData.end(); ++i)
				{
					if ((((*i).second)->url.find(compareStr) != std::wstring::npos) && (urlData->iniFile == ((*i).second)->iniFile))
					{
						if (((*i).second)->stringIndex < rc)
						{
							const char* substring_start = parseData + ovector[2 * ((*i).second)->stringIndex];
							int substring_length = ovector[2 * ((*i).second)->stringIndex + 1] - ovector[2 * ((*i).second)->stringIndex];

							std::string szResult(substring_start, substring_length);

							if (!((*i).second)->regExp.empty())
							{
								// Change the index and parse the substring
								int index = (*i).second->stringIndex;
								(*i).second->stringIndex = (*i).second->stringIndex2;
								ParseData((*i).second, szResult.c_str(), szResult.length());
								(*i).second->stringIndex = index;
							}
							else
							{
								// Set the result
								EnterCriticalSection(&g_CriticalSection);

								// Substitude the [measure] with szResult
								std::wstring wzResult = ConvertUTF8ToWide(szResult.c_str());
								std::wstring wzUrl = ((*i).second)->url;

								wzUrl.replace(wzUrl.find(compareStr), compareStr.size(), wzResult);
								((*i).second)->resultString = DecodeReferences(wzUrl, (*i).second->decodeCharacterReference);

								// Start download threads for the references
								if (((*i).second)->download)
								{
									// Start the download thread
									unsigned int id;
									HANDLE threadHandle = (HANDLE)_beginthreadex(NULL, 0, NetworkDownloadThreadProc, ((*i).second), 0, &id);
									if (threadHandle)
									{
										((*i).second)->dlThreadHandle = threadHandle;
									}
									else  // error
									{
										std::wstring log = L"WebParser.dll: [";
										log += (*i).second->section;
										log += L"] Failed to begin download thread.";
										Log(LOG_ERROR, log.c_str());
									}
								}

								LeaveCriticalSection(&g_CriticalSection);
							}
						}
						else
						{
							std::wstring log = L"WebParser.dll: [";
							log += (*i).second->section;
							log += L"] Not enough substrings!";
							Log(LOG_WARNING, log.c_str());

							// Clear the old result
							EnterCriticalSection(&g_CriticalSection);
							((*i).second)->resultString.clear();
							if ((*i).second->download)
							{
								if ((*i).second->downloadFile.empty())  // cache mode
								{
									if (!(*i).second->downloadedFile.empty())
									{
										// Delete old downloaded file
										DeleteFile((*i).second->downloadedFile.c_str());
									}
								}
								(*i).second->downloadedFile.clear();
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
			log += urlData->section;
			log += L"] Matching error! (";
			log += buffer;
			log += L")\n";
			Log(LOG_ERROR, log.c_str());

			EnterCriticalSection(&g_CriticalSection);
			urlData->resultString = urlData->errorString;

			// Update the references
			std::map<UINT, UrlData*>::iterator i = g_UrlData.begin();
			std::wstring compareStr = L"[";
			compareStr += urlData->section;
			compareStr += L"]";
			for ( ; i != g_UrlData.end(); ++i)
			{
				if ((((*i).second)->url.find(compareStr) != std::wstring::npos) && (urlData->iniFile == ((*i).second)->iniFile))
				{
					((*i).second)->resultString = ((*i).second)->errorString;
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
		log += urlData->section;
		log += L"] PCRE compilation failed at offset ";
		log += buffer;
		log += L": ";
		log += ConvertAsciiToWide(error);
		log += L"\n";
		Log(LOG_ERROR, log.c_str());
	}

	if (urlData->download)
	{
		// Start the download thread
		unsigned int id;
		HANDLE threadHandle = (HANDLE)_beginthreadex(NULL, 0, NetworkDownloadThreadProc, urlData, 0, &id);
		if (threadHandle)
		{
			urlData->dlThreadHandle = threadHandle;
		}
		else  // error
		{
			std::wstring log = L"WebParser.dll: [";
			log += urlData->section;
			log += L"] Failed to begin download thread.";
			Log(LOG_ERROR, log.c_str());
		}
	}
	else
	{
		if (!urlData->finishAction.empty())
		{
			HWND wnd = FindMeterWindow(urlData->iniFile);

			if (wnd != NULL)
			{
				COPYDATASTRUCT copyData;

				copyData.dwData = 1;
				copyData.cbData = (DWORD)(urlData->finishAction.size() + 1) * sizeof(WCHAR);
				copyData.lpData = (void*)urlData->finishAction.c_str();

				// Send the bang to the Rainmeter window
				SendMessage(wnd, WM_COPYDATA, (WPARAM)NULL, (LPARAM)&copyData);
			}
		}
	}
}

/*
  Thread that downloads the file from the new.
*/
unsigned __stdcall NetworkDownloadThreadProc(void* pParam)
{
	UrlData* urlData = (UrlData*)pParam;
	const bool download = !urlData->downloadFile.empty();
	bool ready = false;

	std::wstring url;

	if (urlData->regExp.empty() && urlData->resultString.empty())
	{
		if (!urlData->url.empty() && urlData->url[0] != L'[')
		{
			url = urlData->url;
		}
	}
	else
	{
		EnterCriticalSection(&g_CriticalSection);
		url = urlData->resultString;
		LeaveCriticalSection(&g_CriticalSection);

		std::wstring::size_type pos = url.find(L':');
		if (pos == std::wstring::npos && !url.empty())	// No protocol
		{
			// Add the base url to the string
			if (url[0] == L'/')
			{
				// Absolute path
				pos = urlData->url.find(L'/', 7);	// Assume "http://" (=7)
				if (pos != std::wstring::npos)
				{
					std::wstring path(urlData->url.substr(0, pos));
					url = path + url;
				}
			}
			else
			{
				// Relative path

				pos = urlData->url.rfind(L'/');
				if (pos != std::wstring::npos)
				{
					std::wstring path(urlData->url.substr(0, pos + 1));
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
			PathCanonicalize(buffer, urlData->downloadFile.c_str());

			std::wstring path = buffer;
			std::wstring::size_type pos = path.find_first_not_of(L'\\');
			if (pos != std::wstring::npos)
			{
				path.erase(0, pos);
			}

			PathCanonicalize(buffer, urlData->iniFile.substr(0, urlData->iniFile.find_last_of(L'\\') + 1).c_str());  // "#CURRENTPATH#"
			wcscat(buffer, L"DownloadFile\\");  // "#CURRENTPATH#DownloadFile\"
			CreateDirectory(buffer, NULL);	// Make sure that the folder exists

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
		CreateDirectory(buffer, NULL);	// Make sure that the folder exists
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
				log += urlData->section;
				log += L"] Directory does not exist: ";
				log += directory;
				Log(LOG_ERROR, log.c_str());
			}
			else if (PathIsDirectory(fullpath.c_str()))
			{
				ready = false;

				log = L"WebParser.dll: [";
				log += urlData->section;
				log += L"] Path is a directory, not a file: ";
				log += fullpath;
				Log(LOG_ERROR, log.c_str());
			}
			else if (PathFileExists(fullpath.c_str()))
			{
				DWORD attr = GetFileAttributes(fullpath.c_str());
				if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_READONLY))
				{
					ready = false;

					log = L"WebParser.dll: [";
					log += urlData->section;
					log += L"] File is READ-ONLY: ";
					log += fullpath;
					Log(LOG_ERROR, log.c_str());
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
			HANDLE hFile = CreateFile(fullpath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
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
					ret = RegQueryValueEx(hKey, L"SyncMode5", NULL, NULL, (LPBYTE)&mode, &size);
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
			log += urlData->section;
			log += L"] Downloading url ";
			log += url;
			log += L" to ";
			log += fullpath;
			log += L"\n";
			Log(LOG_DEBUG, log.c_str());

			HRESULT resultCoInitialize = CoInitialize(NULL);  // requires before calling URLDownloadToFile function

			// Download the file
			HRESULT result = URLDownloadToFile(NULL, url.c_str(), fullpath.c_str(), NULL, NULL);
			if (result == S_OK)
			{
				EnterCriticalSection(&g_CriticalSection);

				if (!download)  // cache mode
				{
					if (!urlData->downloadedFile.empty())
					{
						// Delete old downloaded file
						DeleteFile(urlData->downloadedFile.c_str());
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
				urlData->downloadedFile = fullpath;

				LeaveCriticalSection(&g_CriticalSection);

				if (!urlData->finishAction.empty())
				{
					HWND wnd = FindMeterWindow(urlData->iniFile);

					if (wnd != NULL)
					{
						COPYDATASTRUCT copyData;

						copyData.dwData = 1;
						copyData.cbData = (DWORD)(urlData->finishAction.size() + 1) * sizeof(WCHAR);
						copyData.lpData = (void*)urlData->finishAction.c_str();

						// Send the bang to the Rainmeter window
						SendMessage(wnd, WM_COPYDATA, (WPARAM)NULL, (LPARAM)&copyData);
					}
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
				log += urlData->section;
				log += L"] Download failed (";
				log += buffer;
				log += L"): ";
				log += url;
				Log(LOG_ERROR, log.c_str());
			}

			if (SUCCEEDED(resultCoInitialize))
			{
				CoUninitialize();
			}
		}
		else
		{
			std::wstring log = L"WebParser.dll: [";
			log += urlData->section;
			log += L"] Download failed: ";
			log += url;
			Log(LOG_ERROR, log.c_str());
		}
	}
	else
	{
		std::wstring log = L"WebParser.dll: [";
		log += urlData->section;
		log += L"] The url is empty.\n";
		Log(LOG_ERROR, log.c_str());
	}

	if (!ready) // download failed
	{
		EnterCriticalSection(&g_CriticalSection);

		if (!download) // cache mode
		{
			if (!urlData->downloadedFile.empty())
			{
				// Delete old downloaded file
				DeleteFile(urlData->downloadedFile.c_str());
			}
		}

		// Clear old downloaded filename
		urlData->downloadedFile.clear();

		LeaveCriticalSection(&g_CriticalSection);
	}

	EnterCriticalSection(&g_CriticalSection);
	CloseHandle(urlData->dlThreadHandle);
	urlData->dlThreadHandle = 0;
	LeaveCriticalSection(&g_CriticalSection);

	return 0;   // thread completed successfully
}

/*
  This function is called when the value should be
  returned as a string.
*/
LPCTSTR GetString(UINT id, UINT flags)
{
	static std::wstring resultString;

	std::map<UINT, UrlData*>::iterator urlIter = g_UrlData.find(id);

	if (urlIter != g_UrlData.end())
	{
		EnterCriticalSection(&g_CriticalSection);
		if (((*urlIter).second)->download)
		{
			resultString = ((*urlIter).second)->downloadedFile;
		}
		else
		{
			resultString = ((*urlIter).second)->resultString;
		}
		LeaveCriticalSection(&g_CriticalSection);

		return resultString.c_str();
	}

	return NULL;
}

/*
  If the measure needs to free resources before quitting.
  The plugin can export Finalize function, which is called
  when Rainmeter quits (or refreshes).
*/
void Finalize(HMODULE instance, UINT id)
{
	std::map<UINT, UrlData*>::iterator urlIter = g_UrlData.find(id);

	if (urlIter != g_UrlData.end())
	{
		if (((*urlIter).second)->threadHandle)
		{
			// Thread is killed inside critical section so that itself is not inside one when it is terminated
			EnterCriticalSection(&g_CriticalSection);

			TerminateThread(((*urlIter).second)->threadHandle, 0);
			(*urlIter).second->threadHandle = NULL;

			LeaveCriticalSection(&g_CriticalSection);
		}

		if (((*urlIter).second)->dlThreadHandle)
		{
			// Thread is killed inside critical section so that itself is not inside one when it is terminated
			EnterCriticalSection(&g_CriticalSection);

			TerminateThread(((*urlIter).second)->dlThreadHandle, 0);
			(*urlIter).second->dlThreadHandle = NULL;

			LeaveCriticalSection(&g_CriticalSection);
		}

		if (((*urlIter).second)->downloadFile.empty())  // cache mode
		{
			if (!((*urlIter).second)->downloadedFile.empty())
			{
				// Delete the file
				DeleteFile(((*urlIter).second)->downloadedFile.c_str());
			}
		}

		delete (*urlIter).second;
		g_UrlData.erase(urlIter);
	}

	if (g_UrlData.empty())
	{
		// Last one, close all handles
		if (hRootHandle)
		{
			InternetCloseHandle(hRootHandle);
			hRootHandle = NULL;
		}

		g_CERs.clear();

		// Last instance deletes the critical section
		DeleteCriticalSection(&g_CriticalSection);
		g_Initialized = false;
	}
}

/*
	Downloads the given url and returns the webpage as dynamically allocated string.
	You need to delete the returned string after use!
*/
BYTE* DownloadUrl(std::wstring& url, DWORD* dwDataSize, bool forceReload)
{
	HINTERNET hUrlDump;
	DWORD dwSize;
	BYTE* lpData;
	BYTE* lpOutPut;
	BYTE* lpHolding = NULL;
	int nCounter = 1;
	int nBufferSize;
	const int CHUNK_SIZE = 8192;

	std::wstring err = L"WebParser.dll: Fetching: " + url;
	Log(LOG_DEBUG, err.c_str());

	DWORD flags = INTERNET_FLAG_RESYNCHRONIZE;
	if (forceReload)
	{
		flags = INTERNET_FLAG_RELOAD;
	}

	hUrlDump = InternetOpenUrl(hRootHandle, url.c_str(), NULL, NULL, flags, 0);
	if (hUrlDump == NULL)
	{
		if (_wcsnicmp(url.c_str(), L"file://", 7) == 0)  // file scheme
		{
			std::string urlACP = ConvertWideToAscii(url.c_str());
			hUrlDump = InternetOpenUrlA(hRootHandle, urlACP.c_str(), NULL, NULL, flags, 0);
		}

		if (hUrlDump == NULL)
		{
			ShowError(__LINE__);
			return NULL;
		}
	}

	*dwDataSize = 0;

	// Allocate the buffer.
	lpData = new BYTE[CHUNK_SIZE];

	do
	{
		// Read the data.
		if (!InternetReadFile(hUrlDump, (LPVOID)lpData, CHUNK_SIZE, &dwSize))
		{
			ShowError(__LINE__);
			break;
		}
		else
		{
			// Check if all of the data has been read.  This should
			// never get called on the first time through the loop.
			if (dwSize == 0)
			{
				break;
			}

			// Determine the buffer size to hold the new data and the data
			// already written (if any).
			nBufferSize = *dwDataSize + dwSize;

			// Allocate the output buffer.
			lpOutPut = new BYTE[nBufferSize + 3];

			// Make sure the buffer is not the initial buffer.
			if (lpHolding != NULL)
			{
				// Copy the data in the holding buffer.
				memcpy(lpOutPut, lpHolding, *dwDataSize);

				// Delete the old buffer
				delete [] lpHolding;

				lpHolding = lpOutPut;
				lpOutPut = lpOutPut + *dwDataSize;
			}
			else
			{
				lpHolding = lpOutPut;
			}

			// Copy the data buffer.
			memcpy(lpOutPut, lpData, dwSize);

			*dwDataSize += dwSize;

			// End with triple null
			lpOutPut[dwSize] = 0;
			lpOutPut[dwSize + 1] = 0;
			lpOutPut[dwSize + 2] = 0;

			// Increment the number of buffers read.
			++nCounter;

			// Clear the buffer
			memset(lpData, 0, CHUNK_SIZE);
		}
	} while (TRUE);

	// Close the HINTERNET handle.
	InternetCloseHandle(hUrlDump);

	// Delete the existing buffers.
	delete [] lpData;

	// Return.
	return lpHolding;
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

	if (errorMsg == NULL)
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
			err += L")";
		}
		else
		{
			LPVOID lpMsgBuf = NULL;

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
				NULL
			);

			if (lpMsgBuf == NULL)
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
			err += L")";
		}
	}
	else
	{
		err += errorMsg;
	}

	Log(LOG_ERROR, err.c_str());
}

/*
  Writes the log to a file
*/
void Log(int level, const WCHAR* string)
{
	LSLog(level, NULL, string);
}

UINT GetPluginVersion()
{
	return 1013;
}

LPCTSTR GetPluginAuthor()
{
	return L"Rainy (rainy@iki.fi)";
}