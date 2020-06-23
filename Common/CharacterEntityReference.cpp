/* Copyright (C) 2015 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "CharacterEntityReference.h"

namespace CharacterEntityReference {

namespace {

struct Entity
{
	WCHAR* name;
	WCHAR ch;
};

// List from:
// http://www.w3.org/TR/html4/sgml/entities.html
// http://www.w3.org/TR/xhtml1/#C_16
static Entity kEntities[] =
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

WCHAR GetEntityChar(const std::wstring& entity)
{
	static std::unordered_map<std::wstring, WCHAR> s_Map = []()
	{
		std::unordered_map<std::wstring, WCHAR> map;
		const size_t entityCount = _countof(kEntities);
		map.rehash(entityCount);
		for (size_t i = 0; i < entityCount; ++i)
		{
			map.insert(std::make_pair(kEntities[i].name, kEntities[i].ch));
		}
		return map;
	} ();

	auto it = s_Map.find(entity);
	return it != s_Map.end() ? it->second : (WCHAR)0;
}

} // namespace

void Decode(std::wstring& str, int opt)
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

				WCHAR ch = GetEntityChar(name);
				if (ch)
				{
					str.replace(start, end - start + 1, 1, ch);
				}
				++start;
			}
		}
	}
}

} // namespace CharacterEntityReference
