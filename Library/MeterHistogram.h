/*
  Copyright (C) 2001 Kimmo Pekkola

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
/*
  $Header: /home/cvsroot/Rainmeter/Library/MeterHistogram.h,v 1.1.1.1 2005/07/10 18:51:06 rainy Exp $

  $Log: MeterHistogram.h,v $
  Revision 1.1.1.1  2005/07/10 18:51:06  rainy
  no message

  Revision 1.9  2004/06/05 10:55:54  rainy
  Too much changes to be listed in here...

  Revision 1.8  2003/02/10 18:12:45  rainy
  Now uses GDI+

  Revision 1.7  2002/03/31 09:58:53  rainy
  Added some comments

  Revision 1.6  2001/09/26 16:26:23  rainy
  Small adjustement to the interfaces.

  Revision 1.5  2001/09/01 12:58:48  rainy
  Removed MaxValues (i.e. they aren't stored anymore).
  Fixed a bug in bitmap histogram placement.

  Revision 1.4  2001/08/25 17:07:28  rainy
  Added support for background images behind the curves.

  Revision 1.3  2001/08/19 09:13:13  rainy
  Invert moved to the measures.

  Revision 1.2  2001/08/12 15:38:54  Rainy
  Adjusted Update()'s interface.
  Added invert for the secondary measure.

  Revision 1.1.1.1  2001/08/11 10:58:19  Rainy
  Added to CVS.

*/

#ifndef __METERHISTOGRAM_H__
#define __METERHISTOGRAM_H__

#include "Meter.h"
#include "MeterWindow.h"

class CMeterHistogram : public CMeter
{
public:
	CMeterHistogram(CMeterWindow* meterWindow);
	virtual ~CMeterHistogram();

	virtual void ReadConfig(const WCHAR* section);
	virtual void Initialize();
	virtual bool Update();
	virtual bool Draw();
	virtual void BindMeasure(std::list<CMeasure*>& measures);

private:
	std::wstring m_SecondaryMeasureName;	// Name of the secondary measure
	CMeasure* m_SecondaryMeasure;		// Pointer ot the secondary measure
	Gdiplus::Color m_PrimaryColor;			// Color of the primary histogram
	Gdiplus::Color m_SecondaryColor;			// Color of the secondary histogram
	Gdiplus::Color m_BothColor;				// Color when the both histograms overlap

	int m_MeterPos;						// Position of the meter (i.e. where the new value should be placed)
	bool m_Autoscale;
	bool m_Flip;

	std::wstring m_PrimaryImageName;		// Name of the primary image for bitmap histograms
	std::wstring m_SecondaryImageName;	// Name of the secondary image for bitmap histograms
	std::wstring m_BothImageName;		// Name of the image for overlapping histograms

	Gdiplus::Bitmap* m_PrimaryBitmap;			// The primary bitmap
	Gdiplus::Bitmap* m_SecondaryBitmap;			// The secondary bitmap
	Gdiplus::Bitmap* m_BothBitmap;				// The overlap bitmap

	double* m_PrimaryValues;
	double* m_SecondaryValues;

	double m_MaxPrimaryValue;
	double m_MinPrimaryValue;
	double m_MaxSecondaryValue;
	double m_MinSecondaryValue;
};

#endif
