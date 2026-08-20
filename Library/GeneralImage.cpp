// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "GeneralImage.h"
#include "Logger.h"
#include "System.h"
#include "../Common/PathUtil.h"
#include "../Common/StringParser.h"
#include <commoncontrols.h>

namespace {

struct SystemImage
{
	int index;
	int list;
};

std::optional<SystemImage> ParseSystemImage(const std::wstring& imageName)
{
	StringParser parser(imageName);
	if (!parser.Consume(L"SystemImage:")) return std::nullopt;

	const auto index = parser.ConsumeInt();
	if (!index || *index < 0 || !parser.Consume(L',')) return std::nullopt;

	const auto list = parser.ConsumeRestInt();
	if (!list || *list < SHIL_LARGE || *list > SHIL_JUMBO) return std::nullopt;

	return SystemImage{ *index, *list };
}

HRESULT LoadSystemImage(const SystemImage& systemImage, Gfx::Bitmap* bitmap, const Gfx::Canvas& canvas)
{
	// Windows returns SMALL, LARGE, and EXTRALARGE system image lists scaled to the system DPI.
	// This is based on process DPI awareness, so a DPI-unaware thread context cannot disable it.
	// Rainmeter treats an image's pixel dimensions as its logical meter size, then applies display
	// scaling later. Using that icon directly could make the icon larger if an explicit W/H has not
	// been set.
	Microsoft::WRL::ComPtr<IImageList> imageList;
	HRESULT hr = SHGetImageList(systemImage.list, IID_IImageList, (void**)imageList.GetAddressOf());
	int targetWidth = 0;
	int targetHeight = 0;
	if (SUCCEEDED(hr)) hr = imageList->GetIconSize(&targetWidth, &targetHeight);

	// Recover the requested list's logical size using the system DPI that the Shell used. JUMBO is
	// always 256 pixels and must not be normalized.
	if (SUCCEEDED(hr) && systemImage.list <= SHIL_EXTRALARGE)
	{
		const float dpiScale = (float)System::GetSystemDpi() / USER_DEFAULT_SCREEN_DPI;
		targetWidth = (int)roundf(targetWidth / dpiScale);
		targetHeight = (int)roundf(targetHeight / dpiScale);
	}

	// A different DPI-scaled system image list may already be closer to the logical target. For
	// example, LARGE is 48 pixels at 150% DPI. Prefer that native icon and resample only the remaining
	// difference. On a tie, prefer the larger source so the final conversion downsamples.
	int imageWidth = targetWidth;
	int bestDistance = INT_MAX;
	for (int list = SHIL_LARGE; SUCCEEDED(hr) && list <= SHIL_JUMBO; ++list)
	{
		Microsoft::WRL::ComPtr<IImageList> candidate;
		if (FAILED(SHGetImageList(list, IID_IImageList, (void**)candidate.GetAddressOf()))) continue;

		int width = 0;
		int height = 0;
		if (FAILED(candidate->GetIconSize(&width, &height))) continue;

		const int widthDelta = width > targetWidth ? width - targetWidth : targetWidth - width;
		const int heightDelta = height > targetHeight ? height - targetHeight : targetHeight - height;
		const int distance = widthDelta + heightDelta;
		if (distance < bestDistance ||
			(distance == bestDistance && width >= targetWidth && imageWidth < targetWidth))
		{
			imageList = std::move(candidate);
			imageWidth = width;
			bestDistance = distance;
		}
	}

	HICON icon = nullptr;
	if (SUCCEEDED(hr)) hr = imageList->GetIcon(systemImage.index, ILD_TRANSPARENT, &icon);
	if (SUCCEEDED(hr))
	{
		// Convert the selected physical-sized icon to the requested logical size.
		const float scale = targetWidth > 0 ? (float)imageWidth / targetWidth : 1.0f;
		hr = bitmap->LoadFromIcon(canvas, icon, scale);
	}

	if (icon) DestroyIcon(icon);
	return hr;
}

}  // namespace

// GrayScale Matrix
const D2D1_MATRIX_5X4_F GeneralImage::c_GreyScaleMatrix = {
	0.299f, 0.299f, 0.299f, 0.0f,
	0.587f, 0.587f, 0.587f, 0.0f,
	0.114f, 0.114f, 0.114f, 0.0f,
	  0.0f,   0.0f,   0.0f, 1.0f,
	  0.0f,   0.0f,   0.0f, 0.0f
};

