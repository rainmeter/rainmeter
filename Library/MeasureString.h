/* Copyright (C) 2014 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __MEASURESTRING_H__
#define __MEASURESTRING_H__

#include "Measure.h"

class MeasureString : public Measure
{
public:
	MeasureString(Skin* skin, const WCHAR* name);
	virtual ~MeasureString();

	MeasureString(const MeasureString& other) = delete;
	MeasureString& operator=(MeasureString other) = delete;

	virtual const WCHAR* GetStringValue();

	virtual UINT GetTypeID() { return TypeID<MeasureString>(); }

protected:
	virtual void ReadOptions(ConfigParser& parser, const WCHAR* section);
	virtual void UpdateValue();

private:
	std::wstring m_String;
};

#endif
