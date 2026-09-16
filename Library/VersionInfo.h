// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

namespace VersionInfo {

const wchar_t* GetAppVersion();
int GetRevision();
unsigned int GetRainmeterVersion();
const wchar_t* GetBuildTime();
const wchar_t* GetCommitHash();

}