const D2D1_MATRIX_5X4_F GeneralImage::c_IdentityMatrix = {
	1.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 1.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 1.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 1.0f,
	0.0f, 0.0f, 0.0f, 0.0f
};

GeneralImageHelper_DefineOptionArray(GeneralImage::c_DefaultOptionArray, L"");

GeneralImage::GeneralImage(const WCHAR* name, const WCHAR** optionArray, bool disableTransform, Skin* skin) :
	m_Bitmap(nullptr),
	m_BitmapProcessed(nullptr),
	m_Skin(skin),
	m_Name(name ? name : L"ImageName"),
	m_OptionArray(optionArray ? optionArray : c_DefaultOptionArray),
	m_DisableTransform(disableTransform),
	m_Options()
{
}

GeneralImage::~GeneralImage()
{
	DisposeImage();
}

void GeneralImage::DisposeImage()
{
	m_Bitmap.reset();
	m_BitmapProcessed.reset();
}

void GeneralImage::InvalidateDeviceResources()
{
	if (m_Bitmap && m_Bitmap->GetBitmap())
	{
		m_Bitmap->GetBitmap()->InvalidateDeviceResources();
	}

	if (m_BitmapProcessed && m_BitmapProcessed->GetBitmap())
	{
		m_BitmapProcessed->GetBitmap()->InvalidateDeviceResources();
	}
}

bool GeneralImage::IsLoaded()
{
	return GetImage() != nullptr;
}

Gfx::Bitmap* GeneralImage::GetImage()
{
	Gfx::Bitmap* bitmap = m_BitmapProcessed ? m_BitmapProcessed->GetBitmap() :
		(m_Bitmap ? m_Bitmap->GetBitmap() : nullptr);
	if (bitmap && bitmap->HasDeviceResources()) return bitmap;

	if (m_ImageName.empty() || !LoadImage(m_ImageName, m_Options.m_CreateAlphaMask))
	{
		return nullptr;
	}

	return m_BitmapProcessed ? m_BitmapProcessed->GetBitmap() :
		(m_Bitmap ? m_Bitmap->GetBitmap() : nullptr);
}

