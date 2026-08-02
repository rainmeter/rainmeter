// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include <CppUnitTest.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

// VS IntelliSense doesn't seem to work well with the CppUnitTest.h TEST_CLASS macro. This is a
// simpler version, which seems to work fine.
#undef TEST_CLASS
#define TEST_CLASS(className) class className : public TestClass<className>
