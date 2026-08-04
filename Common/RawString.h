// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include <malloc.h>
#include <string.h>

class RawString
{
public:
	RawString() {}

	RawString(const wchar_t* str) :
		m_String(str_alloc(str))
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

	wchar_t* m_String = nullptr;
};
