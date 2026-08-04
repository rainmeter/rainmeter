// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include <malloc.h>
#include <string>
#include <string.h>
#include <string_view>

class RawString
{
public:
	RawString() {}

	RawString(const wchar_t* str) :
		m_String(str_alloc(str))
	{
	}

	RawString(std::wstring_view str) :
		m_String(str_alloc(str.data(), str.size()))
	{
	}

	RawString(const std::wstring& str) :
		RawString(std::wstring_view(str))
	{
	}

	RawString(const RawString& rhs) :
		m_String(str_alloc(rhs.m_String))
	{
	}

	~RawString()
	{
		clear();
	}

	RawString& operator=(const wchar_t* rhs)
	{
		assign(rhs);
		return *this;
	}

	RawString& operator=(std::wstring_view rhs)
	{
		assign(rhs.data(), rhs.size());
		return *this;
	}

	RawString& operator=(const std::wstring& rhs)
	{
		return operator=(std::wstring_view(rhs));
	}

	RawString& operator=(const RawString& rhs)
	{
		if (&rhs != this)
		{
			assign(rhs.m_String);
		}
		return *this;
	}

	const wchar_t* c_str() const
	{
		return m_String ? m_String : L"";
	}

	bool empty() const
	{
		return !m_String || !(*m_String);
	}

	void clear()
	{
		if (m_String)
		{
			free(m_String);
			m_String = nullptr;
		}
	}

private:
	wchar_t* str_alloc(const wchar_t* str)
	{
		if (!str) return nullptr;

		wchar_t* buffer = (wchar_t*)malloc((wcslen(str) + 1) * sizeof(wchar_t));
		if (buffer) wcscpy(buffer, str);
		return buffer;
	}

	wchar_t* str_alloc(const wchar_t* str, size_t length)
	{
		wchar_t* buffer = (wchar_t*)malloc((length + 1) * sizeof(wchar_t));
		if (buffer)
		{
			if (length) wmemcpy(buffer, str, length);
			buffer[length] = L'\0';
		}
		return buffer;
	}

	void assign(const wchar_t* str)
	{
		if (str == m_String) return;

		if (!str)
		{
			clear();
			return;
		}

		const size_t size = (wcslen(str) + 1) * sizeof(wchar_t);
		wchar_t* buffer = (wchar_t*)realloc(m_String, size);
		if (buffer)
		{
			m_String = buffer;
			wcscpy(m_String, str);
		}
	}

	void assign(const wchar_t* str, size_t length)
	{
		if (m_String && str == m_String && length == wcslen(m_String)) return;

		const size_t size = (length + 1) * sizeof(wchar_t);
		wchar_t* buffer = (wchar_t*)realloc(m_String, size);
		if (buffer)
		{
			m_String = buffer;
			if (length) wmemcpy(m_String, str, length);
			m_String[length] = L'\0';
		}
	}

	wchar_t* m_String = nullptr;
};
