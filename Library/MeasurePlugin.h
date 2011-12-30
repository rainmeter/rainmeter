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

#ifndef __MEASUREPLUGIN_H__
#define __MEASUREPLUGIN_H__

#include "Measure.h"

typedef UINT (*INITIALIZE)(HMODULE, LPCTSTR, LPCTSTR, UINT);
typedef VOID (*FINALIZE)(HMODULE, UINT);
typedef UINT (*UPDATE)(UINT);
typedef double (*UPDATE2)(UINT);
typedef LPCTSTR (*GETSTRING)(UINT, UINT);
typedef void (*EXECUTEBANG)(LPCTSTR, UINT);

typedef void (*NEWINITIALIZE)(void*);
typedef void (*NEWRELOAD)(void*, void*, double*);
typedef void (*NEWFINALIZE)(void*);
typedef double (*NEWUPDATE)(void*);
typedef LPCWSTR (*NEWGETSTRING)(void*);
typedef void (*NEWEXECUTEBANG)(void*, const WCHAR*);

class CMeasurePlugin : public CMeasure
{
public:
	CMeasurePlugin(CMeterWindow* meterWindow, const WCHAR* name);
	virtual ~CMeasurePlugin();

	virtual bool Update();
	virtual const WCHAR* GetStringValue(AUTOSCALE autoScale, double scale, int decimals, bool percentual);
	virtual void ExecuteBang(const WCHAR* args);

protected:
	virtual void ReadConfig(CConfigParser& parser, const WCHAR* section);

private:
	bool IsNewApi() { return m_ReloadFunc != NULL; }

	std::wstring m_PluginName;
	HMODULE m_Plugin;

	void* m_ReloadFunc;

	union
	{
		struct
		{
			UINT m_ID;
			bool m_Update2;
		};

		struct
		{
			void* m_PluginData;
		};
	};

	void* m_UpdateFunc;
	void* m_GetStringFunc;
	void* m_ExecuteBangFunc;
};

#endif
