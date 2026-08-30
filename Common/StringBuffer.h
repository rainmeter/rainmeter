// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include <concepts>
#include <stdlib.h>
#include <string_view>
#include <type_traits>

// A null-terminated character buffer that holds up to |N| characters inline and spills to the heap
// beyond that. sizeof(StringBuffer) grows with |N|, so this is meant for short-lived locals and
// must not be stored in a long-lived object.
//
// The buffer cannot be copied or moved, so a function that returns one must construct it in the
// return statement using the constructor below. That makes the return a prvalue, which C++17
// requires to be constructed directly in the caller's storage, in every build configuration.
template<typename T, size_t N>
class StringBuffer
{
	static_assert(std::is_trivial_v<T>);

public:
	StringBuffer() { m_Inline[0] = T(); }

	// Fills the buffer as part of constructing it, so that the result can be returned as a prvalue.
	template<typename Fill>
	requires std::invocable<Fill&, StringBuffer&>
	explicit StringBuffer(Fill&& fill)
	{
		m_Inline[0] = T();
		fill(*this);
	}

	~StringBuffer() { free(m_Heap); }

	StringBuffer(const StringBuffer&) = delete;
	StringBuffer(StringBuffer&&) = delete;
	StringBuffer& operator=(const StringBuffer&) = delete;
	StringBuffer& operator=(StringBuffer&&) = delete;

	// Makes room for |count| characters plus the terminator and returns the buffer, or nullptr if
	// the allocation failed. The existing contents are not preserved.
	T* Reserve(size_t count)
	{
		if (count > m_Capacity)
		{
			T* heap = (T*)malloc((count + 1) * sizeof(T));
			if (!heap) return nullptr;

			free(m_Heap);
			m_Heap = heap;
			m_Capacity = count;
		}
		return data();
	}

	// Terminates the buffer at |count| characters. |count| must not exceed the reserved capacity.
	void SetLength(size_t count)
	{
		m_Length = count;
		data()[count] = T();
	}

	T* data() { return m_Heap ? m_Heap : m_Inline; }
	const T* data() const { return m_Heap ? m_Heap : m_Inline; }
	const T* c_str() const { return data(); }

	size_t length() const { return m_Length; }
	size_t capacity() const { return m_Capacity; }
	bool empty() const { return m_Length == 0; }

	operator std::basic_string_view<T>() const { return { data(), m_Length }; }

	static constexpr size_t InlineCapacity = N;

private:
	T* m_Heap = nullptr;
	size_t m_Capacity = N;
	size_t m_Length = 0;
	T m_Inline[N + 1];
};
