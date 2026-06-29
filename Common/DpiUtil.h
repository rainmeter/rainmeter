/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_COMMON_DPIUTIL_H_
#define RM_COMMON_DPIUTIL_H_

#include <Windows.h>

namespace DpiUtil {

class DpiUnawareScope
{
public:
	DpiUnawareScope() : m_PreviousContext(SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_UNAWARE)) {}
	DpiUnawareScope(const DpiUnawareScope&) = delete;
	DpiUnawareScope& operator=(const DpiUnawareScope&) = delete;

	~DpiUnawareScope()
	{
		SetThreadDpiAwarenessContext(m_PreviousContext);
	}

private:
	DPI_AWARENESS_CONTEXT m_PreviousContext;
};

}  // namespace DpiUtil

#endif
