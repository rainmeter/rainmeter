// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include <string>

namespace CrashDump {

void Initialize(std::wstring dumpFolderPath, std::wstring comment);
bool DidFindRepeatedCrashes();

}  // namespace CrashDump
