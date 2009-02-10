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
/*
  $Header: /home/cvsroot/Rainmeter/Library/MeterImage.h,v 1.1.1.1 2005/07/10 18:51:06 rainy Exp $

  $Log: MeterImage.h,v $
  Revision 1.1.1.1  2005/07/10 18:51:06  rainy
  no message

  Revision 1.5  2004/07/11 17:17:48  rainy
  The image is not locked anymore on disk.

  Revision 1.4  2004/06/05 10:55:54  rainy
  Too much changes to be listed in here...

  Revision 1.3  2003/12/05 15:50:10  Rainy
  Multi-instance changes.

  Revision 1.2  2003/02/10 18:12:45  rainy
  Now uses GDI+

  Revision 1.1  2002/04/27 10:28:31  rainy
  Intial version.

*/

#ifndef __METERIMAGE_H__
#define __METERIMAGE_H__

#include "Meter.h"
#include "MeterWindow.h"

namespace Gdiplus 
{
	class Bitmap;
};

class CMeterImage : public CMeter
{
public:
	CMeterImage(CMeterWindow* meterWindow);
	virtual ~CMeterImage();

	virtual void ReadConfig(const WCHAR* section);
	virtual void Initialize();
	virtual bool Update();
	virtual bool Draw();
	virtual void BindMeasure(std::list<CMeasure*>& measures);

private:
	void LoadImage(bool bLoadAlways);

	Gdiplus::Bitmap* m_Bitmap;			// The bitmap
	std::wstring m_ImageName;			// Name of the image
	bool m_DimensionsDefined;
	HGLOBAL m_hBuffer;
	FILETIME m_Modified;
};

#endif
