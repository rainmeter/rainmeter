/* Copyright (C) 2002 Rainmeter Project Developers
*
* This Source Code Form is subject to the terms of the GNU General Public
* License; either version 2 of the License, or (at your option) any later
* version. If a copy of the GPL was not distributed with this file, You can
* obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __METERGEOMETRY_H__
#define __METERGEOMETRY_H__

#include "Meter.h"
#include <vector>
#include <map>
#include "TintedImage.h"


class MeterVector :
	public Meter
{
public:
	MeterVector(Skin* skin, const WCHAR* name);
	virtual ~MeterVector();

	MeterVector(const MeterVector& other) = delete;
	MeterVector& operator=(MeterVector other) = delete;

	virtual UINT GetTypeID() { return TypeID<MeterVector>(); }

	virtual void Initialize();
	virtual bool Update();
	virtual bool Draw(Gfx::Canvas& canvas);

protected:
	virtual void ReadOptions(ConfigParser& parser, const WCHAR* section);
	virtual void BindMeasures(ConfigParser& parser, const WCHAR* section);

	struct VectorShape {
		
		Microsoft::WRL::ComPtr<ID2D1Geometry> m_Geometry;
		int m_OutlineWidth = 1;
		Gdiplus::Color m_OutlineColor = Gdiplus::Color::Black;
		Gdiplus::Color m_FillColor = Gdiplus::Color::White;
		bool m_ShapeParsed = false;
		float m_Rotation = 0;

		bool m_ConnectEdges = true;

		double m_X = 0;
		double m_Y = 0;
		double m_W = 0;
		double m_H = 0;

		std::wstring m_CombineWith = L"";
		std::wstring m_CombineMode = L"";


		TintedImage* m_Image = NULL;
		double m_ImageW = 0;
		double m_ImageH = 0;
		std::wstring m_ImageName;
		Gdiplus::Rect m_ImageDstRect;
	};
	void ParseRect(VectorShape& shape, RECT& rect);
	bool ParseRoundedRect(VectorShape& shape, LPCWSTR option, ConfigParser& parser);
	bool ParseEllipse(VectorShape& shape, LPCWSTR option, ConfigParser& parser);
	bool ParseCustom(VectorShape& shape, LPCWSTR option, ConfigParser& parser, const WCHAR * section);

	std::vector<std::wstring> CombineOptions(std::vector<std::wstring> options)
	{
		std::vector<std::wstring> end;

		bool inOption = false;
		int parents = 0;
		std::wstring current;

		for (int i = 0; i < options.size(); i++)
		{
			auto strBegin = options[i].find_first_not_of(L"\t");
			if (strBegin == std::string::npos)
			{
				i++;
				continue;
			}
			while (strBegin != options[i].find_last_not_of(L"\t")+1) {
				if (options[i].at(strBegin) == L'(')
				{
					++parents;
					inOption = true;
				}
				else if (options[i].at(strBegin) == L')')
					--parents;
				if (parents == 0)
					inOption = false;
				++strBegin;
			}
			current += options[i];
			if (!inOption) {
				end.push_back(current);
				current.clear();
			}
			else
				current += L",";
		}
		return end;

	}

	Microsoft::WRL::ComPtr<ID2D1PathGeometry> CombineShapes(VectorShape& shape1, VectorShape& shape2)
	{
		D2D1_COMBINE_MODE mode;
		if (_wcsicmp(shape1.m_CombineMode.c_str(), L"Union") == 0) {
			mode = D2D1_COMBINE_MODE_UNION;
			return Gfx::Canvas::CombineGeometry(shape1.m_Geometry.Get(), shape2.m_Geometry.Get(), mode);
		}
		else if (_wcsicmp(shape1.m_CombineMode.c_str(), L"Intersect") == 0) {
			mode = D2D1_COMBINE_MODE_INTERSECT;
			return Gfx::Canvas::CombineGeometry(shape1.m_Geometry.Get(), shape2.m_Geometry.Get(), mode);
		}
		else if (_wcsicmp(shape1.m_CombineMode.c_str(), L"XOR") == 0) {
			mode = D2D1_COMBINE_MODE_XOR;
			return Gfx::Canvas::CombineGeometry(shape1.m_Geometry.Get(), shape2.m_Geometry.Get(), mode);
		}
		else if (_wcsicmp(shape1.m_CombineMode.c_str(), L"Exclude") == 0) {
			mode = D2D1_COMBINE_MODE_EXCLUDE;
			return Gfx::Canvas::CombineGeometry(shape1.m_Geometry.Get(), shape2.m_Geometry.Get(), mode);
		}
		return nullptr;
	}
	/*
	** Loads the image from disk
	**
	*/
	TintedImage* LoadImage(const std::wstring& imageName, bool bLoadAlways, VectorShape &shape)
	{
		if (m_Images.find(imageName) != m_Images.end()) 
			return m_Images[imageName];
		TintedImage* image = new TintedImage(L"ImageName", nullptr, false, m_Skin);
		image->LoadImage(imageName, bLoadAlways);

		if (image->IsLoaded())
		{

			Gdiplus::Bitmap* bitmap = image->GetImage();

			int imageW = bitmap->GetWidth();
			int imageH = bitmap->GetHeight();

			shape.m_ImageW = imageW;
			shape.m_ImageH = imageH;
		}
		m_Images.insert(std::map<std::wstring, TintedImage*>::value_type({ imageName ,image }));
		return image;
	}

private:


	
	Gdiplus::Color m_fillColor;
	Gdiplus::Color m_outlineColor;
	bool m_Solid;
	bool m_connectedEdges;
	float m_lineWidth;
	std::map<std::wstring, VectorShape> m_Shapes;
	std::map<std::wstring, TintedImage*> m_Images;

	Skin* m_Skin;

};
#endif