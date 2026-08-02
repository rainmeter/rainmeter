// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include <malloc.h>

class RawString
{
public:
	RawString() :
		m_String()
	{
	}

	RawString(const wchar_t* str) :
		m_String(str_alloc(str))
	{
	}

	RawString(const RawString& rhs) :
		m_String(str_alloc(rhs.c_str()))
	{
	}

	~RawString()
	{
		clear();
	}

	RawString& operator=(const wchar_t* rhs)
	{
		clear();
		m_String = str_alloc(rhs);
		return *this;
	}

	RawString& operator=(const RawString& rhs)
	{
		if (&rhs != this)
		{
			clear();
			m_String = str_alloc(rhs.m_String);
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
		return str ? _wcsdup(str) : nullptr;
	}

	wchar_t* m_String;
};
