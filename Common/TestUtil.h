// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "UnitTest.h"

#include <Windows.h>
#include <cstdio>
#include <string>

class TemporaryFile
{
public:
	TemporaryFile(const WCHAR* prefix = L"TST")
	{
		WCHAR folder[MAX_PATH];
		Assert::IsTrue(GetTempPath(_countof(folder), folder) != 0);

		WCHAR path[MAX_PATH];
		Assert::IsTrue(GetTempFileName(folder, prefix, 0, path) != 0);
		m_Path = path;
	}

	TemporaryFile(const TemporaryFile&) = delete;
	TemporaryFile& operator=(const TemporaryFile&) = delete;

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
