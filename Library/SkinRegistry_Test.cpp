/*
  Copyright (C) 2013 Rainmeter Team

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

#include "SkinRegistry.h"
#include "../Common/UnitTest.h"

TEST_CLASS(Library_SkinRegistry_Test)
{
public:
	Library_SkinRegistry_Test()
	{
		m_SkinRegistry.Populate(L"..\\..\\..\\Library\\Test\\SkinRegistry\\");
	}

	TEST_METHOD(TestContents)
	{
		std::vector<std::wstring> files1;
		files1.push_back(L"1.ini");

		std::vector<std::wstring> files3;
		files3.push_back(L"1.ini");
		files3.push_back(L"2.ini");
		files3.push_back(L"3.ini");

		Assert::AreEqual(5, m_SkinRegistry.GetFolderCount());

		const auto& folderA1 = m_SkinRegistry.GetFolder(0);
		Assert::AreEqual(L"A1", folderA1.name.c_str());
		Assert::AreEqual((int16_t)1, folderA1.level);
		Assert::IsTrue(folderA1.files.empty());

		const auto& folderA1_B1 = m_SkinRegistry.GetFolder(1);
		Assert::AreEqual(L"B1", folderA1_B1.name.c_str());
		Assert::AreEqual((int16_t)2, folderA1_B1.level);
		Assert::IsTrue(files1 == folderA1_B1.files);

		const auto& folderA1_B2 = m_SkinRegistry.GetFolder(2);
		Assert::AreEqual(L"B2", folderA1_B2.name.c_str());
		Assert::AreEqual((int16_t)2, folderA1_B2.level);
		Assert::IsTrue(files1 == folderA1_B2.files);

		const auto& folderA1_B2_C1 = m_SkinRegistry.GetFolder(3);
		Assert::AreEqual(L"C1", folderA1_B2_C1.name.c_str());
		Assert::AreEqual((int16_t)3, folderA1_B2_C1.level);
		Assert::IsTrue(files1 == folderA1_B2_C1.files);

		const auto& folderA2 = m_SkinRegistry.GetFolder(4);
		Assert::AreEqual(L"A2", folderA2.name.c_str());
		Assert::AreEqual((int16_t)1, folderA2.level);
		Assert::IsTrue(files3 == folderA2.files);
	}

	TEST_METHOD(TestFindFolderIndex)
	{
		Assert::AreEqual(3, m_SkinRegistry.FindFolderIndex(L"A1\\B2\\C1"));
		Assert::AreEqual(-1, m_SkinRegistry.FindFolderIndex(L"A1\\B5\\C1"));
	}

	TEST_METHOD(TestFindIndexes)
	{
		const auto indexes1 = m_SkinRegistry.FindIndexes(L"A1\\B2", L"1.ini");
		Assert::IsTrue(indexes1.folder == 2 && indexes1.file == 0);
		
		const auto indexes2 = m_SkinRegistry.FindIndexes(L"A2", L"2.ini");
		Assert::IsTrue(indexes2.folder == 4 && indexes2.file == 1);

		const auto indexes3 = m_SkinRegistry.FindIndexes(L"A3", L"1.ini");
		Assert::IsFalse(indexes3.IsValid());
	}

	TEST_METHOD(TestFindIndexesForID)
	{
		const auto indexes1 = m_SkinRegistry.FindIndexesForID(30002);
		Assert::IsTrue(indexes1.folder == 2 && indexes1.file == 0);

		const auto indexes2 = m_SkinRegistry.FindIndexesForID(30005);
		Assert::IsTrue(indexes2.folder == 4 && indexes2.file == 1);
	}

	TEST_METHOD(TestGetFolderPath)
	{
		Assert::AreEqual(L"A1\\B2\\C1", m_SkinRegistry.GetFolderPath(3).c_str());
		Assert::AreEqual(L"A2", m_SkinRegistry.GetFolderPath(4).c_str());
	}

private:
	SkinRegistry m_SkinRegistry;
};
