/* Copyright (C) 2015 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_COMMON_CHARACTERENTITYREFERENCE_H_
#define RM_COMMON_CHARACTERENTITYREFERENCE_H_

#include <string>

namespace CharacterEntityReference {

void Decode(std::wstring& str, int opt, bool unescape);

} // namespace CharacterEntityReference

#endif
