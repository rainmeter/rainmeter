/*
  Copyright (C) 2002 Kimmo Pekkola

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
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef __METERLINE_H__
#define __METERLINE_H__

#include "Meter.h"

class MeterLine : public Meter
{
public:
	MeterLine(MeterWindow* meterWindow, const WCHAR* name);
	virtual ~MeterLine();

	virtual UINT GetTypeID() { return TypeID<MeterLine>(); }

	virtual void Initialize();
	virtual bool Update();
	virtual bool Draw(Gfx::Canvas& canvas);

protected:
	virtual void ReadOptions(ConfigParser& parser, const WCHAR* section);
	virtual void BindMeasures(ConfigParser& parser, const WCHAR* section);

private:
	std::vector<Gdiplus::Color> m_Colors;
	std::vector<double> m_ScaleValues;

	bool m_Autoscale;
	bool m_HorizontalLines;
	bool m_Flip;
	double m_LineWidth;
	Gdiplus::Color m_HorizontalColor;

	std::vector< std::vector<double> > m_AllValues;
	int m_CurrentPos;

	bool m_GraphStartLeft;
	bool m_GraphHorizontalOrientation;
};

#endif
