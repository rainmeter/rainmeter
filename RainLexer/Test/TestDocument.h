// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include <cstdint>

#include "ILexer.h"
#include "Scintilla.h"

#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

// An in-memory document that a lexer can be run against, implementing just enough of
// Scintilla's IDocument to stand in for the real editor. Only the eight bit code page is
// supported, which is all the Rainmeter lexer inspects.
class TestDocument final : public Scintilla::IDocument
{
public:
	explicit TestDocument(std::string text) :
		m_Text(std::move(text)),
		m_Styles(m_Text.size(), 0),
		m_LineStates(1, 0)
	{
		m_LineStarts.push_back(0);
		for (size_t i = 0; i < m_Text.size(); ++i)
		{
			if (m_Text[i] == '\n' && i + 1 < m_Text.size())
			{
				m_LineStarts.push_back(static_cast<Sci_Position>(i + 1));
			}
		}

		m_Levels.assign(m_LineStarts.size(), SC_FOLDLEVELBASE);
		m_LineStates.assign(m_LineStarts.size(), 0);
	}

	const std::string& Text() const { return m_Text; }
	const std::vector<char>& Styles() const { return m_Styles; }
	const std::vector<int>& Levels() const { return m_Levels; }
	Sci_Position EndStyled() const { return m_EndStyled; }
	Sci_Position LineCount() const { return static_cast<Sci_Position>(m_LineStarts.size()); }

	//
	// IDocument
	//

	int SCI_METHOD Version() const override { return Scintilla::dvRelease4; }

	void SCI_METHOD SetErrorStatus(int status) override { m_ErrorStatus = status; }

	Sci_Position SCI_METHOD Length() const override
	{
		return static_cast<Sci_Position>(m_Text.size());
	}

	void SCI_METHOD GetCharRange(char* buffer, Sci_Position position, Sci_Position lengthRetrieve) const override
	{
		if (position < 0 || lengthRetrieve <= 0 || position + lengthRetrieve > Length())
		{
			// Scintilla never asks for a range outside the document, so treat it as a
			// failure of the lexer rather than something to paper over.
			std::abort();
		}

		memcpy(buffer, m_Text.data() + position, static_cast<size_t>(lengthRetrieve));
	}

	char SCI_METHOD StyleAt(Sci_Position position) const override
	{
		// Scintilla's SplitVector silently ignores out of range access, so mirror that
		// rather than crashing: a lexer reading out of bounds is caught by the guard
		// rails in the runner instead.
		if (position < 0 || position >= Length())
		{
			return 0;
		}

		return m_Styles[static_cast<size_t>(position)];
	}

	Sci_Position SCI_METHOD LineFromPosition(Sci_Position position) const override
	{
		if (position <= 0)
		{
			return 0;
		}

		const auto it = std::upper_bound(m_LineStarts.begin(), m_LineStarts.end(), position);
		return static_cast<Sci_Position>(std::distance(m_LineStarts.begin(), it) - 1);
	}

	Sci_Position SCI_METHOD LineStart(Sci_Position line) const override
	{
		if (line < 0)
		{
			return 0;
		}

		if (line >= LineCount())
		{
			return Length();
		}

		return m_LineStarts[static_cast<size_t>(line)];
	}

	Sci_Position SCI_METHOD LineEnd(Sci_Position line) const override
	{
		if (line < 0 || line >= LineCount())
		{
			return Length();
		}

		Sci_Position end = (line + 1 < LineCount()) ? m_LineStarts[static_cast<size_t>(line) + 1] : Length();
		if (end > 0 && m_Text[static_cast<size_t>(end) - 1] == '\n')
		{
			--end;
		}
		if (end > 0 && m_Text[static_cast<size_t>(end) - 1] == '\r')
		{
			--end;
		}

		return end;
	}

	int SCI_METHOD GetLevel(Sci_Position line) const override
	{
		if (line < 0 || line >= LineCount())
		{
			return SC_FOLDLEVELBASE;
		}

		return m_Levels[static_cast<size_t>(line)];
	}

