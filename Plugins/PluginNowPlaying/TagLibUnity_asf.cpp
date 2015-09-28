/* Copyright (C) 2014 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

// TagLib unity build: This file includes several TagLib source files. By compiling all of them
// together, the build performance is greatly increased.

#pragma warning(push)
#pragma warning(disable: 4244; disable: 4267)

#include "taglib\asf\asfattribute.cpp"
#include "taglib\asf\asffile.cpp"
#include "taglib\asf\asfpicture.cpp"
#include "taglib\asf\asfproperties.cpp"
#include "taglib\asf\asftag.cpp"

#pragma warning(pop)
