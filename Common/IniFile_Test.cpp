// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "IniFile.h"
#include "UnitTest.h"

#include <Windows.h>
#include <cstdio>
#include <string>
#include <vector>

namespace IniFile {

class TemporaryFile
{
public:
	TemporaryFile()
	{
		WCHAR folder[MAX_PATH];
		Assert::IsTrue(GetTempPath(_countof(folder), folder) != 0);

		WCHAR path[MAX_PATH];
		Assert::IsTrue(GetTempFileName(folder, L"INI", 0, path) != 0);
		m_Path = path;
	}

	~TemporaryFile()
	{
		SetFileAttributes(m_Path.c_str(), FILE_ATTRIBUTE_NORMAL);
		DeleteFile(m_Path.c_str());
	}

	const std::wstring& GetPath() const { return m_Path; }

	void Remove() const
	{
		Assert::IsTrue(DeleteFile(m_Path.c_str()) != FALSE);
	}

	void Write(const std::string& bytes) const
	{
		FILE* file;
		Assert::AreEqual(0, _wfopen_s(&file, m_Path.c_str(), L"wb"));
		Assert::AreEqual(bytes.size(), fwrite(bytes.data(), 1, bytes.size(), file));
		Assert::AreEqual(0, fclose(file));
	}

	std::string Read() const
	{
		FILE* file;
		Assert::AreEqual(0, _wfopen_s(&file, m_Path.c_str(), L"rb"));
		Assert::AreEqual(0, fseek(file, 0, SEEK_END));
		const long size = ftell(file);
		Assert::IsTrue(size >= 0);
		Assert::AreEqual(0, fseek(file, 0, SEEK_SET));

		std::string bytes((size_t)size, '\0');
		Assert::AreEqual(bytes.size(), fread(bytes.data(), 1, bytes.size(), file));
		Assert::AreEqual(0, fclose(file));
		return bytes;
	}

private:
	std::wstring m_Path;
};

std::string ToBytes(const std::wstring& text)
{
	return std::string((const char*)text.data(), text.length() * sizeof(WCHAR));
}

TEST_CLASS(Common_IniFile_Test)
{
public:
	TEST_METHOD(TestReadSectionAndKey)
	{
		TemporaryFile file;
		file.Write(
			"[Alpha]\r\nOne=first\r\noNe=second\r\nEmpty=\"\"\r\n"
			"[aLPHa]\r\nOne=later\r\n");

		const auto values = ReadSection(file.GetPath(), L" alpha ");
		Assert::IsFalse(values.IsEmpty());
		Assert::AreEqual(L"first", values.GetKey(L"ONE", L"").c_str());
		Assert::AreEqual(L"", values.GetKey(L"empty", L"missing").c_str());
		Assert::AreEqual(L"default", values.GetKey(L"missing", L"default").c_str());

		const auto ordered = ReadSectionInOrder(file.GetPath(), L"Alpha");
		Assert::AreEqual((size_t)3, ordered.size());
		Assert::AreEqual(L"One", ordered[0].first.c_str());
		Assert::AreEqual(L"first", ordered[0].second.c_str());
		Assert::AreEqual(L"oNe", ordered[1].first.c_str());
		Assert::AreEqual(L"second", ordered[1].second.c_str());
		Assert::AreEqual(L"Empty", ordered[2].first.c_str());
		Assert::AreEqual(L"", ordered[2].second.c_str());

		const auto sections = ReadSectionNames(file.GetPath());
		Assert::AreEqual((size_t)1, sections.size());
		Assert::AreEqual(L"Alpha", sections[0].c_str());

		const auto value = ReadKey(file.GetPath(), L"ALPHA", L"one");
		Assert::IsTrue(value.has_value());
		Assert::AreEqual(L"first", value->c_str());
		Assert::IsTrue(ReadKey(file.GetPath(), L"Alpha", L"Empty").has_value());
		Assert::IsFalse(ReadKey(file.GetPath(), L"Alpha", L"Missing").has_value());
		Assert::AreEqual(L"first", ReadKey(file.GetPath(), L"Alpha", L"One", L"default").c_str());
		Assert::AreEqual(L"", ReadKey(file.GetPath(), L"Alpha", L"Empty", L"default").c_str());
		Assert::AreEqual(L"default", ReadKey(file.GetPath(), L"Alpha", L"Missing", L"default").c_str());
	}

	TEST_METHOD(TestReadIntKey)
	{
		TemporaryFile file;
		file.Write("[Alpha]\r\nNumber=-12\r\nInvalid=no\r\nEmpty=\r\n");

		Assert::AreEqual(-12, ReadIntKey(file.GetPath(), L"Alpha", L"Number", 7));
		Assert::AreEqual(0, ReadIntKey(file.GetPath(), L"Alpha", L"Invalid", 7));
		Assert::AreEqual(0, ReadIntKey(file.GetPath(), L"Alpha", L"Empty", 7));
		Assert::AreEqual(7, ReadIntKey(file.GetPath(), L"Alpha", L"Missing", 7));

		const auto section = ReadSection(file.GetPath(), L"Alpha");
		Assert::AreEqual(-12, section.GetIntKey(L"Number", 7));
		Assert::AreEqual(0, section.GetIntKey(L"Invalid", 7));
		Assert::AreEqual(0, section.GetIntKey(L"Empty", 7));
		Assert::AreEqual(7, section.GetIntKey(L"Missing", 7));
	}

	TEST_METHOD(TestCreateFile)
	{
		TemporaryFile file;
		file.Remove();

		Assert::IsTrue(WriteKey(file.GetPath(), L"Section", L"Key", L"Value"));
		Assert::AreEqual("[Section]\r\nKey=Value\r\n", file.Read().c_str());
	}

	TEST_METHOD(TestOverwriteAndAppendKey)
	{
		TemporaryFile file;
		file.Write("[Alpha]\n  One   = 1 ; comment\n\n[Bravo]\nThree=3");

		Assert::IsTrue(WriteKey(file.GetPath(), L"alpha", L"one", L"new"));
		Assert::AreEqual("[Alpha]\n  One   =new\r\n\n[Bravo]\nThree=3", file.Read().c_str());

		Assert::IsTrue(WriteKey(file.GetPath(), L"Alpha", L"Added", L"a"));
		Assert::AreEqual("[Alpha]\n  One   =new\r\nAdded=a\r\n\n[Bravo]\nThree=3", file.Read().c_str());
	}

	TEST_METHOD(TestFirstDuplicateWins)
	{
		TemporaryFile file;
		file.Write("[Dup]\r\nKey=first\r\nKey=second\r\n[Dup]\r\nKey=third\r\n");

		Assert::IsTrue(WriteKey(file.GetPath(), L"Dup", L"Key", L"changed"));
		Assert::AreEqual("[Dup]\r\nKey=changed\r\nKey=second\r\n[Dup]\r\nKey=third\r\n", file.Read().c_str());
	}

	TEST_METHOD(TestDeleteKeyAndSection)
	{
		TemporaryFile file;
		file.Write(
			"[Alpha]\r\nOne=1\r\nTwo=2\r\n\r\n"
			"[Bravo]\r\n; comment\r\nThree=3\r\n\r\n"
			"[Charlie]\r\nFour=4\r\n");

		Assert::IsTrue(DeleteKey(file.GetPath(), L"Alpha", L"One"));
		Assert::IsTrue(DeleteSection(file.GetPath(), L"Bravo"));
		Assert::AreEqual(
			"[Alpha]\r\nTwo=2\r\n\r\n\r\n[Charlie]\r\nFour=4\r\n",
			file.Read().c_str());
	}

	TEST_METHOD(TestWriteSection)
	{
		TemporaryFile file;
		file.Write("[Alpha]\n; comment\nOne=1\n\n[Bravo]\nTwo=2");

		Assert::IsTrue(WriteSection(file.GetPath(), L"Alpha", { L"A=1", L"NoEquals", L" C = 3 " }));
		Assert::AreEqual(
			"[Alpha]\nA=1\r\nNoEquals\r\n C = 3 \r\n\n[Bravo]\nTwo=2",
			file.Read().c_str());

		Assert::IsTrue(WriteSection(file.GetPath(), L"Bravo", {}));
		Assert::AreEqual(
			"[Alpha]\nA=1\r\nNoEquals\r\n C = 3 \r\n\n[Bravo]\n",
			file.Read().c_str());
	}

	TEST_METHOD(TestWriter)
	{
		TemporaryFile file;
		file.Write(
			"[Alpha]\r\nOne=1\r\nOld=old\r\n\r\n"
			"[Bravo]\r\nTwo=2\r\n");

		Writer ini(file.GetPath());
		Assert::IsTrue(ini.IsValid());
		ini.WriteKey(L"Alpha", L"One", L"changed");
		ini.DeleteKey(L"Alpha", L"Old");
		ini.WriteKey(L"Alpha", L"Added", L"new");
		ini.WriteSection(L"Bravo", { L"Replacement=3" });
		ini.WriteKey(L"Charlie", L"Four", L"4");

		Assert::IsTrue(ini.Save());
		Assert::AreEqual(
			"[Alpha]\r\nOne=changed\r\nAdded=new\r\n\r\n[Bravo]\r\nReplacement=3\r\n[Charlie]\r\nFour=4\r\n",
			file.Read().c_str());
	}

	TEST_METHOD(TestUnchangedWriterDoesNotCreateFile)
	{
		TemporaryFile file;
		file.Remove();

		Writer ini(file.GetPath());
		Assert::IsTrue(ini.Save());
		Assert::AreEqual(INVALID_FILE_ATTRIBUTES, GetFileAttributes(file.GetPath().c_str()));
	}

	TEST_METHOD(TestDeleteOnlyWriterDoesNotCreateFile)
	{
		TemporaryFile file;
		file.Remove();

		Writer ini(file.GetPath());
		ini.DeleteKey(L"Missing", L"Key");
		ini.DeleteSection(L"Missing");
		Assert::IsTrue(ini.Save());
		Assert::AreEqual(INVALID_FILE_ATTRIBUTES, GetFileAttributes(file.GetPath().c_str()));
	}

	TEST_METHOD(TestInvalidWriterDoesNotSaveEarlierChanges)
	{
		TemporaryFile file;
		file.Write("[Alpha]\r\nOne=1\r\n");

		Writer ini(file.GetPath());
		ini.WriteKey(L"Alpha", L"One", L"changed");
		ini.WriteKey(L"Alpha", L"Two", std::wstring(L"bad\0value", 9));

		Assert::IsFalse(ini.IsValid());
		Assert::IsFalse(ini.Save());
		Assert::AreEqual("[Alpha]\r\nOne=1\r\n", file.Read().c_str());
	}

	TEST_METHOD(TestUtf16WithoutBomStaysUtf16WithoutBom)
	{
		TemporaryFile file;
		file.Write(ToBytes(L"[Alpha]\r\nOne=1\r\n"));

		Assert::IsTrue(WriteKey(file.GetPath(), L"Alpha", L"Two", L"\u65E5\u672C"));
		Assert::IsTrue(ToBytes(L"[Alpha]\r\nOne=1\r\nTwo=\u65E5\u672C\r\n") == file.Read());
	}

	TEST_METHOD(TestUtf16BomOnlyStaysUtf16)
	{
		TemporaryFile file;
		file.Write("\xFF\xFE");

		const auto text = ReadFileText(file.GetPath());
		Assert::IsTrue(text.has_value());
		Assert::IsTrue(text->IsEmpty());
		Assert::IsTrue(WriteKey(file.GetPath(), L"Alpha", L"One", L"1"));
		Assert::IsTrue(std::string("\xFF\xFE") + ToBytes(L"[Alpha]\r\nOne=1\r\n") == file.Read());
	}

	TEST_METHOD(TestUtf8WithoutBomStaysUtf8WithoutBom)
	{
		TemporaryFile file;
		file.Write("[Alpha]\r\nOne=\xC3\xA9\r\n");

		Assert::AreEqual(L"\u00E9", ReadKey(file.GetPath(), L"Alpha", L"One", L"").c_str());
		Assert::IsTrue(WriteKey(file.GetPath(), L"Alpha", L"Two", L"\u65E5"));
		Assert::AreEqual("[Alpha]\r\nOne=\xC3\xA9\r\nTwo=\xE6\x97\xA5\r\n", file.Read().c_str());
	}

	TEST_METHOD(TestAsciiFileWritesAsUtf8)
	{
		TemporaryFile file;
		file.Write("[Alpha]\r\nOne=1\r\n");

		Assert::IsTrue(WriteKey(file.GetPath(), L"Alpha", L"Two", L"\u65E5"));
		Assert::AreEqual("[Alpha]\r\nOne=1\r\nTwo=\xE6\x97\xA5\r\n", file.Read().c_str());
	}

	TEST_METHOD(TestUtf8BomStaysUtf8WithBom)
	{
		TemporaryFile file;
		file.Write("\xEF\xBB\xBF[Alpha]\r\nOne=1\r\n");

		Assert::IsTrue(WriteKey(file.GetPath(), L"Alpha", L"Two", L"2"));
		Assert::AreEqual("\xEF\xBB\xBF[Alpha]\r\nOne=1\r\nTwo=2\r\n", file.Read().c_str());
	}

	TEST_METHOD(TestUtf8BomOnlyStaysUtf8)
	{
		TemporaryFile file;
		file.Write("\xEF\xBB\xBF");

		const auto text = ReadFileText(file.GetPath());
		Assert::IsTrue(text.has_value());
		Assert::IsTrue(text->IsEmpty());
		Assert::IsTrue(WriteKey(file.GetPath(), L"Alpha", L"One", L"\u65E5"));
		Assert::AreEqual("\xEF\xBB\xBF[Alpha]\r\nOne=\xE6\x97\xA5\r\n", file.Read().c_str());
	}

	TEST_METHOD(TestInvalidUtf8FallsBackToAnsi)
	{
		const BYTE bytes[] = { 0xC3, 0xA9, 0xFF };
		const auto text = DecodedText::FromMemory(bytes, _countof(bytes));

		Assert::IsTrue(text.GetEncoding() == Encoding::ANSI);
	}
};

}  // namespace IniFile
