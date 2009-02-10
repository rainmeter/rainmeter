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
  $Header: /home/cvsroot/Rainmeter/Library/MeasurePlugin.h,v 1.1.1.1 2005/07/10 18:51:06 rainy Exp $

  $Log: MeasurePlugin.h,v $
  Revision 1.1.1.1  2005/07/10 18:51:06  rainy
  no message

  Revision 1.5  2004/06/05 10:55:54  rainy
  Too much changes to be listed in here...

  Revision 1.4  2002/07/01 15:34:57  rainy
  Added GetStringValue.

  Revision 1.3  2002/04/26 18:24:15  rainy
  Modified the Update method to support disabled measures.

  Revision 1.2  2001/12/23 10:17:02  rainy
  The plugins get unique ID automatically.
  The plugins are also loaded from the Rainmeter's folder.

  Revision 1.1  2001/10/28 09:07:19  rainy
  Inital version

*/

#ifndef __MEASUREPLUGIN_H__
#define __MEASUREPLUGIN_H__

#include "Measure.h"

typedef UINT (*INITIALIZE)(HMODULE, LPCTSTR, LPCTSTR, UINT); 
typedef VOID (*FINALIZE)(HMODULE, UINT);
typedef UINT (*UPDATE)(UINT); 
typedef double (*UPDATE2)(UINT); 
typedef LPCTSTR (*GETSTRING)(UINT, UINT); 
typedef void (*EXECUTEBANG)(LPCTSTR, UINT);

class CMeasurePlugin : public CMeasure
{
public:
	CMeasurePlugin(CMeterWindow* meterWindow);
	virtual ~CMeasurePlugin();

	virtual void ReadConfig(CConfigParser& parser, const WCHAR* section);
	virtual bool Update();
	virtual const WCHAR* GetStringValue(bool autoScale, double scale, int decimals, bool percentual);
	virtual void ExecuteBang(const WCHAR* args);

private:
	std::wstring m_PluginName;
	HMODULE m_Plugin;
	UINT m_ID;

	INITIALIZE InitializeFunc;
	FINALIZE FinalizeFunc;
	UPDATE UpdateFunc;
	UPDATE2 UpdateFunc2;
	GETSTRING GetStringFunc;
	EXECUTEBANG ExecuteBangFunc;
};

#endif
