/* Copyright (C) 2013 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"

#ifndef __PLUGINRUNCOMMAND_H__
#define __PLUGINRUNCOMMAND_H__

enum OutputType
{
	OUTPUTTYPE_ANSI,
	OUTPUTTYPE_UTF8,
	OUTPUTTYPE_UTF16
};

struct Measure
{
	// Options
	std::wstring program;
	std::wstring parameter;
	std::wstring finishAction;
	std::wstring outputFile;
	std::wstring folder;
	WORD state;
	int timeout;
	OutputType outputType;

	// Internal values
	double value;
	std::wstring result;
	std::recursive_mutex mutex;
	bool threadActive;
	void* skin;
	void* rm;

	// Process info
	HANDLE hProc;
	DWORD dwPID;

	Measure() :
		program(),
		parameter(),
		finishAction(),
		outputFile(),
		folder(),
		state(0),
		timeout(-1),
		outputType(OUTPUTTYPE_UTF16),
		value(-1.0),
		result(),
		mutex(),
		threadActive(false),
		skin(),
		rm(),
		hProc(INVALID_HANDLE_VALUE),
		dwPID(0)
		{ }
};

#endif
