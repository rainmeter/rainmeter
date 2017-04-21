/* Copyright (C) 2010 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "../Common/PathUtil.h"
#include "TintedImage.h"
#include "ConfigParser.h"
#include "System.h"
#include "Logger.h"

using namespace Gdiplus;


class ImageCachePool
{
public:
	static std::wstring CreateKey(const std::wstring& name, ULONGLONG time, DWORD size, WCHAR* exifOrientation)
	{
		std::wstring key;

		WCHAR buffer[MAX_PATH];
		if (PathCanonicalize(buffer, name.c_str()))
		{
			key = buffer;
		}
		else
		{
			key = name;
		}
		_wcsupr(&key[0]);

		size_t len = _snwprintf_s(buffer, _TRUNCATE, L":%llx:%x:%s", time, size, exifOrientation);
		key.append(buffer, len);

		return key;
	}

	static Bitmap* GetCache(const std::wstring& key)
	{
		std::unordered_map<std::wstring, ImageCache*>::const_iterator iter = c_CacheMap.find(key);
		if (iter != c_CacheMap.end())
		{
			return (*iter).second->GetCache();
		}
		return nullptr;
	}

	static void AddCache(const std::wstring& key, Bitmap* bitmap, HGLOBAL hBuffer)
	{
		std::unordered_map<std::wstring, ImageCache*>::const_iterator iter = c_CacheMap.find(key);
		if (iter != c_CacheMap.end())
		{
			(*iter).second->AddRef();
			//LogDebugF(L"* ADD: key=%s, ref=%i", key.c_str(), (*iter).second->GetRef());
		}
		else
		{
			c_CacheMap[key] = new ImageCache(bitmap, hBuffer);
			//LogDebugF(L"* ADD: key=%s, ref=new", key.c_str());
		}
	}

	static void RemoveCache(const std::wstring& key)
	{
		std::unordered_map<std::wstring, ImageCache*>::const_iterator iter = c_CacheMap.find(key);
		if (iter != c_CacheMap.end())
		{
			ImageCache* cache = (*iter).second;
			cache->Release();
			//LogDebugF(L"* REMOVE: key=%s, ref=%i", key.c_str(), cache->GetRef());

			if (cache->IsInvalid())
			{
				//LogDebugF(L"* EMPTY-ERASE: key=%s", key.c_str());
				c_CacheMap.erase(iter);
				delete cache;
			}
		}
	}

private:
	class ImageCache
	{
	public:
		ImageCache(Bitmap* bitmap, HGLOBAL hBuffer) : m_Bitmap(bitmap), m_hBuffer(hBuffer), m_Ref(1) {}
		~ImageCache() { Dispose(); }

		void AddRef() { ++m_Ref; }
		void Release() { if (m_Ref > 0) { --m_Ref; } if (m_Ref == 0) { Dispose(); } }

		bool IsInvalid() { return m_Ref == 0; }
		//int GetRef() { return m_Ref; }
		Bitmap* GetCache() { return m_Bitmap; }

	private:
		ImageCache() {}
		ImageCache(const ImageCache& cache) {}

		void Dispose() { delete m_Bitmap; m_Bitmap = nullptr; if (m_hBuffer) { ::GlobalFree(m_hBuffer); m_hBuffer = nullptr; } }

		Bitmap* m_Bitmap;
		HGLOBAL m_hBuffer;
		int m_Ref;
	};

	static std::unordered_map<std::wstring, ImageCache*> c_CacheMap;
};
std::unordered_map<std::wstring, ImageCachePool::ImageCache*> ImageCachePool::c_CacheMap;


#define PI	(3.14159265f)
#define CONVERT_TO_RADIANS(X)	((X) * (PI / 180.0f))

// GrayScale Matrix
const Gdiplus::ColorMatrix TintedImage::c_GreyScaleMatrix = {
	0.299f, 0.299f, 0.299f, 0.0f, 0.0f,
	0.587f, 0.587f, 0.587f, 0.0f, 0.0f,
	0.114f, 0.114f, 0.114f, 0.0f, 0.0f,
	  0.0f,   0.0f,   0.0f, 1.0f, 0.0f,
	  0.0f,   0.0f,   0.0f, 0.0f, 1.0f
};

const Gdiplus::ColorMatrix TintedImage::c_IdentityMatrix = {
	1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 0.0f, 1.0f
};

TintedImageHelper_DefineOptionArray(TintedImage::c_DefaultOptionArray, L"");

/*
** If disableTransform is true, ImageCrop and ImageRotate are ignored.
**
*/
TintedImage::TintedImage(const WCHAR* name, const WCHAR** optionArray, bool disableTransform, Skin* skin) : m_DisableTransform(disableTransform),
	m_Name(name ? name : L"ImageName"),
	m_OptionArray(optionArray ? optionArray : c_DefaultOptionArray),
	m_Bitmap(),
	m_BitmapTint(),
	m_NeedsCrop(false),
	m_NeedsTinting(false),
	m_NeedsTransform(false),
	m_Crop(-1, -1, -1, -1),
	m_CropMode(CROPMODE_TL),
	m_GreyScale(false),
	m_ColorMatrix(new ColorMatrix(c_IdentityMatrix)),
	m_Flip(RotateNoneFlipNone),
	m_Rotate(),
	m_UseExifOrientation(false),
	m_Skin(skin),
	m_HasPathChanged(false)
{
}

