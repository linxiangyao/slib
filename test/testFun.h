//#ifndef __TEST_FUN_H_
//#define __TEST_FUN_H_
//
//#include "testS.h"
//#include "../src/util/fun.h"
//using namespace std;
//USING_NAMESPACE_S
//
//
//
//
//
//
//int __printint(int i)
//{
//    cout << "i=" << i << endl;
//    return i;
//}
//typedef int(*iifun_t) (int);
//
//int __print2int(int i, int j)
//{
//    cout << "i=" << i <<", j=" << j << endl;
//    return i;
//}
//
//
//class A
//{
//public:
//    A() { m_i = 999; }
//
//    int f(int i)
//    {
//        cout << "i=" << i << ", m_i=" << m_i++ << endl;
//        return i;
//    }
//
//    int f(int i, int j)
//    {
//        cout << "i=" << i << ", j= " << j << ", m_i=" << m_i++ << endl;
//        return i;
//    }
//private:
//    int m_i;
//};
//
//typedef int (A::*aiifun_t)(int);
//
//
//
//void __testFun()
//{
//	printf("\n__testFun ---------------------------------------------------------\n");
//    //{
//    //    A a;
//    //    A* pa = &a;
//    //    aiifun_t aiifun = &A::f;
//    //    (a.*aiifun)(2);
//    //    (pa->*aiifun)(3);
//    //}
//
//    {
//        Function<int(int)> f(&__printint);
//        f.invoke(1);
//    }
//
//    {
//        Function<int(int, int)> f(&__print2int);
//        f.invoke(2, 10000);
//    }
//
//    {
//        A a;
//        Function<int(A::*)(int)> f(&a, &A::f);
//        f.invoke(5);
//        f.invoke(6);
//    }
//
//    {
//        A a;
//        Function<int(A::*)(int, int)> f(&a, &A::f);
//        f.invoke(7, 1000000);
//        f.invoke(8, 1000000);
//    }
//}
//
//#endif // !__TEST_FUN_H
//
//
//
//
//
//
//
//
//
///*
//
//
//template<typename T>
//struct has_hello {
//template<typename U, int& (U::*)()> struct HELPS;
//template<typename U> static char Test(HELPS<U, &U::hello>*);
//template<typename U> static int Test(...);
//const static bool Has = sizeof(Test<T>(0)) == sizeof(char);
//};
//
//struct A
//{
//int& hello() {
//cout << "A is Hello." << endl;
//return x;
//}
//int x;
//};
//
////template<typename T>
//struct AB
//{
//static long long m_f(long long);
//static char m_f(char);
//const static bool Has = sizeof(m_f((long long)0)) == sizeof(long long);
////const static bool Has = sizeof(m_f<T>(0)) == sizeof(char);
//};
//
//int ffsdfsdf()
//{
////    has_hello<A>::Has;
//return 0;
//}
//
//template<typename T = int>
//class TestClass
//{
//public:
//void f()
//{
//
//}
//
//T m_t;
//};
//
//
////template<typename T=float>
////class TestClass
////{
////public:
////    void f()
////    {
////
////    }
////
////    T m_t;
////};
//
//template<>
//class TestClass<float>
//{
//public:
//void f()
//{
//
//}
//
//float m_t;
//};
//
//
//
//*/
//
