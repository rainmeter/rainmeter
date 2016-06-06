/* Copyright (C) 2016 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_LIBRARY_METERGEOMETRY_H_
#define RM_LIBRARY_METERGEOMETRY_H_

#include "Meter.h"
#include <vector>
#include "..\Common\Gfx\Shape.h"

class MeterGeometry :
	public Meter
{
public:
	MeterGeometry(Skin* skin, const WCHAR* name);
	virtual ~MeterGeometry();

	MeterGeometry(const MeterGeometry& other) = delete;
	MeterGeometry& operator=(MeterGeometry other) = delete;

	UINT GetTypeID() override { return TypeID<MeterGeometry>(); }

	void Initialize() override;
	bool Update() override;
	bool Draw(Gfx::Canvas& canvas) override;

protected:
	void ReadOptions(ConfigParser& parser, const WCHAR* section) override;
	void BindMeasures(ConfigParser& parser, const WCHAR* section) override;

private:
	struct GeometryShape : Gfx::Shape {
		GeometryShape() : 
			m_Rotation(),
			m_RotationCenter(),
			m_Skew(),
			m_Scale(D2D1::SizeF(1, 1)),
			m_Offset()

		{}

		double m_Rotation;
		D2D1_POINT_2F m_RotationCenter;
		D2D1_POINT_2F m_Skew;
		D2D1_SIZE_F m_Scale;
		D2D1_SIZE_F m_Offset;

		D2D1_RECT_F m_Bounds;
	};
	bool ParseShape(GeometryShape & shape, const LPCWSTR& optionName, const LPCWSTR& optionValue);
	bool ReplaceShapeOption(GeometryShape & shape, const LPCWSTR& optionName, const LPCWSTR& optionValue);

	std::vector<std::wstring> CustomTokenize(const std::wstring& str, const std::wstring& delimiters);
	D2D1_POINT_2F ParsePoint(const LPCWSTR& string, double defaultVal);
	D2D1_SIZE_F ParseSize(const LPCWSTR& string, double defaultVal);
	bool ContainsMeasures(const std::wstring& str);
	bool IsPostOption(const LPCWSTR& option);
	bool IsShape(const LPCWSTR& option);

	Microsoft::WRL::ComPtr<ID2D1Geometry> ParseRect(GeometryShape& shape, RECT& rect);
	double ParseRotation(const WCHAR* string, double defaultValue, GeometryShape& shape);

	std::map<std::wstring, GeometryShape> m_Shapes;
	std::map<const std::wstring, std::vector<std::pair<std::wstring, std::wstring>>> m_MeasureOptions;
	bool m_NeedsRedraw;
};

#endif