/* Copyright (C) 2001 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __MEASUREPLUGIN_H__
#define __MEASUREPLUGIN_H__

#include "Measure.h"
#include "Export.h"

typedef UINT (*INITIALIZE)(HMODULE, LPCTSTR, LPCTSTR, UINT);
typedef VOID (*FINALIZE)(HMODULE, UINT);
typedef UINT (*UPDATE)(UINT);
typedef double (*UPDATE2)(UINT);
typedef LPCTSTR (*GETSTRING)(UINT, UINT);
typedef void (*EXECUTEBANG)(LPCWSTR, UINT);

typedef void (*NEWINITIALIZE)(void*, void*);
typedef void (*NEWRELOAD)(void*, void*, double*);
typedef void (*NEWFINALIZE)(void*);
typedef double (*NEWUPDATE)(void*);
typedef LPCWSTR (*NEWGETSTRING)(void*);
typedef void (*NEWEXECUTEBANG)(void*, LPCWSTR);

typedef LPCWSTR(*CUSTOMFUNCTION)(void*, const int, const WCHAR* argv[]);

class MeasurePlugin : public Measure
{
public:
	MeasurePlugin(Skin* skin, const WCHAR* name);
	virtual ~MeasurePlugin();

	MeasurePlugin(const MeasurePlugin& other) = delete;
	MeasurePlugin& operator=(MeasurePlugin other) = delete;

	virtual UINT GetTypeID() { return TypeID<MeasurePlugin>(); }

	virtual const WCHAR* GetStringValue();
	virtual void Command(const std::wstring& command);

	bool CommandWithReturn(const std::wstring& command, std::wstring& strValue);

protected:
	virtual void ReadOptions(ConfigParser& parser, const WCHAR* section);
	virtual void UpdateValue();

private:
	bool IsNewApi() { return m_ReloadFunc != nullptr; }

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
