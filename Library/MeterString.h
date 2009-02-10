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
  $Header: /home/cvsroot/Rainmeter/Library/MeterString.h,v 1.1.1.1 2005/07/10 18:51:06 rainy Exp $

  $Log: MeterString.h,v $
  Revision 1.1.1.1  2005/07/10 18:51:06  rainy
  no message

  Revision 1.9  2004/07/11 17:18:32  rainy
  Fixed width calculation.

  Revision 1.8  2004/06/05 10:55:54  rainy
  Too much changes to be listed in here...

  Revision 1.7  2003/02/10 18:12:44  rainy
  Now uses GDI+

  Revision 1.6  2002/07/01 15:32:20  rainy
  Added NumOfDecimals

  Revision 1.5  2002/03/31 09:58:53  rainy
  Added some comments

  Revision 1.4  2001/09/26 16:26:23  rainy
  Small adjustement to the interfaces.

  Revision 1.3  2001/09/01 12:57:33  rainy
  Added support for percentual measuring.

  Revision 1.2  2001/08/19 09:12:44  rainy
  no message

  Revision 1.1  2001/08/12 15:35:07  Rainy
  Inital Version


*/

#ifndef __METERSTRING_H__
#define __METERSTRING_H__

#include "Meter.h"
#include "MeterWindow.h"
namespace Gdiplus
{
class Font;
}

class CMeterString : public CMeter
{
public:
	CMeterString(CMeterWindow* meterWindow);
	virtual ~CMeterString();

	virtual int GetX(bool abs = false);

	virtual void ReadConfig(const WCHAR* section);
	virtual void Initialize();
	virtual bool Update();
	virtual bool Draw();
	virtual void BindMeasure(std::list<CMeasure*>& measures);

private:
	enum TEXTSTYLE
	{
		NORMAL,
		BOLD,
		ITALIC,
		BOLDITALIC
	};

	bool DrawString(Gdiplus::RectF* rect);

	Gdiplus::Color m_Color;		// The color of the text
	std::wstring m_Postfix;		// The postfix of the text
	std::wstring m_Prefix;		// The prefix of the text
	std::wstring m_Text;			// The text
	std::wstring m_FontFace;		// name of the font face
	bool m_AutoScale;			// true, if the value should be autoscaled
	METER_ALIGNMENT m_Align;	// Alignment of the text
	TEXTSTYLE m_Style;			// Style of the text
	int m_FontSize;				// Size of the fonts
	double m_Scale;				// Scaling if autoscale is not used
	bool m_NoDecimals;			// Number of decimals to use
	bool m_Percentual;			// True, if the value should be given as %
	bool m_AntiAlias;			// True, the text is antialiased
	bool m_ClipString;			// True, the text is clipped in borders (adds ellipsis to the end of the string)
	Gdiplus::Font* m_Font;		// The font
	Gdiplus::FontFamily* m_FontFamily;		// The font family
	int m_NumOfDecimals;		// Number of decimals to be displayed
	bool m_DimensionsDefined;
	Gdiplus::REAL m_Angle;

	std::wstring m_String;

	std::vector<std::wstring> m_MeasureNames;
	std::vector<CMeasure*> m_Measures;
};

#endif

/*
E	eksa	10^18
P	peta	10^15
T	tera	10^12
G	giga	10^9
M	mega	10^6
k	kilo	10^3
*/	
