// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "Svg.h"
#include "Canvas.h"
#include "StringUtil.h"

#include <Shlwapi.h>

namespace {

// Applies |attribute| to |element| and all of its descendants. Failures are ignored since an
// attribute is rarely valid for every element in the document.
void SetAttributeRecursive(ID2D1SvgElement* element, const WCHAR* attribute, const WCHAR* value)
{
	if (!element->IsTextContent())
	{
		element->SetAttributeValue(attribute, D2D1_SVG_ATTRIBUTE_STRING_TYPE_SVG, value);
	}

	Microsoft::WRL::ComPtr<ID2D1SvgElement> child;
	element->GetFirstChild(child.GetAddressOf());
	while (child)
	{
		SetAttributeRecursive(child.Get(), attribute, value);

		Microsoft::WRL::ComPtr<ID2D1SvgElement> next;
		if (FAILED(element->GetNextChild(child.Get(), next.GetAddressOf()))) break;
		child = std::move(next);
	}
}

}  // namespace

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
		// The stream takes a copy of the data, so the conversion only needs to outlive this scope.
		// Inline SVG data is almost always well under 32K, which keeps it off the heap.
		StringBuffer<char, 32 * 1024> data;
		StringUtil::NarrowUTF8(m_Source.c_str(), (int)m_Source.length(), data);
		if (data.empty()) return E_INVALIDARG;

		stream.Attach(SHCreateMemStream(
			reinterpret_cast<const BYTE*>(data.data()), static_cast<UINT>(data.length())));
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

HRESULT Svg::SetAttribute(const std::wstring& selector, const std::wstring& attribute, const std::wstring& value)
{
	if (!m_Document || selector.empty() || attribute.empty()) return E_INVALIDARG;

	Microsoft::WRL::ComPtr<ID2D1SvgElement> root;
	m_Document->GetRoot(root.GetAddressOf());
	if (!root) return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);

	if (selector == L"*")
	{
		SetAttributeRecursive(root.Get(), attribute.c_str(), value.c_str());
	}
	else
	{
		HRESULT hr = S_OK;
		Microsoft::WRL::ComPtr<ID2D1SvgElement> element;
		if (_wcsicmp(selector.c_str(), L"svg") == 0)
		{
			element = root;
		}
		else
		{
			hr = m_Document->FindElementById(selector.c_str(), element.GetAddressOf());
		}
		if (FAILED(hr)) return hr;
		if (!element) return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);

		hr = element->SetAttributeValue(attribute.c_str(), D2D1_SVG_ATTRIBUTE_STRING_TYPE_SVG, value.c_str());
		if (FAILED(hr)) return hr;
	}

	// Root width, height, and viewBox overrides can change the meter's intrinsic size.
	m_Size = GetIntrinsicSize();
	return m_Document->SetViewportSize(m_Size);
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
