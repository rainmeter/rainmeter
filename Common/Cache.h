#pragma once
#include <unordered_map>
#include <queue>

template <class T>
struct Deleter
{
	Deleter() = delete;
	~Deleter() = delete;

	static void Delete(T& item) {}
};

template <class T>
struct Deleter<T*>
{
	Deleter() = delete;
	~Deleter() = delete;

	static void Delete(T* item) { if (item) delete item; }
};

template <class T>
class CacheItem
{
public:
	CacheItem(void* cache, const std::wstring& key, T& newBitmap);
	~CacheItem();

	T Get() const;
	std::wstring GetKey() const;

	void AddReference();
	void RemoveReference();

private:
	void* m_Cache;
	std::wstring m_Key;
	T m_Bitmap;
	int m_Refs;
};

template <class T>
class Cache
{
public:
	void Put(const std::wstring& key, T& newBitmap);
	void Remove(const std::wstring& key);
	CacheItem<T>* Get(const std::wstring& key);
	T GetItem(const std::wstring& key);
	bool Contains(const std::wstring& key);
	void ClearUnreferencedItems();

private:
	friend class CacheItem<T>;
	std::unordered_map<std::wstring, CacheItem<T>*> m_Items;
	std::deque<std::wstring> m_DeleteQueue;
};


// Template implementation

template <class T>
CacheItem<T>::CacheItem(void* cache, const std::wstring& key, T& newBitmap) : m_Cache(cache), m_Key(key), m_Bitmap(newBitmap), m_Refs()
{
}

template <class T>
CacheItem<T>::~CacheItem()
{
	Deleter<T>::Del(m_Bitmap);
}

template <class T>
T CacheItem<T>::Get() const
{
	return m_Bitmap;
}

template <class T>
std::wstring CacheItem<T>::GetKey() const
{
	return m_Key;
}

template <class T>
void CacheItem<T>::AddReference()
{
	m_Refs++;
	auto& q = static_cast<Cache<T>*>(m_Cache)->m_DeleteQueue;
	q.erase(std::remove(q.begin(), q.end(), m_Key), q.end());
}

template <class T>
void CacheItem<T>::RemoveReference()
{
	m_Refs--;
	if (m_Refs <= 0)
	{
		static_cast<Cache<T>*>(m_Cache)->Remove(m_Key);
	}
}

template <class T>
void Cache<T>::Put(const std::wstring& key, T& newBitmap)
{
	if (Contains(key))
	{
		delete m_Items[key];
		m_Items[key] = nullptr;
	}

	m_Items.emplace(key, new CacheItem<T>(this, key, newBitmap));
}

template <class T>
void Cache<T>::Remove(const std::wstring& key)
{
	m_DeleteQueue.push_back(key);
}

template <class T>
CacheItem<T>* Cache<T>::Get(const std::wstring& key)
{
	if (Contains(key))
	{
		return m_Items[key];
	}
	return nullptr;
}

template <class T>
T Cache<T>::GetItem(const std::wstring& key)
{
	return Get(key)->Get();
}

template <class T>
bool Cache<T>::Contains(const std::wstring& key)
{
	return m_Items.find(key) != m_Items.end();
}

template <class T>
void Cache<T>::ClearUnreferencedItems()
{
	while (m_DeleteQueue.size())
	{
		auto it = m_Items.find(m_DeleteQueue.front());
		if (it != m_Items.end())
		{
			delete m_Items[m_DeleteQueue.front()];
			m_Items.erase(it);
		}
		m_DeleteQueue.pop_front();
	}

}
