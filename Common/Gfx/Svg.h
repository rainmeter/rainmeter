/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once

#include <d2d1_3.h>
#include <string>
#include <string_view>
#include <wrl/client.h>

namespace Gfx {

class Canvas;

class Svg
{
public:
	Svg(const std::wstring& source);
	~Svg();

	Svg(const Svg& other) = delete;
	Svg& operator=(Svg other) = delete;

	FLOAT GetWidth() const { return m_Size.width; }
	FLOAT GetHeight() const { return m_Size.height; }
	const std::wstring& GetSource() const { return m_Source; }
	static bool IsInlineData(std::wstring_view source) { return source.starts_with(L"<svg"); }

	HRESULT Load(const Canvas& canvas);
	bool HasDeviceResources() const { return m_Document != nullptr; }
	void InvalidateDeviceResources() { m_Document.Reset(); }

private:
	friend class Canvas;

	D2D1_SIZE_F GetIntrinsicSize() const;

	std::wstring m_Source;
	bool m_InlineData;
	D2D1_SIZE_F m_Size;
	Microsoft::WRL::ComPtr<ID2D1SvgDocument> m_Document;
};

}  // namespace Gfx