TintedImage::~TintedImage()
{
	DisposeImage();

	delete m_ColorMatrix;
}

/*
** Disposes the image buffers.
**
*/
void TintedImage::DisposeImage()
{
	delete m_BitmapTint;
	m_BitmapTint = nullptr;

	m_Bitmap = nullptr;

	if (!m_CacheKey.empty())
	{
		ImageCachePool::RemoveCache(m_CacheKey);
		m_CacheKey.clear();
	}
}

/*
** Loads the image from file handle
**
*/
Bitmap* TintedImage::LoadImageFromFileHandle(HANDLE fileHandle, DWORD fileSize, HGLOBAL* phBuffer)
{
	HGLOBAL hBuffer = ::GlobalAlloc(GMEM_MOVEABLE, fileSize);
	if (hBuffer)
	{
		void* pBuffer = ::GlobalLock(hBuffer);
		if (pBuffer)
		{
			DWORD readBytes;
			ReadFile(fileHandle, pBuffer, fileSize, &readBytes, nullptr);
			::GlobalUnlock(hBuffer);

			IStream* pStream = nullptr;
			if (::CreateStreamOnHGlobal(hBuffer, FALSE, &pStream) == S_OK)
			{
				Bitmap* bitmap = Bitmap::FromStream(pStream);
				pStream->Release();

				if (Ok == bitmap->GetLastStatus())
				{
					GUID guid;
					if (Ok == bitmap->GetRawFormat(&guid) && guid != ImageFormatIcon)
					{
						// Gather EXIF orientation information
						if (m_UseExifOrientation)
						{
							UINT size = bitmap->GetPropertyItemSize(PropertyTagOrientation);
							if (size)
							{
								RotateFlipType flip = RotateNoneFlipNone;
								PropertyItem* orientation = (PropertyItem*)new BYTE[size];
								bitmap->GetPropertyItem(PropertyTagOrientation, size, orientation);
								if (orientation)
								{
									switch(*(short*)orientation->value)
									{
									case 8:
										flip = Rotate270FlipNone;
										break;

									case 7:
										flip = Rotate270FlipX;
										break;

									case 6:
										flip = Rotate90FlipNone;
										break;

									case 5:
										flip = Rotate90FlipX;
										break;

									case 4:
										flip = Rotate180FlipX;
										break;

									case 3:
										flip = Rotate180FlipNone;
										break;

									case 2:
										flip = RotateNoneFlipX;
										break;

									default:
										flip = RotateNoneFlipNone;
									}

									bitmap->RotateFlip(flip);
								}
								delete [] orientation;
							}
						}

						////////////////////////////////////////////
						// Convert loaded image to faster blittable bitmap (may increase memory usage slightly)
						Rect r(0, 0, bitmap->GetWidth(), bitmap->GetHeight());
						Bitmap* clone = new Bitmap(r.Width, r.Height, PixelFormat32bppPARGB);
						{
							Graphics graphics(clone);
							graphics.DrawImage(bitmap, r, 0, 0, r.Width, r.Height, UnitPixel);
						}
						delete bitmap;
						bitmap = clone;

						::GlobalFree(hBuffer);
						hBuffer = nullptr;
						////////////////////////////////////////////
					}
					*phBuffer = hBuffer;
					return bitmap;
				}

				delete bitmap;
			}
		}

		::GlobalFree(hBuffer);
	}

	*phBuffer = nullptr;
	return nullptr;
}

