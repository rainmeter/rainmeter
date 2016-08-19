/* Copyright (C) 2016 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_LIBRARY_METERGEOMETRY_H_
#define RM_LIBRARY_METERGEOMETRY_H_

#include "Meter.h"
#include "..\Common\Gfx\Shape\Shape.h"
#include "..\Common\Gfx\Shape\RectangleShape.h"
#include <vector>
#include <functional>
#include <map>

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
	bool HitTest(int x, int y) override;

protected:
	void ReadOptions(ConfigParser& parser, const WCHAR* section) override;
	void BindMeasures(ConfigParser& parser, const WCHAR* section) override;

private:

	std::map<const std::wstring, std::unique_ptr<Gfx::Shape>> m_Shapes;
	std::map<const std::wstring, std::vector<std::pair<std::wstring, std::wstring>>> m_MeasureModifiers;
	void ParseModifiers(ConfigParser& parser, const WCHAR* section, Gfx::Shape** mainShape, const WCHAR* modifierString, bool isExtended = false);
	bool m_NeedsRedraw;

	D2D1_COLOR_F ToGeometryColor(const Gdiplus::Color& color) // Fix this and meter to color!
	{
		return D2D1::ColorF(color.GetR() / 255.0f, color.GetG() / 255.0f, color.GetB() / 255.0f, color.GetA() / 255.0f);
	}

	Gfx::Shape* ParseRectangle(const std::wstring& parameters);

	std::unordered_map<const WCHAR*, std::function<Gfx::Shape*(const std::wstring&)>> shapeRegistry
	{
		{ L"RECTANGLE",[&](const std::wstring& parameters) { return ParseRectangle(parameters); } }
	};
	std::unordered_map<const WCHAR*, std::function<void(ConfigParser&, const std::wstring&, Gfx::Shape* shape, const WCHAR*)>> modifierRegistry
	{
		{ L"FILLCOLOR",[&](ConfigParser& parser, const std::wstring& parameters, Gfx::Shape* shape, const WCHAR* section) { shape->SetFillColor(ToGeometryColor(ConfigParser::ParseColor(parameters.c_str()))); } },
		{ L"STROKEWIDTH",[](ConfigParser& parser, const std::wstring& parameters, Gfx::Shape* shape, const WCHAR* section) { shape->SetStrokeWidth(ConfigParser::ParseInt(parameters.c_str(), 0)); } },
		{ L"STROKECOLOR",[&](ConfigParser& parser, const std::wstring& parameters, Gfx::Shape* shape, const WCHAR* section) { shape->SetStrokeColor(ToGeometryColor(ConfigParser::ParseColor(parameters.c_str()))); } },
		{ L"EXTEND",[&](ConfigParser& parser, const std::wstring& parameters, Gfx::Shape* shape, const WCHAR* section) {
			
			std::vector<std::wstring> tokens = parser.Tokenize(parameters.c_str(), L",");
			for (auto token : tokens) {
				std::wstring options = parser.ReadString(section, token.c_str(), L"");
				if (!options.empty())
					ParseModifiers(parser, section, &shape, options.c_str(), true);
			}
		} 
		}
	};
};

#endif