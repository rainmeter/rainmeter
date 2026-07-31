/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "Svg.h"
#include "Canvas.h"
#include "StringUtil.h"

#include <Shlwapi.h>

namespace Gfx {

Svg::Svg(const std::wstring& source) :
	m_Source(source),
	m_InlineData(IsInlineData(source)),
	m_Size(D2D1::SizeF(0.0f, 0.0f))
{
}

Svg::~Svg()
{
}

HRESULT Svg::Load(const Canvas& canvas)
{
	m_Document.Reset();
	m_Size = D2D1::SizeF(0.0f, 0.0f);

	Microsoft::WRL::ComPtr<IStream> stream;
	if (m_InlineData)
	{
		const std::string data = StringUtil::NarrowUTF8(m_Source);
		if (data.empty()) return E_INVALIDARG;

		stream.Attach(SHCreateMemStream(
			reinterpret_cast<const BYTE*>(data.data()), static_cast<UINT>(data.size())));
		if (!stream) return E_OUTOFMEMORY;
	}
	else
	{
		const HRESULT hr = SHCreateStreamOnFileEx(m_Source.c_str(), STGM_READ | STGM_SHARE_DENY_WRITE, FILE_ATTRIBUTE_NORMAL, FALSE, nullptr, stream.GetAddressOf());
		if (FAILED(hr)) return hr;
	}

	const D2D1_SIZE_F defaultSize = D2D1::SizeF(300.0f, 150.0f);
	HRESULT hr = canvas.m_Target->CreateSvgDocument(stream.Get(), defaultSize, m_Document.GetAddressOf());
	if (FAILED(hr)) return hr;

	m_Size = GetIntrinsicSize();
	hr = m_Document->SetViewportSize(m_Size);
	if (FAILED(hr))
	{
		m_Document.Reset();
		m_Size = D2D1::SizeF(0.0f, 0.0f);
	}
	return hr;
}

D2D1_SIZE_F Svg::GetIntrinsicSize() const
{
	D2D1_SIZE_F size = D2D1::SizeF(300.0f, 150.0f);
	Microsoft::WRL::ComPtr<ID2D1SvgElement> root;
	m_Document->GetRoot(root.GetAddressOf());
	if (!root) return size;

	D2D1_SVG_LENGTH width = {};
	D2D1_SVG_LENGTH height = {};
	const bool hasWidth = SUCCEEDED(root->GetAttributeValue(L"width", D2D1_SVG_ATTRIBUTE_POD_TYPE_LENGTH, &width, sizeof(width))) &&
		width.units == D2D1_SVG_LENGTH_UNITS_NUMBER && width.value > 0.0f;
	const bool hasHeight = SUCCEEDED(root->GetAttributeValue(L"height", D2D1_SVG_ATTRIBUTE_POD_TYPE_LENGTH, &height, sizeof(height))) &&
		height.units == D2D1_SVG_LENGTH_UNITS_NUMBER && height.value > 0.0f;
	if (hasWidth) size.width = width.value;
	if (hasHeight) size.height = height.value;

	D2D1_SVG_VIEWBOX viewBox = {};
	const bool hasViewBox = SUCCEEDED(root->GetAttributeValue(L"viewBox", D2D1_SVG_ATTRIBUTE_POD_TYPE_VIEWBOX, &viewBox, sizeof(viewBox))) &&
		viewBox.width > 0.0f && viewBox.height > 0.0f;
	if (hasViewBox)
	{
		if (!hasWidth && !hasHeight)
		{
			size = D2D1::SizeF(viewBox.width, viewBox.height);
		}
		else if (hasWidth && !hasHeight)
		{
			size.height = size.width * viewBox.height / viewBox.width;
		}
		else if (!hasWidth && hasHeight)
		{
			size.width = size.height * viewBox.width / viewBox.height;
		}
	}

	return size;
}

}  // namespace Gfx
