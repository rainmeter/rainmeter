/* Copyright (C) 2016 Rainmeter Project Developers
*
* This Source Code Form is subject to the terms of the GNU General Public
* License; either version 2 of the License, or (at your option) any later
* version. If a copy of the GPL was not distributed with this file, You can
* obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_LIBRARY_MeterShape_H_
#define RM_LIBRARY_MeterShape_H_

#include "Meter.h"
#include "..\Common\Gfx\Shape\Shape.h"
#include "..\Common\Gfx\Shape\RectangleShape.h"
#include <map>

class MeterShape :
	public Meter
{
public:
	MeterShape(Skin* skin, const WCHAR* name);
	virtual ~MeterShape();

	MeterShape(const MeterShape& other) = delete;
	MeterShape& operator=(MeterShape other) = delete;

	UINT GetTypeID() override { return TypeID<MeterShape>(); }

	void Initialize() override;
	bool Update() override;
	bool Draw(Gfx::Canvas& canvas) override;
	bool HitTest(int x, int y) override;

protected:
	void ReadOptions(ConfigParser& parser, const WCHAR* section) override;
	void BindMeasures(ConfigParser& parser, const WCHAR* section) override;

private:
	std::wstring HandleModifier(const WCHAR* modifier, ConfigParser& parser, const WCHAR* parameters, Gfx::Shape* shape, const WCHAR* section, int recursion = 0);
	WCHAR* IsModifier(const WCHAR* modifier);
	const WCHAR* HandleShape(Gfx::Shape*& shape, const WCHAR* shapeType, const WCHAR* parameters);
	WCHAR* IsShape(const WCHAR* shape);
	bool CompareWChar(const WCHAR* str1, const WCHAR* str2);

	std::vector<Gfx::Shape*> m_Shapes;
	bool ParseModifiers(ConfigParser& parser, const WCHAR* section, Gfx::Shape*& mainShape, const WCHAR* shapeId, const WCHAR* modifierString, int recursion = 0);
	bool m_NeedsRedraw;

	D2D1_COLOR_F ToGeometryColor(const Gdiplus::Color& color) // Fix this and meter to color!
	{
		return D2D1::ColorF(color.GetR() / 255.0f, color.GetG() / 255.0f, color.GetB() / 255.0f, color.GetA() / 255.0f);
	}

};

#endif