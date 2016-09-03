/* Copyright (C) 2013 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include <CppUnitTest.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

// VS IntelliSense doesn't seem to work well with the CppUnitTest.h TEST_CLASS macro. This is a 
// simpler version, which seems to work fine.
#undef TEST_CLASS
#define TEST_CLASS(className) class className : public TestClass<className>