void GeneralImage::ReadOptions(ConfigParser& parser, std::wstring_view section, const WCHAR* imagePath)
{
	parser.ReadString(m_Path, section, m_OptionArray[OptionIndexImagePath], imagePath);
	PathUtil::AppendBackslashIfMissing(m_Path);

	if (!m_DisableTransform)
	{
		m_Options.m_Crop.left = m_Options.m_Crop.top = m_Options.m_Crop.right = m_Options.m_Crop.bottom = -1;
		m_Options.m_CropMode = ImageOptions::CROPMODE_TL;

		const std::wstring& crop = parser.ReadString(section, m_OptionArray[OptionIndexImageCrop], L"");
		if (!crop.empty())
		{
			StringParser values(crop);

			// Reads the next value of the crop, if there is one.
			auto readValue = [&]() -> std::optional<int>
			{
				values.ConsumeWhitespace();
				if (values.IsConsumed()) return std::nullopt;

				return parser.ParseInt(values.ConsumeUntilOrRest(
					L',', StringParser::SkipWhitespace | StringParser::SkipNestedParentheses), 0);
			};

			const auto x = readValue();
			const auto y = readValue();
			const auto width = readValue();
			const auto height = readValue();
			if (x && y && width && height)
			{
				m_Options.m_Crop.left   = (FLOAT)*x;
				m_Options.m_Crop.top    = (FLOAT)*y;
				m_Options.m_Crop.right  = (FLOAT)(*x + *width);
				m_Options.m_Crop.bottom = (FLOAT)(*y + *height);

				if (const auto mode = readValue())
				{
					m_Options.m_CropMode = (ImageOptions::CROPMODE)*mode;
				}
			}
			else
			{
				LogErrorF(m_Skin, L"%s=%s is not valid in [%.*s]", m_OptionArray[OptionIndexImageCrop], crop.c_str(),
					(int)section.length(), section.data());
			}

			if (m_Options.m_CropMode < ImageOptions::CROPMODE_TL || m_Options.m_CropMode > ImageOptions::CROPMODE_C)
			{
				m_Options.m_CropMode = ImageOptions::CROPMODE_TL;
				LogErrorF(m_Skin, L"%s=%s (origin) is not valid in [%.*s]", m_OptionArray[OptionIndexImageCrop], crop.c_str(),
					(int)section.length(), section.data());
			}
		}
	}

	m_Options.m_GreyScale = parser.ReadBool(section, m_OptionArray[OptionIndexGreyscale], false);

	D2D1_COLOR_F tint = parser.ReadColor(section, m_OptionArray[OptionIndexImageTint], D2D1::ColorF(D2D1::ColorF::White));
	int alpha = parser.ReadInt(section, m_OptionArray[OptionIndexImageAlpha], (INT)(tint.a * 255));  // for backwards compatibility
	alpha = min(255, alpha);
	alpha = max(0, alpha);

	m_Options.m_ColorMatrix = c_IdentityMatrix;

	// Read in the Color Matrix
	// It has to be read in like this because it crashes when reading over 17 floats
	// at one time. The parser does it fine, but after putting the returned values
	// into the Color Matrix the next time the parser is used it crashes.
	// Note: is this still relevant? Kept for BWC
	std::vector<FLOAT> matrix1 = parser.ReadFloats(section, m_OptionArray[OptionIndexColorMatrix1]);
	if (matrix1.size() == 5)
	{
		for (int i = 0; i < 4; ++i)  // The fifth column must be 0.
		{
			m_Options.m_ColorMatrix.m[0][i] = matrix1[i];
		}
	}
	else
	{
		m_Options.m_ColorMatrix.m[0][0] = tint.r;
	}

	std::vector<FLOAT> matrix2 = parser.ReadFloats(section, m_OptionArray[OptionIndexColorMatrix2]);
	if (matrix2.size() == 5)
	{
		for (int i = 0; i < 4; ++i)  // The fifth column must be 0.
		{
			m_Options.m_ColorMatrix.m[1][i] = matrix2[i];
		}
	}
	else
	{
		m_Options.m_ColorMatrix.m[1][1] = tint.g;
	}

	std::vector<FLOAT> matrix3 = parser.ReadFloats(section, m_OptionArray[OptionIndexColorMatrix3]);
	if (matrix3.size() == 5)
	{
		for (int i = 0; i < 4; ++i)  // The fifth column must be 0.
		{
			m_Options.m_ColorMatrix.m[2][i] = matrix3[i];
		}
	}
	else
	{
		m_Options.m_ColorMatrix.m[2][2] = tint.b;
	}

	std::vector<FLOAT> matrix4 = parser.ReadFloats(section, m_OptionArray[OptionIndexColorMatrix4]);
	if (matrix4.size() == 5)
	{
		for (int i = 0; i < 4; ++i)  // The fifth column must be 0.
		{
			m_Options.m_ColorMatrix.m[3][i] = matrix4[i];
		}
	}
	else
	{
		m_Options.m_ColorMatrix.m[3][3] = alpha / 255.0f;
	}

	std::vector<FLOAT> matrix5 = parser.ReadFloats(section, m_OptionArray[OptionIndexColorMatrix5]);
	if (matrix5.size() == 5)
	{
		for (int i = 0; i < 4; ++i)  // The fifth column must be 1.
		{
			m_Options.m_ColorMatrix.m[4][i] = matrix5[i];
		}
	}

	static constexpr ConfigParser::EnumOption<Gfx::Util::FlipType> s_Flips[] =
	{
		{ L"NONE", Gfx::Util::FlipType::None },
		{ L"HORIZONTAL", Gfx::Util::FlipType::Horizontal },
		{ L"VERTICAL", Gfx::Util::FlipType::Vertical },
		{ L"BOTH", Gfx::Util::FlipType::Both },
	};
	m_Options.m_Flip = parser.ReadEnum(section, m_OptionArray[OptionIndexImageFlip], Gfx::Util::FlipType::None, s_Flips);

	if (!m_DisableTransform)
	{
		m_Options.m_Rotate = (FLOAT)parser.ReadFloat(section, m_OptionArray[OptionIndexImageRotate], 0.0);
	}

	m_Options.m_UseExifOrientation = parser.ReadBool(section, m_OptionArray[OptionIndexUseExifOrientation], false);
}

