/* Copyright (C) 2021 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_COMMON_NETWORKUTIL_H_
#define RM_COMMON_NETWORKUTIL_H_

#include <winsock2.h>
#include <ws2tcpip.h>
#include <Iphlpapi.h>

class __declspec(novtable) NetworkUtil final
{
public:
	static void Initialize();
	static void Finalize();

	static bool UpdateInterfaceTable();

	static const ULONG GetInterfaceCount() { return s_InterfaceCount; }
	static MIB_IF_ROW2* GetInterfaceTable() { if (s_InterfaceTable) { return s_InterfaceTable->Table; } return nullptr; }

	static ULONG FindBestInterface(LPCWSTR interfaceName);
	static ULONG GetIndexFromIfIndex(const ULONG ifIndex);

	static LPCWSTR GetInterfaceTypeString(const ULONG ifIndex);
	static LPCWSTR GetInterfaceMediaConnectionString(const NET_IF_MEDIA_CONNECT_STATE state);
	static LPCWSTR GetInterfaceOperStatusString(const IF_OPER_STATUS status);

private:
	NetworkUtil() = delete;
	~NetworkUtil() = delete;

	NetworkUtil(const NetworkUtil& other) = delete;
	NetworkUtil& operator=(NetworkUtil other) = delete;

	static void DisposeInterfaceTable();

	static ULONG s_InterfaceCount;
	static MIB_IF_TABLE2* s_InterfaceTable;
};

#endif
