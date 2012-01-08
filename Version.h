#pragma once
#define MAKE_VER(major, minor1, minor2) major * 1000000 + minor1 * 1000 + minor2

#define FILEVER        2,3,0,1091
#define PRODUCTVER     2,3,0,1091
#define STRFILEVER     "2.3.0.1091"
#define STRPRODUCTVER  "2.3.0.1091"

#define APPVERSION L"2.3.0"
#define RAINMETER_VERSION MAKE_VER(2, 3, 0)

const int revision_number = 1091;
const bool revision_beta = true;
