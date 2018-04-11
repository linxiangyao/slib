//#ifndef S_INVOKER_H
//#define S_INVOKER_H
//#include <string>
//#include <vector>
//#include <map>
//#include <algorithm>
//#include "ns.h"
//S_NAMESPACE_BEGIN
//
//
//
//
//
//template<typename Funtor>
//class InvokeDispatcher
//{
//};
//
//template<typename Funtor>
//class InvokeDispatcher<Funtor*>
//{
//public:
//	void registerFuntor(Funtor* f)
//	{
//		m_funs.push_back(f);
//	}
//
//	void removeFuntor(Funtor* f)
//	{
//		m_funs.erase(std::find(m_funs.begin(), m_funs.end(), f));
//	}
//
//protected:
//	std::vector<Funtor*> m_funs;
//};
//
//
//#define DECLARE_INVOKE_FUN_0(FunName) void invoke_##FunName() {  for (size_t i = 0; i < m_funs.size(); ++i) { m_funs[i]->FunName(); } }
//#define DECLARE_INVOKE_FUN_1(FunName, ArgType1, arg1) void invoke_##FunName(ArgType1 arg1) {  for (size_t i = 0; i < m_funs.size(); ++i) { m_funs[i]->FunName(arg1); } }
//#define DECLARE_INVOKE_FUN_2(FunName, ArgType1, arg1, ArgType2, arg2) void invoke_##FunName(ArgType1 arg1, ArgType2 arg2) {  for (size_t i = 0; i < m_funs.size(); ++i) { m_funs[i]->FunName(arg1, arg2); } }
//#define DECLARE_INVOKE_FUN_3(FunName, ArgType1, arg1, ArgType2, arg2, ArgType3, arg3) void invoke_##FunName(ArgType1 Arg1, ArgType2 arg2, ArgType3 arg3) {  for (size_t i = 0; i < m_funs.size(); ++i) { m_funs[i]->FunName(arg1, arg2, arg3); } }
//
//
//
//
//S_NAMESPACE_END
//#endif
//
