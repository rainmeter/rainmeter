/* Copyright (C) 2001 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __MEASUREREGISTRY_H__
#define __MEASUREREGISTRY_H__

#include "Measure.h"

class MeasureRegistry : public Measure
{
public:
	MeasureRegistry(Skin* skin, const WCHAR* name);
	virtual ~MeasureRegistry();

	MeasureRegistry(const MeasureRegistry& other) = delete;
	MeasureRegistry& operator=(MeasureRegistry other) = delete;

	virtual UINT GetTypeID() { return TypeID<MeasureRegistry>(); }

	virtual const WCHAR* GetStringValue();

protected:
	virtual void ReadOptions(ConfigParser& parser, const WCHAR* section);
	virtual void UpdateValue();

private:
	std::wstring m_RegKeyName;
	std::wstring m_RegValueName;
	std::wstring m_StringValue;
    HKEY m_RegKey;
    HKEY m_HKey;
};

#endif