/*
** Loads the image from disk
**
*/
void TintedImage::LoadImage(const std::wstring& imageName, bool bLoadAlways)
{
	// Load the bitmap if defined
	if (!imageName.empty())
	{
		std::wstring filename = m_Path + imageName;
		if (m_Skin) m_Skin->MakePathAbsolute(filename);
		m_HasPathChanged = false;

		// Check extension and if it is missing, add .png
		size_t pos = filename.rfind(L'\\');
		if (filename.find(L'.', (pos == std::wstring::npos) ? 0 : pos + 1) == std::wstring::npos)
		{
			filename += L".png";
		}

		// Read the bitmap to memory so that it's not locked by GDI+
		DWORD fileSize;
		HANDLE fileHandle = CreateFile(filename.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
		if (fileHandle != INVALID_HANDLE_VALUE && (fileSize = GetFileSize(fileHandle, nullptr)) != INVALID_FILE_SIZE)
		{
			// Compare the filename/timestamp/filesize to check if the file has been changed (don't load if it's not)
			ULONGLONG fileTime;
			GetFileTime(fileHandle, nullptr, nullptr, (LPFILETIME)&fileTime);
			std::wstring key = ImageCachePool::CreateKey(filename, fileTime, fileSize, m_UseExifOrientation ? L"EXIF" : L"NONE");

			if (bLoadAlways || wcscmp(key.c_str(), m_CacheKey.c_str()) != 0)
			{
				DisposeImage();

				Bitmap* bitmap = ImageCachePool::GetCache(key);
				HGLOBAL hBuffer = nullptr;

				m_Bitmap = (bitmap) ?
					bitmap :
					LoadImageFromFileHandle(fileHandle, fileSize, &hBuffer);

				if (m_Bitmap)
				{
					m_CacheKey = key;
					ImageCachePool::AddCache(key, m_Bitmap, hBuffer);

					// Check whether the new image needs tinting (or cropping, flipping, rotating)
					if (!m_NeedsCrop)
					{
						if (m_Crop.Width >= 0 || m_Crop.Height >= 0)
						{
							m_NeedsCrop = true;
						}
					}
					if (!m_NeedsTinting)
					{
						if (m_GreyScale || !CompareColorMatrix(m_ColorMatrix, &c_IdentityMatrix))
						{
							m_NeedsTinting = true;
						}
					}
					if (!m_NeedsTransform)
					{
						if (m_Flip != RotateNoneFlipNone || m_Rotate != 0.0f)
						{
							m_NeedsTransform = true;
						}
					}
				}
				else
				{
					LogErrorF(m_Skin, L"%s: Unable to load: %s", m_Name, filename.c_str());
				}
			}
			CloseHandle(fileHandle);

			if (m_Bitmap)
			{
				// We need a copy of the image if has tinting (or flipping, rotating)
				if (m_NeedsCrop || m_NeedsTinting || m_NeedsTransform)
				{
					delete m_BitmapTint;
					m_BitmapTint = nullptr;

					if (m_Bitmap->GetWidth() > 0 && m_Bitmap->GetHeight() > 0)
					{
						ApplyCrop();

						if (!m_BitmapTint || (m_BitmapTint->GetWidth() > 0 && m_BitmapTint->GetHeight() > 0))
						{
							ApplyTint();
							ApplyTransform();
						}
					}

					m_NeedsCrop = false;
					m_NeedsTinting = false;
					m_NeedsTransform = false;
				}
			}
		}
		else
		{
			LogErrorF(m_Skin, L"%s: Unable to open: %s", m_Name, filename.c_str());

			if (fileHandle != INVALID_HANDLE_VALUE)
			{
				CloseHandle(fileHandle);
			}
			DisposeImage();
		}
	}
	else if (IsLoaded())
	{
		DisposeImage();
	}
}

/*
** This will apply the cropping.
**
*/
void TintedImage::ApplyCrop()
{
	if (m_Crop.Width >= 0 && m_Crop.Height >= 0)
	{
		if (m_Crop.Width == 0 || m_Crop.Height == 0)
		{
			m_BitmapTint = new Bitmap(0, 0, PixelFormat32bppPARGB);  // create dummy bitmap
		}
		else
		{
			int imageW = m_Bitmap->GetWidth();
			int imageH = m_Bitmap->GetHeight();

			int x, y;

			switch (m_CropMode)
			{
			case CROPMODE_TL:
			default:
				x = m_Crop.X;
				y = m_Crop.Y;
				break;

			case CROPMODE_TR:
				x = m_Crop.X + imageW;
				y = m_Crop.Y;
				break;

			case CROPMODE_BR:
				x = m_Crop.X + imageW;
				y = m_Crop.Y + imageH;
				break;

			case CROPMODE_BL:
				x = m_Crop.X;
				y = m_Crop.Y + imageH;
				break;

			case CROPMODE_C:
				x = m_Crop.X + (imageW / 2);
				y = m_Crop.Y + (imageH / 2);
				break;
			}

			Rect r(0, 0, m_Crop.Width, m_Crop.Height);
			m_BitmapTint = new Bitmap(r.Width, r.Height, PixelFormat32bppPARGB);

			Graphics graphics(m_BitmapTint);
			graphics.DrawImage(m_Bitmap, r, x, y, r.Width, r.Height, UnitPixel);
		}
	}
}

/*
** This will apply the Greyscale matrix and the color tinting.
**
*/
void TintedImage::ApplyTint()
{
	bool useColorMatrix = !CompareColorMatrix(m_ColorMatrix, &c_IdentityMatrix);

	if (m_GreyScale || useColorMatrix)
	{
		Bitmap* original = GetImage();
		Bitmap* tint;

		if (m_GreyScale && !useColorMatrix)
		{
			tint = TurnGreyscale(original);
		}
		else
		{
			ImageAttributes ImgAttr;
			ImgAttr.SetColorMatrix(m_ColorMatrix, ColorMatrixFlagsDefault, ColorAdjustTypeBitmap);

			Rect r(0, 0, original->GetWidth(), original->GetHeight());

			tint = new Bitmap(r.Width, r.Height, PixelFormat32bppPARGB);

			Graphics graphics(tint);

			if (m_GreyScale)
			{
				Bitmap* gray = TurnGreyscale(original);
				graphics.DrawImage(gray, r, 0, 0, r.Width, r.Height, UnitPixel, &ImgAttr);
				delete gray;
			}
			else
			{
				graphics.DrawImage(original, r, 0, 0, r.Width, r.Height, UnitPixel, &ImgAttr);
			}
		}

		delete m_BitmapTint;
		m_BitmapTint = tint;
	}
}

/*
** Turns the image greyscale by applying a greyscale color matrix.
** Note that the returned bitmap image must be freed by caller.
**
*/
Bitmap* TintedImage::TurnGreyscale(Bitmap* source)
{
	ImageAttributes ImgAttr;
	ImgAttr.SetColorMatrix(&c_GreyScaleMatrix, ColorMatrixFlagsDefault, ColorAdjustTypeBitmap);

	// We need a blank bitmap to paint our greyscale to in case of alpha
	Rect r(0, 0, source->GetWidth(), source->GetHeight());
	Bitmap* bitmap = new Bitmap(r.Width, r.Height, PixelFormat32bppPARGB);

	Graphics graphics(bitmap);
	graphics.DrawImage(source, r, 0, 0, r.Width, r.Height, UnitPixel, &ImgAttr);

	return bitmap;
}

/*
** This will apply the flipping and rotating.
**
*/
void TintedImage::ApplyTransform()
{
	if (m_Rotate != 0.0f)
	{
		Bitmap* original = GetImage();

		REAL originalW = (REAL)original->GetWidth();
		REAL originalH = (REAL)original->GetHeight();

		REAL cos_f = cos(CONVERT_TO_RADIANS(m_Rotate)), sin_f = sin(CONVERT_TO_RADIANS(m_Rotate));

		REAL transformW = fabs(originalW * cos_f) + fabs(originalH * sin_f);
		REAL transformH = fabs(originalW * sin_f) + fabs(originalH * cos_f);

		Bitmap* transform = new Bitmap((int)(transformW + 0.5f), (int)(transformH + 0.5f), PixelFormat32bppPARGB);

		Graphics graphics(transform);
		graphics.SetPixelOffsetMode(PixelOffsetModeHighQuality);

		REAL cx = transformW / 2.0f;
		REAL cy = transformH / 2.0f;

		Matrix rotateMatrix;
		rotateMatrix.RotateAt(m_Rotate, PointF(cx, cy));

		graphics.SetTransform(&rotateMatrix);

		if (m_Flip != RotateNoneFlipNone)
		{
			original->RotateFlip(m_Flip);
		}

		RectF r(cx - originalW / 2.0f, cy - originalH / 2.0f, originalW, originalH);
		graphics.DrawImage(original, r, -0.5f, -0.5f, originalW + 1.0f, originalH + 1.0f, UnitPixel);  // Makes the anti-aliased edge

		if (m_Flip != RotateNoneFlipNone)
		{
			original->RotateFlip(m_Flip);
		}

		delete m_BitmapTint;
		m_BitmapTint = transform;
	}
	else if (m_Flip != RotateNoneFlipNone)
	{
		Bitmap* original = GetImage();

		Rect r(0, 0, original->GetWidth(), original->GetHeight());
		Bitmap* transform = new Bitmap(r.Width, r.Height, PixelFormat32bppPARGB);

		Graphics graphics(transform);

		original->RotateFlip(m_Flip);

		graphics.DrawImage(original, r, 0, 0, r.Width, r.Height, UnitPixel);

		original->RotateFlip(m_Flip);

		delete m_BitmapTint;
		m_BitmapTint = transform;
	}
}

/*
** Read the meter-specific options from the ini-file.
**
*/
void TintedImage::ReadOptions(ConfigParser& parser, const WCHAR* section, const WCHAR* imagePath)
{
	// Store the current values so we know if the image needs to be tinted or transformed
	Rect oldCrop = m_Crop;
	CROPMODE oldCropMode = m_CropMode;
	bool oldGreyScale = m_GreyScale;
	ColorMatrix oldColorMatrix = *m_ColorMatrix;
	RotateFlipType oldFlip = m_Flip;
	REAL oldRotate = m_Rotate;
	std::wstring oldPath = m_Path;

	m_Path = parser.ReadString(section, m_OptionArray[OptionIndexImagePath], imagePath);
	PathUtil::AppendBackslashIfMissing(m_Path);

	m_HasPathChanged = (oldPath != m_Path);

	if (!m_DisableTransform)
	{
		m_Crop.X = m_Crop.Y = m_Crop.Width = m_Crop.Height = -1;
		m_CropMode = CROPMODE_TL;

		const std::wstring& crop = parser.ReadString(section, m_OptionArray[OptionIndexImageCrop], L"");
		if (!crop.empty())
		{
			if (wcschr(crop.c_str(), L','))
			{
				WCHAR* context = nullptr;
				WCHAR* parseSz = _wcsdup(crop.c_str());
				WCHAR* token;

				token = wcstok(parseSz, L",", &context);
				if (token)
				{
					m_Crop.X = parser.ParseInt(token, 0);

					token = wcstok(nullptr, L",", &context);
					if (token)
					{
						m_Crop.Y = parser.ParseInt(token, 0);

						token = wcstok(nullptr, L",", &context);
						if (token)
						{
							m_Crop.Width = parser.ParseInt(token, 0);

							token = wcstok(nullptr, L",", &context);
							if (token)
							{
								m_Crop.Height = parser.ParseInt(token, 0);

								token = wcstok(nullptr, L",", &context);
								if (token)
								{
									m_CropMode = (CROPMODE)parser.ParseInt(token, 0);
								}
							}
						}
					}
				}
				free(parseSz);
			}

			if (m_CropMode < CROPMODE_TL || m_CropMode > CROPMODE_C)
			{
				m_CropMode = CROPMODE_TL;
				LogErrorF(m_Skin, L"%s=%s (origin) is not valid in [%s]",  m_OptionArray[OptionIndexImageCrop], crop, section);
			}
		}
	}

	m_NeedsCrop = (oldCrop.X != m_Crop.X || oldCrop.Y != m_Crop.Y || oldCrop.Width != m_Crop.Width || oldCrop.Height != m_Crop.Height || oldCropMode != m_CropMode);

	m_GreyScale = parser.ReadBool(section, m_OptionArray[OptionIndexGreyscale], false);

	Color tint = parser.ReadColor(section, m_OptionArray[OptionIndexImageTint], Color::White);
	int alpha = parser.ReadInt(section, m_OptionArray[OptionIndexImageAlpha], tint.GetAlpha());  // for backwards compatibility
	alpha = min(255, alpha);
	alpha = max(0, alpha);

	*m_ColorMatrix = c_IdentityMatrix;

	// Read in the Color Matrix
	// It has to be read in like this because it crashes when reading over 17 floats
	// at one time. The parser does it fine, but after putting the returned values
	// into the Color Matrix the next time the parser is used it crashes.
	std::vector<Gdiplus::REAL> matrix1 = parser.ReadFloats(section, m_OptionArray[OptionIndexColorMatrix1]);
	if (matrix1.size() == 5)
	{
		for (int i = 0; i < 4; ++i)  // The fifth column must be 0.
		{
			m_ColorMatrix->m[0][i] = matrix1[i];
		}
	}
	else
	{
		m_ColorMatrix->m[0][0] = (REAL)tint.GetRed() / 255.0f;
	}

	std::vector<Gdiplus::REAL> matrix2 = parser.ReadFloats(section, m_OptionArray[OptionIndexColorMatrix2]);
	if (matrix2.size() == 5)
	{
		for (int i = 0; i < 4; ++i)  // The fifth column must be 0.
		{
			m_ColorMatrix->m[1][i] = matrix2[i];
		}
	}
	else
	{
		m_ColorMatrix->m[1][1] = (REAL)tint.GetGreen() / 255.0f;
	}

	std::vector<Gdiplus::REAL> matrix3 = parser.ReadFloats(section, m_OptionArray[OptionIndexColorMatrix3]);
	if (matrix3.size() == 5)
	{
		for (int i = 0; i < 4; ++i)  // The fifth column must be 0.
		{
			m_ColorMatrix->m[2][i] = matrix3[i];
		}
	}
	else
	{
		m_ColorMatrix->m[2][2] = (REAL)tint.GetBlue() / 255.0f;
	}

	std::vector<Gdiplus::REAL> matrix4 = parser.ReadFloats(section, m_OptionArray[OptionIndexColorMatrix4]);
	if (matrix4.size() == 5)
	{
		for (int i = 0; i < 4; ++i)  // The fifth column must be 0.
		{
			m_ColorMatrix->m[3][i] = matrix4[i];
		}
	}
	else
	{
		m_ColorMatrix->m[3][3] = (REAL)alpha / 255.0f;
	}

	std::vector<Gdiplus::REAL> matrix5 = parser.ReadFloats(section, m_OptionArray[OptionIndexColorMatrix5]);
	if (matrix5.size() == 5)
	{
		for (int i = 0; i < 4; ++i)  // The fifth column must be 1.
		{
			m_ColorMatrix->m[4][i] = matrix5[i];
		}
	}

	m_NeedsTinting = (oldGreyScale != m_GreyScale || !CompareColorMatrix(&oldColorMatrix, m_ColorMatrix));

	m_UseExifOrientation = parser.ReadBool(section, m_OptionArray[OptionIndexUseExifOrientation], false);

	const WCHAR* flip = parser.ReadString(section, m_OptionArray[OptionIndexImageFlip], L"NONE").c_str();
	if (_wcsicmp(flip, L"NONE") == 0)
	{
		m_Flip = RotateNoneFlipNone;
	}
	else if (_wcsicmp(flip, L"HORIZONTAL") == 0)
	{
		m_Flip = RotateNoneFlipX;
	}
	else if (_wcsicmp(flip, L"VERTICAL") == 0)
	{
		m_Flip = RotateNoneFlipY;
	}
	else if (_wcsicmp(flip, L"BOTH") == 0)
	{
		m_Flip = RotateNoneFlipXY;
	}
	else
	{
		LogErrorF(m_Skin, L"%s=%s (origin) is not valid in [%s]",  m_OptionArray[OptionIndexImageFlip], flip, section);
	}

	if (!m_DisableTransform)
	{
		m_Rotate = (REAL)parser.ReadFloat(section, m_OptionArray[OptionIndexImageRotate], 0.0);
	}

	m_NeedsTransform = (oldFlip != m_Flip || oldRotate != m_Rotate);
}

/*
** Compares the two given color matrices.
**
*/
bool TintedImage::CompareColorMatrix(const Gdiplus::ColorMatrix* a, const Gdiplus::ColorMatrix* b)
{
	for (int i = 0; i < 5; ++i)
	{
		for (int j = 0; j < 4; ++j)  // The fifth column is reserved.
		{
			if (a->m[i][j] != b->m[i][j])
			{
				return false;
			}
		}
	}
	return true;
}
