/* Copyright (C) 2001 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __METERPLUGIN_H__
#define __METERPLUGIN_H__

#include "Meter.h"
#include "Export.h"
#include "MeasurePlugin.h"
#include "../Common/Gfx/Canvas.h"

typedef void (*METERRELOAD)(void*, void*);
typedef void (*METERUPDATE)(void*, void*, double, LPCWSTR);
typedef bool (*METERDRAW)(void*, void*, ID2D1DeviceContext*);

class MeterPlugin : public Meter
{
public:
	MeterPlugin(Skin* skin, const WCHAR* name);
	virtual ~MeterPlugin();

	MeterPlugin(const MeterPlugin& other) = delete;
	MeterPlugin& operator=(MeterPlugin other) = delete;

	virtual UINT GetTypeID() { return TypeID<MeterPlugin>(); }

	virtual bool Update();
	virtual bool Draw(Gfx::Canvas& canvas);

protected:
	virtual void ReadOptions(ConfigParser& parser, const WCHAR* section);
	virtual void BindMeasures(ConfigParser& parser, const WCHAR* section);

private:
	// returns if loading and freeing the plugin library is handled by the measure
	bool m_PluginHandledByMeasure;

	std::wstring m_PluginName;
	HMODULE m_Plugin;

	void* m_PluginData;

	MeasurePlugin *m_PluginMeasure;

	void* m_MeterReloadFunc;
	void* m_MeterUpdateFunc;
	void* m_MeterDrawFunc;
};

#endif

