// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

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
