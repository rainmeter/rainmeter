#pragma once

namespace Gfx
{

struct BitmapSegment
{
	BitmapSegment(Microsoft::WRL::ComPtr<ID2D1Bitmap1>& bitmap, UINT x, UINT y, UINT width, UINT height);

	Microsoft::WRL::ComPtr<ID2D1Bitmap1> m_Bitmap;
	UINT m_X;
	UINT m_Y;
	UINT m_Width;
	UINT m_Height;
};

class BitmapHandle
{
public:
	BitmapHandle(const std::vector<BitmapSegment>& segments, UINT width, UINT height);

private:
	friend class BitmapLoader;
	std::vector<BitmapSegment> m_Segments;
	UINT m_Width;
	UINT m_Height;
};

class BitmapD2D
{
public:
	BitmapD2D(const std::wstring& path, const BitmapHandle& bitmap)
		: m_Path(path),
		  m_Bitmap(bitmap)
	{
	}

private:
	friend class BitmapLoader;
	std::wstring m_Path;
	BitmapHandle m_Bitmap;
};

} // namespace Gfx