bool GeneralImage::LoadImage(const std::wstring& imageName, bool createAlphaMask)
{
	m_ImageName = imageName;

	if (!m_Skin || imageName.empty())
	{
		DisposeImage();
		return false;
	}

	const bool hasImage = m_Bitmap && m_Bitmap->GetBitmap()->HasDeviceResources() &&
		m_Options.m_CreateAlphaMask == createAlphaMask;
	auto loadImageIfNeeded = [&](const ImageOptions& info, auto&& loader)
	{
		auto handle = GetImageCache().Get(info);
		if (!handle || !handle->GetBitmap()->HasDeviceResources())
		{
			auto bitmap = std::make_unique<Gfx::Bitmap>(info.m_Path, 0, info.m_CreateAlphaMask);
			if (SUCCEEDED(loader(bitmap.get())))
			{
				GetImageCache().Put(info, bitmap.release());
				handle = GetImageCache().Get(info);
			}
			else
			{
				handle.reset();
			}
		}

		DisposeImage();
		if (!handle) return false;

		m_Bitmap = std::move(handle);
		m_Options.m_Path = info.m_Path;
		m_Options.m_FileSize = info.m_FileSize;
		m_Options.m_FileTime = info.m_FileTime;
		m_Options.m_CreateAlphaMask = info.m_CreateAlphaMask;
		ApplyTransforms();
		return true;
	};

	const auto systemImage = ParseSystemImage(imageName);
	if (systemImage)
	{
		ImageOptions info;
		info.m_Path = imageName;
		info.m_FileSize = 1;
		info.m_FileTime = 1;
		info.m_CreateAlphaMask = createAlphaMask;

		if (hasImage && m_Bitmap->GetKey() == info)
		{
			ApplyTransforms();
			return true;
		}

		return loadImageIfNeeded(info, [&](Gfx::Bitmap* bitmap)
		{
			return LoadSystemImage(*systemImage, bitmap, m_Skin->GetCanvas());
		});
	}

	std::wstring filename = m_Path + imageName;
	m_Skin->MakePathAbsolute(filename);

	// Check extension and if it is missing, add .png
	size_t pos = filename.rfind(L'\\');
	if (filename.find(L'.', (pos == std::wstring::npos) ? 0 : pos + 1) == std::wstring::npos)
	{
		filename += L".png";
	}

	if (hasImage && !m_Bitmap->GetBitmap()->HasFileChanged(filename))
	{
		ApplyTransforms();
		return true;
	}

	ImageOptions info;
	Gfx::Bitmap::GetFileInfo(filename, &info);
	info.m_CreateAlphaMask = createAlphaMask;
	if (!info.isValid())
	{
		LogErrorF(m_Skin, L"%s: Unable to open: %s", m_Name, filename.c_str());
		DisposeImage();
		return false;
	}

	return loadImageIfNeeded(info, [&](Gfx::Bitmap* bitmap)
	{
		return bitmap->Load(m_Skin->GetCanvas());
	});
}

D2D1_SIZE_F GeneralImage::ApplyCrop(Gfx::Util::EffectStream* stream, Gfx::Bitmap* bitmap) const
{
	const FLOAT imageW = (FLOAT)bitmap->GetWidth();
	const FLOAT imageH = (FLOAT)bitmap->GetHeight();

	auto& canvas = m_Skin->GetCanvas();

	// Make sure to get the any size changes from EXIF data
	auto size = stream->GetSize(canvas);
	if (size.width <= 0.0f && size.height <= 0.0f)
	{
		size.width = imageW;
		size.height = imageH;
	}

	const auto& crop = m_Options.m_Crop;
	if (crop.right == -1.0f && crop.left == -1.0f && crop.top == -1.0f && crop.bottom == -1.0f)
	{
		return size;
	}

	if (crop.right - crop.left >= 0.0f && crop.bottom - crop.top >= 0.0f)
	{
		FLOAT x = 0.0f;
		FLOAT y = 0.0f;

		switch (m_Options.m_CropMode)
		{
		case ImageOptions::CROPMODE_TL:
		default:
			x = crop.left;
			y = crop.top;
			break;

		case ImageOptions::CROPMODE_TR:
			x = crop.left + imageW;
			y = crop.top;
			break;

		case ImageOptions::CROPMODE_BR:
			x = crop.left + imageW;
			y = crop.top + imageH;
			break;

		case ImageOptions::CROPMODE_BL:
			x = crop.left;
			y = crop.top + imageH;
			break;

		case ImageOptions::CROPMODE_C:
			x = crop.left + (imageW / 2.0f);
			y = crop.top + (imageH / 2.0f);
			break;
		}

		const D2D1_RECT_F rect = D2D1::RectF(x, y, crop.right - crop.left + x, crop.bottom - crop.top + y);
		stream->Crop(canvas, rect);

		size.width = rect.right - rect.left;
		size.height = rect.bottom - rect.top;
	}

	return size;
}

