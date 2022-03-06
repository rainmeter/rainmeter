/* Copyright (C) 2011 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __LYRICS_H__
#define __LYRICS_H__

class Lyrics
{
public:
	static bool GetFromInternet(const std::wstring& artist, const std::wstring& title, std::wstring& out);

private:
	static bool GetFromLetras(const std::wstring& artist, const std::wstring& title, std::wstring& data);
};

#endif
