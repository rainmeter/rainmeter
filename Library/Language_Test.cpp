/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "Language.h"
#include "resource.h"
#include "../Common/UnitTest.h"

extern "C" IMAGE_DOS_HEADER __ImageBase;

TEST_CLASS(Library_Language_Test)
{
public:
	TEST_METHOD(TestEnglishLanguageLoads)
	{
		WCHAR modulePath[MAX_PATH];
		const DWORD length = GetModuleFileNameW((HMODULE)&__ImageBase, modulePath, _countof(modulePath));
		Assert::IsTrue(length > 0 && length < _countof(modulePath));

		std::wstring languageDirectory(modulePath, length);
		languageDirectory.resize(languageDirectory.find_last_of(L"\\/") + 1);
		languageDirectory += L"Languages\\";

		Language language;
		Assert::IsTrue(language.Load(languageDirectory, L"1033"));
		Assert::IsTrue(language.IsLoaded());
		Assert::AreEqual((LCID)1033, language.GetLCID());
		Assert::AreEqual((unsigned short)60, language.GetButtonWidth());
		Assert::AreEqual((unsigned short)65, language.GetLabelWidth());
		Assert::IsFalse(language.IsRTL());
		Assert::AreEqual(L"About", language.GetString(IDS_About));
	}
};
