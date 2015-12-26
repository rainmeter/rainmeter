/* Copyright (C) 2015 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_COMMON_SCOPEDFUNCTION_H_
#define RM_COMMON_SCOPEDFUNCTION_H_

// Executes function T when the ScopedFunction is destructed.
template<typename T>
class ScopedFunction
{
public:
	explicit ScopedFunction(T&& func) : m_Func(std::move(func)) {}
	explicit ScopedFunction(const T&) = delete;

	~ScopedFunction()
	{
		m_Func();
	}

	void operator=(T&&) = delete;
	void operator=(const T&) = delete;

private:
	T m_Func;
};

// Helper to create ScopedFunction instances. Use as follows:
//  auto scopedFunction = Scoped([&] { work(); });
template<typename T>
ScopedFunction<T> Scoped(T t)
{
	return ScopedFunction<T>(std::move(t));
}

#endif