void GeneralImage::ApplyTransforms()
{
	if (!m_Bitmap) return;

	auto* bitmap = m_Bitmap->GetBitmap();
	if (!HasActiveTransforms(bitmap))
	{
		m_BitmapProcessed.reset();
		return;
	}

	if (m_BitmapProcessed && m_BitmapProcessed->GetKey() == m_Options &&
		m_BitmapProcessed->GetBitmap()->HasDeviceResources()) return;

	m_BitmapProcessed.reset();

	auto handle = GetImageCache().Get(m_Options);
	if (!handle || !handle->GetBitmap()->HasDeviceResources())
	{
		auto& canvas = m_Skin->GetCanvas();
		auto* stream = bitmap->CreateEffectStream();

		// To preserve backwards compatibility, apply transforms in the following order:
		// 1. Exif orientation
		// 2. Crop
		// 3. Tinting (greyscale first, then color matrix)
		// 4. Transforms (GDI+ flips, then rotates)

		if (m_Options.m_UseExifOrientation) stream->ApplyExifOrientation(canvas);

		const auto crop = ApplyCrop(stream, bitmap);
		auto* croppedBitmap = stream->ToBitmap(canvas, &crop);
		if (!croppedBitmap)
		{
			delete stream;
			stream = nullptr;
			return;
		}

		if (croppedBitmap != bitmap)
		{
			delete stream;
			stream = croppedBitmap->CreateEffectStream();
		}

		if (m_Options.m_GreyScale) stream->Tint(canvas, c_GreyScaleMatrix);

		if (!CompareColorMatrix(m_Options.m_ColorMatrix, c_IdentityMatrix)) stream->Tint(canvas, m_Options.m_ColorMatrix);

		stream->Flip(canvas, m_Options.m_Flip);

		if (m_Options.m_Rotate != 0.0f) stream->Rotate(canvas, m_Options.m_Rotate);

		auto* newBitmap = stream->ToBitmap(canvas, nullptr);

		delete stream;
		stream = nullptr;

		if (croppedBitmap != bitmap)
		{
			delete croppedBitmap;
			croppedBitmap = nullptr;
		}

		if (newBitmap != nullptr)
		{
			GetImageCache().Put(m_Options, newBitmap);
			handle = GetImageCache().Get(m_Options);
			if (!handle) return;
		}
	}

	if (handle)
	{
		m_BitmapProcessed = std::move(handle);
	}
}

bool GeneralImage::HasActiveTransforms(Gfx::Bitmap* bitmap) const
{
	if (m_Options.m_UseExifOrientation)
	{
		const int orientation = bitmap->GetOrientation();
		if (orientation >= 2 && orientation <= 8)
		{
			return true;
		}
	}

	const auto& crop = m_Options.m_Crop;
	if ((crop.right != -1.0f || crop.left != -1.0f || crop.top != -1.0f || crop.bottom != -1.0f) &&
		crop.right - crop.left >= 0.0f && crop.bottom - crop.top >= 0.0f)
	{
		return true;
	}

	return m_Options.m_GreyScale ||
		!CompareColorMatrix(m_Options.m_ColorMatrix, c_IdentityMatrix) ||
		m_Options.m_Flip != Gfx::Util::FlipType::None ||
		m_Options.m_Rotate != 0.0f;
}

bool GeneralImage::CompareColorMatrix(const D2D1_MATRIX_5X4_F& a, const D2D1_MATRIX_5X4_F& b)
{
	for (int i = 0; i < 5; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			if (a.m[i][j] != b.m[i][j])
			{
				return false;
			}
		}
	}
	return true;
}
