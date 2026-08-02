// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

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
