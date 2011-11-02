/*
  Copyright (C) 2010 Kimmo Pekkola, MattKing, spx

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "StdAfx.h"
#include "TintedImage.h"
#include "ConfigParser.h"
#include "System.h"
#include "Error.h"
#include "Litestep.h"

using namespace Gdiplus;

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

	void Dispose() { delete m_Bitmap; m_Bitmap = NULL; if (m_hBuffer) { ::GlobalFree(m_hBuffer); m_hBuffer = NULL; } }

	Bitmap* m_Bitmap;
	HGLOBAL m_hBuffer;
	int m_Ref;
};

class ImageCachePool
{
public:
	static std::wstring CreateKey(const std::wstring& fname, FILETIME ftime, DWORD fileSize)
	{
		WCHAR buffer[MAX_PATH];

		std::wstring key = (PathCanonicalize(buffer, fname.c_str())) ? buffer : fname;

		_snwprintf_s(buffer, _TRUNCATE, L":%x%08x:%x", ftime.dwHighDateTime, ftime.dwLowDateTime, fileSize);
		key += buffer;

		std::transform(key.begin(), key.end(), key.begin(), ::towlower);
		return key;
	}

	static Bitmap* GetCache(const std::wstring& key)
	{
		std::unordered_map<std::wstring, ImageCache*>::const_iterator iter = c_CacheMap.find(key);
		if (iter != c_CacheMap.end())
		{
			return (*iter).second->GetCache();
		}
		return NULL;
	}

	static void AddCache(const std::wstring& key, ImageCache* cache)
	{
		std::unordered_map<std::wstring, ImageCache*>::const_iterator iter = c_CacheMap.find(key);
		if (iter != c_CacheMap.end())
		{
			(*iter).second->AddRef();
			//LogWithArgs(LOG_DEBUG, L"* ADD: key=%s, ref=%i", key.c_str(), (*iter).second->GetRef());
		}
		else if (cache)
		{
			c_CacheMap[key] = cache;
			//LogWithArgs(LOG_DEBUG, L"* ADD: key=%s, ref=%i", key.c_str(), cache->GetRef());
		}
	}

	static void RemoveCache(const std::wstring& key)
	{
		std::unordered_map<std::wstring, ImageCache*>::const_iterator iter = c_CacheMap.find(key);
		if (iter != c_CacheMap.end())
		{
			ImageCache* cache = (*iter).second;
			cache->Release();
			//LogWithArgs(LOG_DEBUG, L"* REMOVE: key=%s, ref=%i", key.c_str(), cache->GetRef());

			if (cache->IsInvalid())
			{
				//LogWithArgs(LOG_DEBUG, L"* EMPTY-ERASE: key=%s", key.c_str());
				c_CacheMap.erase(iter);
				delete cache;
			}
		}
	}

private:
	static std::unordered_map<std::wstring, ImageCache*> c_CacheMap;
};
std::unordered_map<std::wstring, ImageCache*> ImageCachePool::c_CacheMap;


#define PI	(3.14159265f)
#define CONVERT_TO_RADIANS(X)	((X) * (PI / 180.0f))

// GrayScale Matrix
const Gdiplus::ColorMatrix CTintedImage::c_GreyScaleMatrix = {
	0.299f, 0.299f, 0.299f, 0.0f, 0.0f,
	0.587f, 0.587f, 0.587f, 0.0f, 0.0f,
	0.114f, 0.114f, 0.114f, 0.0f, 0.0f,
	  0.0f,   0.0f,   0.0f, 1.0f, 0.0f,
	  0.0f,   0.0f,   0.0f, 0.0f, 1.0f
};

const Gdiplus::ColorMatrix CTintedImage::c_IdentityMatrix = {
	1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 0.0f, 1.0f
};

CTintedImageHelper_DefineConfigArray(CTintedImage::c_DefaultConfigArray, L"");

/*
** CTintedImage
**
** The constructor.
**
** If disableTransform is true, following configs are ignored:
**  - ImageCrop
**  - ImageRotate
**
*/
CTintedImage::CTintedImage(const WCHAR* name, const WCHAR** configArray, bool disableTransform) : m_DisableTransform(disableTransform),
	m_ConfigName(name ? name : L"Image"),
	m_ConfigArray(configArray ? configArray : c_DefaultConfigArray),

	m_Bitmap(),
	m_BitmapTint(),
	m_NeedsCrop(false),
	m_NeedsTinting(false),
	m_NeedsTransform(false),
	m_Crop(-1, -1, -1, -1),
	m_CropMode(CROPMODE_TL),
	m_GreyScale(false),
	m_ColorMatrix(new ColorMatrix),
	m_Flip(RotateNoneFlipNone),
	m_Rotate()
{
	*m_ColorMatrix = c_IdentityMatrix;
}

