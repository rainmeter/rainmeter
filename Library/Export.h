/* Copyright (C) 2011 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __EXPORT_H__
#define __EXPORT_H__

#include "../Plugins/API/RainmeterAPI.h"

/* DEPRECATED */ LIBRARY_EXPORT __declspec(deprecated) LPCWSTR ReadConfigString(LPCWSTR section, LPCWSTR option, LPCWSTR defValue);

/* DEPRECATED */ LIBRARY_EXPORT __declspec(deprecated) LPCWSTR PluginBridge(LPCWSTR command, LPCWSTR data);

#endif
