/*
  Copyright (C) 2013 Brian Ferguson

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

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
		value(-1.0f),
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
