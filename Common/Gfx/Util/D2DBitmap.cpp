#include "StdAfx.h"
#include "BitmapD2D.h"

namespace Gfx
{
BitmapSegment::BitmapSegment(Microsoft::WRL::ComPtr<ID2D1Bitmap1>& bitmap, UINT x, UINT y, UINT width, UINT height) 
	: m_Bitmap(bitmap), 
	m_X(x), 
	m_Y(y), 
	m_Width(width), 
	m_Height(height)
{}

BitmapHandle::BitmapHandle(const std::vector<BitmapSegment>& segments, UINT width, UINT height)
	: m_Segments(segments),
	m_Width(width),
	m_Height(height)
{
}

} // namespace Gfx