/*
** ~CTintedImage
**
** The destructor
**
*/
CTintedImage::~CTintedImage()
{
	DisposeImage();

	delete m_ColorMatrix;
}

/*
** DisposeImage
**
** Disposes the image buffers.
**
*/
void CTintedImage::DisposeImage()
{
	delete m_BitmapTint;
	m_BitmapTint = NULL;

	m_Bitmap = NULL;

	if (!m_CacheKey.empty())
	{
		ImageCachePool::RemoveCache(m_CacheKey);
		m_CacheKey.clear();
	}
}

/*
** LoadImageFromFileHandle
**
** Loads the image from file handle
**
*/
Bitmap* CTintedImage::LoadImageFromFileHandle(HANDLE fileHandle, DWORD fileSize, ImageCache** ppCache)
{
	HGLOBAL hBuffer = ::GlobalAlloc(GMEM_MOVEABLE, fileSize);
	if (hBuffer)
	{
		void* pBuffer = ::GlobalLock(hBuffer);
		if (pBuffer)
		{
			DWORD readBytes;
			ReadFile(fileHandle, pBuffer, fileSize, &readBytes, NULL);
			::GlobalUnlock(hBuffer);

			IStream* pStream = NULL;
			if (::CreateStreamOnHGlobal(hBuffer, FALSE, &pStream) == S_OK)
			{
				Bitmap* bitmap = Bitmap::FromStream(pStream);
				pStream->Release();

				if (Ok == bitmap->GetLastStatus())
				{
					////////////////////////////////////////////
					// Convert loaded image to faster blittable bitmap (may increase memory usage slightly)
					{
						Rect r(0, 0, bitmap->GetWidth(), bitmap->GetHeight());
						Bitmap* clone = new Bitmap(r.Width, r.Height, PixelFormat32bppPARGB);
						{
							Graphics graphics(clone);
							graphics.DrawImage(bitmap, r, 0, 0, r.Width, r.Height, UnitPixel);
						}
						delete bitmap;
						bitmap = clone;

						::GlobalFree(hBuffer);
						hBuffer = NULL;
					}
					////////////////////////////////////////////
					*ppCache = new ImageCache(bitmap, hBuffer);
					return bitmap;
				}

				delete bitmap;
			}
		}

		::GlobalFree(hBuffer);
	}

	return NULL;
}

