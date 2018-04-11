//#ifndef S_RVECTOR_H_
//#define S_RVECTOR_H_
//#include <vector>
//
//
//
//template<typename Data>
//class RVector
//{
//};
//
//template<typename Data>
//class RVector<Data*>
//{
//public:
//    RVector()
//    {
//    }
//
//	void init(size_t data_count)
//	{
//		for (size_t i = 0; i < data_count; ++i)
//		{
//			m_datas.push_back(new Data());
//		}
//	}
//
//    virtual ~RVector()
//    {
//        for (size_t i = 0; i < m_datas.size(); ++i)
//        {
//            delete m_datas[i];
//        }
//        m_datas.clear();
//    }
//
//    Data* operator[](size_t index) const
//    {
//        return m_datas[index];
//    }
//
//    size_t size() const
//    {
//        return m_datas.size();
//    }
//
//
//    Data* at(size_t index) const
//    {
//        return m_datas[index];
//    }
//private:
//    std::vector<Data*> m_datas;
//};
//
//
//
//
//
//#endif
//
//
