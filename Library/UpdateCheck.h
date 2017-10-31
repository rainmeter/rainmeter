/* Copyright (C) 2017 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __UPDATE_CHECK_H__
#define __UPDATE_CHECK_H__

#define JSON_NOEXCEPTION

#include "json/json.hpp"

class Updater
{
public:
	static Updater& GetInstance();

	void CheckUpdate();
	void CheckLanguage();

private:
	Updater();
	~Updater();

	Updater(const Updater& other) = delete;
	Updater& operator=(Updater other) = delete;

	static void CheckVersion(void* dummy);
	static int ParseVersion(LPCWSTR str);

	nlohmann::json m_Status;

	static LPCWSTR c_UpdateURL;
};

// Convenience function.
inline Updater& GetUpdater() { return Updater::GetInstance(); }

#endif
