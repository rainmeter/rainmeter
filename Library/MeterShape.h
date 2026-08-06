// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "../Common/Map.h"
#include "../Common/StringParser.h"
#include "../Common/Gfx/Shape.h"
#include "Meter.h"

class MeterShape : public Meter
{
public:
	MeterShape(Skin* skin, const WCHAR* name);
	virtual ~MeterShape();

	MeterShape(const MeterShape& other) = delete;
	MeterShape& operator=(MeterShape other) = delete;

	UINT GetTypeID() override { return TypeID<MeterShape>(); }

	virtual bool Update();
	virtual bool Draw(Gfx::Canvas& canvas);
	virtual void InvalidateDeviceResources() override;

	bool HitTest(int x, int y);

protected:
	virtual void ReadOptions(ConfigParser& parser, const WCHAR* section);
	virtual void BindMeasures(ConfigParser& parser, const WCHAR* section);

private:
	void Dispose();

	bool CreateShape(std::wstring_view definition, ConfigParser& parser, const WCHAR* section, bool& isCombined, size_t keyId);
	bool CreateCombinedShape(ConfigParser& parser, size_t shapeId, std::wstring& options);

	void ParseModifiers(Gfx::Shape& shape, StringParser& modifiers, ConfigParser& parser, const WCHAR* section, bool recursive = false);
	bool ParseTransformModifers(ConfigParser& parser, Gfx::Shape& shape, StringParser& transform);
	bool ParseGradient(Gfx::Shape& shape, ConfigParser& parser, Gfx::BrushType type, std::wstring_view options, bool altGamma, bool isStroke);
	bool ParsePath(ConfigParser& parser, std::wstring_view options, D2D1_FILL_MODE fillMode);
	std::wstring ReadShapeOption(ConfigParser& parser, const WCHAR* section, std::wstring key);

	std::vector<Gfx::Shape> m_Shapes;
	StringMap<std::wstring> m_ShapeOptions;
};
