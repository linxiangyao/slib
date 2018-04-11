//#ifndef S_HASH_GROUP_MAP_H_
//#define S_HASH_GROUP_MAP_H_
//#include <vector>
//#include <map>
//
//
//
//
//
//template<typename Key, typename Value>
//class HashGroupMap
//{
//public:
//	typedef size_t(*fun_t_hash_key_to_group_index_fun)(const Key& key);
//	typedef std::map<Key, Value> __Group;
//
//	class iterator
//	{
//	public:
//		iterator& operator++()
//		{
//			__Group* group = m_hash_map->m_group_vector[m_group_index];
//			if (m_it == group->end())
//			{
//				return *this;
//			}
//
//			m_it++;
//
//			while (m_group_index < m_hash_map->m_group_vector.size() - 1)
//			{
//				m_group_index++;
//				__Group* group = m_hash_map->m_group_vector[m_group_index];
//				if (group->size() == 0)
//					continue;
//					
//				m_it = group->begin();
//				break;
//			}
//
//			first = m_it->first;
//			second = m_it->second;
//
//			return *this;
//		}
//
//		bool operator ==(iterator& r) const
//		{
//			return m_it == r.m_it;
//		}
//
//		bool operator !=(iterator& r) const
//		{
//			return m_it != r.m_it;
//		}
//
//		Key first;
//		Value second;
//
//	private:
//		friend class HashGroupMap;
//		HashGroupMap* m_hash_map;
//		size_t m_group_index;
//		typename __Group::iterator m_it;
//	};
//
//	~HashGroupMap()
//	{
//		for (size_t i = 0; i < m_group_vector.size(); ++i)
//		{
//			delete m_group_vector[i];
//		}
//	}
//
//	bool init(size_t group_count, fun_t_hash_key_to_group_index_fun hash_fun = NULL)
//	{
//		if (group_count == 0)
//			return false;
//
//		m_hash_fun = hash_fun;
//		for (size_t i = 0; i < group_count; ++i)
//		{
//			m_group_vector.push_back(new __Group());
//		}
//		return true;
//	}
//
//	iterator begin()
//	{
//		iterator it;
//		it.m_hash_map = this;
//		it.m_group_index = 0;
//		it.m_it = m_group_vector[0]->begin();
//		return it;
//	}
//
//	iterator end()
//	{
//		iterator it;
//		it.m_hash_map = this;
//		it.m_group_index = 0;
//		it.m_it = m_group_vector[m_group_vector.size() - 1]->end();
//		return it;
//	}
//
//	size_t size() const
//	{
//		size_t s = 0;
//		for (size_t i = 0; i < m_group_vector.size(); ++i)
//		{
//			s += m_group_vector[i]->size();
//		}
//		return s;
//	}
//
//	bool hasKey(const Key& key) const
//	{
//		size_t group_index = m_hash_fun(key);
//		__Group* group = __getGroupByGroupIndex(group_index);
//		if (group == NULL)
//			return false;
//
//		__Group::iterator it = group->find(key);
//		if (it == group->end())
//			return false;
//		return true;
//	}
//
//	void eraseByKey(const Key& key)
//	{
//		size_t group_index = m_hash_fun(key);
//		__Group* group = __getGroupByGroupIndex(group_index);
//		if (group == NULL)
//			return;
//
//		group->erase(key);
//	}
//
//	bool getValueByKey(const Key& key, Value* value) const
//	{
//		size_t group_index = m_hash_fun(key);
//		__Group* group = __getGroupByGroupIndex(group_index);
//		if (group == NULL)
//			return false;
//
//		__Group::iterator it = group->find(key);
//		if (it == group->end())
//			return false;
//
//		*value = it.second;
//		return true;
//	}
//
//	bool setKeyValue(const Key& key, const Value& value)
//	{
//		size_t group_index = m_hash_fun(key);
//		__Group* group = __getGroupByGroupIndex(group_index);
//		if (group == NULL)
//			return false;
//
//		(*group)[key] = value;
//		return true;
//	}
//
//	void clear()
//	{
//		for (size_t i = 0; i < m_group_vector.size(); ++i)
//		{
//			m_group_vector[i]->clear();
//		}
//	}
//
//
//private:
//	__Group* __getGroupByGroupIndex(size_t index) const
//	{
//		if (index >= m_group_vector.size())
//			return NULL;
//		return m_group_vector[index];
//	}
//
//	fun_t_hash_key_to_group_index_fun m_hash_fun;
//	std::vector<__Group*> m_group_vector;
//};
//
//
//
//
//
//#endif
//
//