/*
** LoadImage
**
** Loads the image from disk
**
*/
void CTintedImage::LoadImage(const std::wstring& imageName, bool bLoadAlways)
{
	// Load the bitmap if defined
	if (!imageName.empty())
	{
		std::wstring filename = imageName;

		// Check extension and if it is missing, add .png
		size_t pos = filename.find_last_of(L"\\");
		if (pos == std::wstring::npos) pos = 0;
		if (std::wstring::npos == filename.find(L'.', pos))
		{
			filename += L".png";
		}

		// Read the bitmap to memory so that it's not locked by GDI+
		DWORD fileSize;
		HANDLE fileHandle = CreateFile(filename.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
		if (fileHandle != INVALID_HANDLE_VALUE && (fileSize = GetFileSize(fileHandle, NULL)) != INVALID_FILE_SIZE)
		{
			// Compare the filename/timestamp/filesize to check if the file has been changed (don't load if it's not)
			FILETIME tmpTime;
			GetFileTime(fileHandle, NULL, NULL, &tmpTime);
			std::wstring key = ImageCachePool::CreateKey(filename, tmpTime, fileSize);

			if (bLoadAlways || key != m_CacheKey)
			{
				DisposeImage();

				Bitmap* bitmap = ImageCachePool::GetCache(key);
				ImageCache* cache = NULL;

				m_Bitmap = (bitmap) ?
					bitmap :
					LoadImageFromFileHandle(fileHandle, fileSize, &cache);

				if (m_Bitmap)
				{
					m_CacheKey = key;
					ImageCachePool::AddCache(key, cache);

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
					LogWithArgs(LOG_ERROR, L"%s: Unable to load: %s", m_ConfigName.c_str(), filename.c_str());
				}
			}
			CloseHandle(fileHandle);

			if (m_Bitmap)
			{
				// We need a copy of the image if has tinting (or flipping, rotating)
				if (m_NeedsCrop || m_NeedsTinting || m_NeedsTransform)
				{
					delete m_BitmapTint;
					m_BitmapTint = NULL;

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
			LogWithArgs(LOG_ERROR, L"%s: Unable to open: %s", m_ConfigName.c_str(), filename.c_str());
			DisposeImage();
		}
	}
	else if (IsLoaded())
	{
		DisposeImage();
	}
}

/*
** ApplyCrop
**
** This will apply the cropping.
**
*/
void CTintedImage::ApplyCrop()
{
	if (m_Crop.Width >= 0 && m_Crop.Height >= 0)
	{
		if (m_Crop.Width == 0 || m_Crop.Height == 0)
		{
			m_BitmapTint = new Bitmap(0, 0, PixelFormat24bppRGB);  // create dummy bitmap
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
			m_BitmapTint = new Bitmap(r.Width, r.Height,
				AdjustNonAlphaPixelFormat(m_Bitmap, x >= 0 && y >= 0 && (x + m_Crop.Width) <= imageW && (y + m_Crop.Height) <= imageH));

			Graphics graphics(m_BitmapTint);
			graphics.DrawImage(m_Bitmap, r, x, y, r.Width, r.Height, UnitPixel);
		}
	}
}

/*
** ApplyTint
**
** This will apply the Greyscale matrix and the color tinting.
**
*/
void CTintedImage::ApplyTint()
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
** TurnGreyscale
**
** Turns the image greyscale by applying a greyscale color matrix.
** Note that the returned bitmap image must be freed by caller.
**
*/
Bitmap* CTintedImage::TurnGreyscale(Bitmap* source)
{
	ImageAttributes ImgAttr;
	ImgAttr.SetColorMatrix(&c_GreyScaleMatrix, ColorMatrixFlagsDefault, ColorAdjustTypeBitmap);

	// We need a blank bitmap to paint our greyscale to in case of alpha
	Rect r(0, 0, source->GetWidth(), source->GetHeight());
	Bitmap* bitmap = new Bitmap(r.Width, r.Height, AdjustNonAlphaPixelFormat(source));

	Graphics graphics(bitmap);
	graphics.DrawImage(source, r, 0, 0, r.Width, r.Height, UnitPixel, &ImgAttr);

	return bitmap;
}

/*
** ApplyTransform
**
** This will apply the flipping and rotating.
**
*/
void CTintedImage::ApplyTransform()
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
		Bitmap* transform = new Bitmap(r.Width, r.Height, AdjustNonAlphaPixelFormat(original));

		Graphics graphics(transform);

		original->RotateFlip(m_Flip);

		graphics.DrawImage(original, r, 0, 0, r.Width, r.Height, UnitPixel);

		original->RotateFlip(m_Flip);

		delete m_BitmapTint;
		m_BitmapTint = transform;
	}
}

/*
** ReadConfig
**
** Read the meter-specific configs from the ini-file.
**
*/
void CTintedImage::ReadConfig(CConfigParser& parser, const WCHAR* section)
{
	// Store the current values so we know if the image needs to be tinted or transformed
	Rect oldCrop = m_Crop;
	CROPMODE oldCropMode = m_CropMode;
	bool oldGreyScale = m_GreyScale;
	ColorMatrix oldColorMatrix = *m_ColorMatrix;
	RotateFlipType oldFlip = m_Flip;
	REAL oldRotate = m_Rotate;

	if (!m_DisableTransform)
	{
		m_Crop.X = m_Crop.Y = m_Crop.Width = m_Crop.Height = -1;
		m_CropMode = CROPMODE_TL;

		std::wstring crop = parser.ReadString(section, m_ConfigArray[ConfigIndexImageCrop], L"");
		if (!crop.empty())
		{
			if (wcschr(crop.c_str(), L','))
			{
				WCHAR* parseSz = _wcsdup(crop.c_str());
				WCHAR* token;

				token = wcstok(parseSz, L",");
				if (token)
				{
					m_Crop.X = _wtoi(token);
				}
				token = wcstok(NULL, L",");
				if (token)
				{
					m_Crop.Y = _wtoi(token);
				}
				token = wcstok(NULL, L",");
				if (token)
				{
					m_Crop.Width = _wtoi(token);
				}
				token = wcstok(NULL, L",");
				if (token)
				{
					m_Crop.Height = _wtoi(token);
				}
				token = wcstok(NULL, L",");
				if (token)
				{
					m_CropMode = (CROPMODE)_wtoi(token);
				}
				free(parseSz);
			}

			if (m_CropMode < CROPMODE_TL || m_CropMode > CROPMODE_C)
			{
				std::wstring error = m_ConfigArray[ConfigIndexImageCrop];
				error += L"=";
				error += crop;
				error += L" (origin) is not valid in meter [";
				error += section;
				error += L"].";
				throw CError(error, __LINE__, __FILE__);
			}
		}
	}

	m_NeedsCrop = (oldCrop.X != m_Crop.X || oldCrop.Y != m_Crop.Y || oldCrop.Width != m_Crop.Width || oldCrop.Height != m_Crop.Height || oldCropMode != m_CropMode);

	m_GreyScale = 0!=parser.ReadInt(section, m_ConfigArray[ConfigIndexGreyscale], 0);

	Color tint = parser.ReadColor(section, m_ConfigArray[ConfigIndexImageTint], Color::White);
	int alpha = parser.ReadInt(section, m_ConfigArray[ConfigIndexImageAlpha], tint.GetAlpha());  // for backwards compatibility
	alpha = min(255, alpha);
	alpha = max(0, alpha);

	*m_ColorMatrix = c_IdentityMatrix;

	// Read in the Color Matrix
	// It has to be read in like this because it crashes when reading over 17 floats
	// at one time. The parser does it fine, but after putting the returned values
	// into the Color Matrix the next time the parser is used it crashes.
	std::vector<Gdiplus::REAL> matrix1 = parser.ReadFloats(section, m_ConfigArray[ConfigIndexColorMatrix1]);
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

	std::vector<Gdiplus::REAL> matrix2 = parser.ReadFloats(section, m_ConfigArray[ConfigIndexColorMatrix2]);
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

	std::vector<Gdiplus::REAL> matrix3 = parser.ReadFloats(section, m_ConfigArray[ConfigIndexColorMatrix3]);
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

	std::vector<Gdiplus::REAL> matrix4 = parser.ReadFloats(section, m_ConfigArray[ConfigIndexColorMatrix4]);
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

	std::vector<Gdiplus::REAL> matrix5 = parser.ReadFloats(section, m_ConfigArray[ConfigIndexColorMatrix5]);
	if (matrix5.size() == 5)
	{
		for (int i = 0; i < 4; ++i)  // The fifth column must be 1.
		{
			m_ColorMatrix->m[4][i] = matrix5[i];
		}
	}

	m_NeedsTinting = (oldGreyScale != m_GreyScale || !CompareColorMatrix(&oldColorMatrix, m_ColorMatrix));

	std::wstring flip = parser.ReadString(section, m_ConfigArray[ConfigIndexImageFlip], L"NONE");
	if (_wcsicmp(flip.c_str(), L"NONE") == 0)
	{
		m_Flip = RotateNoneFlipNone;
	}
	else if (_wcsicmp(flip.c_str(), L"HORIZONTAL") == 0)
	{
		m_Flip = RotateNoneFlipX;
	}
	else if (_wcsicmp(flip.c_str(), L"VERTICAL") == 0)
	{
		m_Flip = RotateNoneFlipY;
	}
	else if (_wcsicmp(flip.c_str(), L"BOTH") == 0)
	{
		m_Flip = RotateNoneFlipXY;
	}
	else
	{
		std::wstring error = m_ConfigArray[ConfigIndexImageFlip];
		error += L"=";
		error += flip;
		error += L" is not valid in meter [";
		error += section;
		error += L"].";
		throw CError(error, __LINE__, __FILE__);
	}

	if (!m_DisableTransform)
	{
		m_Rotate = (REAL)parser.ReadFloat(section, m_ConfigArray[ConfigIndexImageRotate], 0.0);
	}

	m_NeedsTransform = (oldFlip != m_Flip || oldRotate != m_Rotate);
}

/*
** CompareColorMatrix
**
** Compares the two given color matrices.
**
*/
bool CTintedImage::CompareColorMatrix(const Gdiplus::ColorMatrix* a, const Gdiplus::ColorMatrix* b)
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
