// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include <string_view>

#include "../ThirdParty/pcre/config.h"
#include "../ThirdParty/pcre/pcre.h"

class Pcre
{
public:
	Pcre() : m_Pcre(nullptr), m_ErrorOffset(0), m_Offset(0) {}

	Pcre(const WCHAR* pattern, const char** error) : m_Pcre(nullptr), m_ErrorOffset(0), m_Offset(0)
	{
		Compile(pattern, error);
	}

	~Pcre() { Reset(); }

	Pcre(const Pcre&) = delete;
	Pcre& operator=(const Pcre&) = delete;

	void Compile(const WCHAR* pattern, const char** error)
	{
		Reset();
		m_Pcre = pcre16_compile((PCRE_SPTR16)pattern, 0, error, &m_ErrorOffset, nullptr);
	}

	void Reset()
	{
		pcre16_free(m_Pcre);
		m_Pcre = nullptr;
		m_ErrorOffset = 0;
		m_Offset = 0;
	}

	int Execute(std::wstring_view subject, int options, int* offsets, int offsetCount) const
	{
		return pcre16_exec(m_Pcre, nullptr, (PCRE_SPTR16)subject.data(), (int)subject.length(), m_Offset, options, offsets, offsetCount);
	}

	int GetErrorOffset() const { return m_ErrorOffset; }
	void SetOffset(int offset) { m_Offset = offset; }

	explicit operator bool() const { return m_Pcre != nullptr; }

private:
	pcre16* m_Pcre;
	int m_ErrorOffset;
	int m_Offset;
};
