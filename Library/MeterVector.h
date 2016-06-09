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
#include <memory>
#include "../Common/Gfx/GeometryShape.h"

#define CreateCustomOptionArray(name) \
	const WCHAR* (name)[TintedImage::OptionCount] = { \
		L"", \
		L"", \
		L"", \
		L"", \
		L"", \
		L"", \
		L"", \
		L"", \
		L"", \
		L"", \
		L"", \
		L"", \
		L"" \
	};
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
	struct VectorShape :  Gfx::GeometryShape {

		VectorShape() : Gfx::GeometryShape(),
			m_Rotation(),
			m_RotationCenter(D2D1::Point2F(FLT_MIN, FLT_MIN)),
			m_Skew(),
			m_Offset(),
			m_ConnectEdges(true),
			m_ShouldRender(true),
			m_X(),
			m_Y(),
			m_W(),
			m_H(),
			m_CombineWith(),
			m_CombineMode(),
			m_ImageW(),
			m_ImageH(),
			m_ImageRotation(),
			m_ImageName(),
			m_ImageDstRect(),
			ShapeType(),
			ShapeOptions(),
			m_ShapeParsed()
		{}


		float m_Rotation;
		D2D1_POINT_2F m_RotationCenter;
		D2D1_POINT_2F m_Skew;
		D2D1_SIZE_F m_Scale = D2D1::SizeF(1, 1);
		D2D1_SIZE_F m_Offset;

		bool m_ConnectEdges;
		bool m_ShouldRender;

		double m_X;
		double m_Y;
		double m_W;
		double m_H;

		std::wstring m_CombineWith;
		std::wstring m_CombineMode;


		double m_ImageW;
		double m_ImageH;
		double m_ImageRotation;
		std::wstring m_ImageName;
		Gdiplus::Rect m_ImageDstRect;

		std::wstring ShapeType;
		std::wstring ShapeOptions;
		bool m_ShapeParsed;
	};

	//Functions to load the different shape types
	bool ParseRect(VectorShape& shape, RECT& rect);
	bool ParseRoundedRect(VectorShape& shape, LPCWSTR option, ConfigParser& parser);
	bool ParseEllipse(VectorShape& shape, LPCWSTR option, ConfigParser& parser);
	bool ParsePie(VectorShape& shape, LPCWSTR option, ConfigParser& parser);
	bool ParseArc(VectorShape& shape, LPCWSTR option, ConfigParser& parser);
	bool ParseCurve(VectorShape& shape, LPCWSTR option, ConfigParser& parser);
	bool ParseCustom(VectorShape& shape, LPCWSTR option, ConfigParser& parser, const WCHAR * section);

	/*
	** Loads the image from disk, mostly copied from MeterImage. Transformed to store images during runtime
	**
	*/
	TintedImage* LoadVectorImage(const std::wstring imageName, const WCHAR** OptionArray, VectorShape &shape, ConfigParser& parser, const WCHAR* section);
	//Slight modification to the Tokenize function, it should probably be in ConfigParser. I'll let Brian and/or ponru decide, located at the bottom of the cpp
	std::vector<std::wstring> CustomTokenize(const std::wstring& str, const std::wstring& delimiters);
	//These could probably be moved to ConfigParser too, located at the bottom of the cpp
	D2D1_POINT_2F ParsePoint(const LPCWSTR& string, ConfigParser& parser, double defaultVal = 0);
	D2D1_SIZE_F ParseSize(const LPCWSTR& string, ConfigParser& parser, double defaultVal = 0);

	//Functions to parse some of the more advanced options
	void ParseImage(std::wstring options, VectorShape& shape, ConfigParser& parser, const WCHAR* section);
	void ParseGradient(std::wstring options, VectorShape& shape, ConfigParser& parser, const WCHAR* section);
	void ParseStrokeStyle(std::wstring options, VectorShape& shape, ConfigParser& parser, const WCHAR* section);
	double ParseRotation(const LPCWSTR& string, ConfigParser& parser, VectorShape& shape);
	Microsoft::WRL::ComPtr<ID2D1Geometry> CombineShapes(VectorShape& shape1, std::vector<VectorShape*> shapes);
	bool CombineGeometry(VectorShape& shape);



private:

	std::map<std::wstring, VectorShape> m_Shapes;
	std::map<std::wstring, std::unique_ptr<TintedImage>> m_Images;
	CreateCustomOptionArray(c_CusomOptionArray);

};
#endif