	int SCI_METHOD SetLevel(Sci_Position line, int level) override
	{
		if (line < 0 || line >= LineCount())
		{
			// Fold walks one line past the end of the range it was given; record that it
			// happened so the runner can report it rather than silently accepting it.
			++m_LevelsOutOfRange;
			return SC_FOLDLEVELBASE;
		}

		const int previous = m_Levels[static_cast<size_t>(line)];
		m_Levels[static_cast<size_t>(line)] = level;
		return previous;
	}

	int SCI_METHOD GetLineState(Sci_Position line) const override
	{
		if (line < 0 || line >= LineCount())
		{
			return 0;
		}

		return m_LineStates[static_cast<size_t>(line)];
	}

	int SCI_METHOD SetLineState(Sci_Position line, int state) override
	{
		if (line < 0 || line >= LineCount())
		{
			return 0;
		}

		const int previous = m_LineStates[static_cast<size_t>(line)];
		m_LineStates[static_cast<size_t>(line)] = state;
		return previous;
	}

	void SCI_METHOD StartStyling(Sci_Position position) override
	{
		m_EndStyled = position;
	}

	bool SCI_METHOD SetStyleFor(Sci_Position length, char style) override
	{
		for (Sci_Position i = 0; i < length; ++i, ++m_EndStyled)
		{
			if (m_EndStyled < 0 || m_EndStyled >= Length())
			{
				// Same tolerance as Scintilla, but counted so the runner can report it.
				++m_StylesOutOfRange;
				continue;
			}

			m_Styles[static_cast<size_t>(m_EndStyled)] = style;
		}

		return true;
	}

	bool SCI_METHOD SetStyles(Sci_Position length, const char* styles) override
	{
		for (Sci_Position i = 0; i < length; ++i, ++m_EndStyled)
		{
			if (m_EndStyled < 0 || m_EndStyled >= Length())
			{
				++m_StylesOutOfRange;
				continue;
			}

			m_Styles[static_cast<size_t>(m_EndStyled)] = styles[i];
		}

		return true;
	}

	void SCI_METHOD DecorationSetCurrentIndicator(int) override {}
	void SCI_METHOD DecorationFillRange(Sci_Position, int, Sci_Position) override {}
	void SCI_METHOD ChangeLexerState(Sci_Position, Sci_Position) override {}

	int SCI_METHOD CodePage() const override { return 0; }
	bool SCI_METHOD IsDBCSLeadByte(char) const override { return false; }
	const char* SCI_METHOD BufferPointer() override { return m_Text.c_str(); }
	int SCI_METHOD GetLineIndentation(Sci_Position) override { return 0; }

	Sci_Position SCI_METHOD GetRelativePosition(Sci_Position positionStart, Sci_Position characterOffset) const override
	{
		return std::clamp<Sci_Position>(positionStart + characterOffset, 0, Length());
	}

	int SCI_METHOD GetCharacterAndWidth(Sci_Position position, Sci_Position* pWidth) const override
	{
		if (pWidth != nullptr)
		{
			*pWidth = 1;
		}

		if (position < 0 || position >= Length())
		{
			return 0;
		}

		return static_cast<unsigned char>(m_Text[static_cast<size_t>(position)]);
	}

	// Counts of accesses Scintilla would have discarded. A lexer that stays inside the
	// document leaves both at zero.
	int StylesOutOfRange() const { return m_StylesOutOfRange; }
	int LevelsOutOfRange() const { return m_LevelsOutOfRange; }

private:
	std::string m_Text;
	std::vector<char> m_Styles;
	std::vector<Sci_Position> m_LineStarts;
	std::vector<int> m_Levels;
	std::vector<int> m_LineStates;
	Sci_Position m_EndStyled = 0;
	int m_ErrorStatus = 0;
	int m_StylesOutOfRange = 0;
	int m_LevelsOutOfRange = 0;
};
