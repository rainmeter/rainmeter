/*
  Copyright (C) 2013 Birunthan Mohanathas

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
#include "Install.h"
#include "Resource.h"

extern "C" {
#include "lzma/Alloc.h"
#include "lzma/7zFile.h"
#include "lzma/7zVersion.h"
#include "lzma/LzmaDec.h"
#include "lzma/7zCrc.h"
#include "lzma/7z.h"
#include "lzma/7zMemInStream.h"
#include "lzma/7zAlloc.h"
}  // extern "C"

void ExtractPayload(const void* payload, size_t payloadSize, const WCHAR* prefix)
{
	CMemInStream memStream;
	MemInStream_Init(&memStream, payload, payloadSize);

	CrcGenerateTable();

	ISzAlloc alloc = { SzAlloc, SzFree };
	CSzArEx db;
	SzArEx_Init(&db);
	SRes res = SzArEx_Open(&db, &memStream.s, &alloc, &alloc);
	if (res != SZ_OK)
	{
		SzArEx_Free(&db, &alloc);
		return;
	}

	WCHAR buffer[MAX_PATH];
	UInt32 blockIndex = 0xFFFFFFFF;
	Byte* outBuffer = 0;
	size_t outBufferSize = 0;

	for (UInt32 i = 0; i < db.db.NumFiles; i++)
	{
		size_t offset = 0;
		size_t outSizeProcessed = 0;
		const CSzFileItem* f = db.db.Files + i;

		SzArEx_GetFileNameUtf16(&db, i, (UInt16*)buffer);
		WCHAR* destPath = buffer;
		if (wcsncmp(destPath, prefix, 3) == 0 ||
			wcsncmp(destPath, L"ALL", 3) == 0)
		{
			// Skip the prefix (X64/X32/ALL) and the path separater.
			destPath += 4;
		}
		else
		{
			// This file isn't for this arch.
			continue;
		}

		if (!f->IsDir)
		{
			res = SzArEx_Extract(
				&db, &memStream.s, i, &blockIndex, &outBuffer, &outBufferSize, &offset,
				&outSizeProcessed, &alloc, &alloc);
			if (res != SZ_OK)
			{
				break;
			}
		}

		for (size_t j = 0; destPath[j] != L'\0'; ++j)
		{
			if (destPath[j] == L'/')
			{
				destPath[j] = L'\0';
				CreateDirectory(destPath, NULL);
				destPath[j] = CHAR_PATH_SEPARATOR;
			}
		}

		if (f->IsDir)
		{
			CreateDirectory(destPath, NULL);
			continue;
		}

		CSzFile outFile;
		if (OutFile_OpenW(&outFile, destPath))
		{
			res = SZ_ERROR_FAIL;
			break;
		}
		size_t processedSize = outSizeProcessed;
		if (File_Write(&outFile, outBuffer + offset, &processedSize) != 0 ||
			processedSize != outSizeProcessed)
		{
			res = SZ_ERROR_FAIL;
			break;
		}
		if (File_Close(&outFile))
		{
			res = SZ_ERROR_FAIL;
			break;
		}

		if (f->AttribDefined) SetFileAttributesW(destPath, f->Attrib);
	}

	IAlloc_Free(&alloc, outBuffer);
	SzArEx_Free(&db, &alloc);

	if (res == SZ_OK)
	{
		// Success.
	}
}

void DoInstall(InstallOptions& options)
{
#ifdef INSTALLER_INCLUDE_PAYLOAD
	const auto module = GetModuleHandle(nullptr);
	HRSRC payload = FindResource(
		module, MAKEINTRESOURCE(IDR_PAYLOAD), MAKEINTRESOURCE(PAYLOAD_RESOURCE_TYPEID));
	const size_t payloadSize = SizeofResource(module, payload);
	const void* payloadData = LockResource(LoadResource(module, payload));

	SetCurrentDirectory(options.targetPath);
	ExtractPayload(
		payloadData, payloadSize, options.arch == InstallArch::X32 ? L"X32" : L"X64");
#endif
}
