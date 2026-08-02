// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

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
