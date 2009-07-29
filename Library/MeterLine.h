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
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#ifndef __METERLINE_H__
#define __METERLINE_H__

#include "Meter.h"
#include "MeterWindow.h"

class CMeterLine : public CMeter
{
public:
	CMeterLine(CMeterWindow* meterWindow);
	virtual ~CMeterLine();

	virtual void ReadConfig(const WCHAR* section);
	virtual void Initialize();
	virtual bool Update();
	virtual bool Draw(Gdiplus::Graphics& graphics);
	virtual void BindMeasure(std::list<CMeasure*>& measures);

private:
	std::vector<std::wstring> m_MeasureNames;		// Name of the other measures
	std::vector<CMeasure*> m_Measures;				// Pointer ot the other measures

	std::vector<Gdiplus::Color> m_Colors;			// Color of the histograms
	std::vector<double> m_ScaleValues;				// The scale values for the measures

	bool m_Autoscale;								// If true, the meter is automatically adjusted to show all values
	bool m_HorizontalLines;							// If true, horizontal lines will ba drawn on the meter
	bool m_Flip;
	double m_LineWidth;
	Gdiplus::Color m_HorizontalColor;						// Color of the horizontal lines

	std::vector< std::vector<double> > m_AllValues;			// All the values to be drawn
	int m_CurrentPos;												// Place of the current value
};

#endif
