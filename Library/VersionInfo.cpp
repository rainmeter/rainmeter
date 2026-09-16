// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "VersionInfo.h"
#include "../Version.h"

namespace VersionInfo {

const wchar_t* GetAppVersion()
{
	return APPVERSION;
}

int GetRevision()
{
	return revision_number;
}

unsigned int GetRainmeterVersion()
{
	return RAINMETER_VERSION;
}

const wchar_t* GetBuildTime()
{
#ifdef BUILD_TIME
	return BUILD_TIME;
#else
	return nullptr;
#endif
}

const wchar_t* GetCommitHash()
{
#ifdef COMMIT_HASH
	return COMMIT_HASH;
#else
	return nullptr;
#endif
}

}
