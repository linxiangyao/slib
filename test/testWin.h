#include "../src/comm/comm.h"
#if defined(S_OS_WIN)
#include "../src/socket/socketLib.h"
#include "../src/thread/threadLib.h"
#include "testS.h"
using namespace std;
USING_NAMESPACE_S




//class __testSocketApi__testIoOverlapRunnable : public IRun
//{
//public:
//    __testSocketApi__testIoOverlapRunnable()
//    {
//    }
//
//    ~__testSocketApi__testIoOverlapRunnable()
//    {
//    }
//
//    void run()
//    {
//        __print("client: start");
//
//        SOCKET s = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
//        WSAOVERLAPPED  overlapped;
//        WSAEVENT eventArray[WSA_MAXIMUM_WAIT_EVENTS];
//        struct sockaddr addr;
//        DWORD dwSend = 0;
//
//
//        __print("client: exit");
//    }
//
//    void stop()
//    {
//    }
//};
//
//void __testSocketApi__testIoOverlap()
//{
//    __testSocketApi__testSocketServerRunnable sr;
//    Thread st(&sr);
//    st.start();
//
//    Thread::sleep(10);
//    __testSocketApi__testIoOverlapRunnable cr;
//    Thread ct(&cr);
//    ct.start();
//
//    __sleepAndPrintAndPauseConsole(800, "app: press to exit-----------------");
//    cr.stop();
//    Thread::sleep(10);
//    sr.stop();
//}



//
//
//
//
//class __testSocketApi__testWsaEventRunnable : public IRun
//{
//public:
//    __testSocketApi__testWsaEventRunnable(WSAEVENT e)
//    {
//        event = e;
//    }
//
//    void run()
//    {
//        __print("thread: enter");
//        WSAWaitForMultipleEvents(1, &event, FALSE, WSA_INFINITE, FALSE);
//        __print("thread: exit");
//    }
//
//    WSAEVENT event;
//};
//
//
//void __testSocketApi__testWsaEvent()
//{
//    WSAEVENT event = WSACreateEvent();
//
//    __testSocketApi__testWsaEventRunnable sr(event);
//    Thread st(&sr);
//    st.start();
//
//    __sleepAndPrintAndPauseConsole(10, "app: press to set event-----------------");
//    WSASetEvent(event);
//
//    WSACloseEvent(event);
//}
//
//
//
//
//
//class __testSocketApi__testWsaAsyncEventSelectClientRunnable : public IRun
//{
//public:
//    __testSocketApi__testWsaAsyncEventSelectClientRunnable()
//    {
//        api = new SocketApi(ESocketApiType_asyncSelect);
//        event = WSACreateEvent();
//    }
//
//    void run()
//    {
//        __print("client: start");
//        if (!api->openTcpSocket(&s))
//        {
//            __print("client: fail to openTcpSocket, exit");
//            return;
//        }
//
//        if (!api->tcpSocketConnect(&s, "127.0.0.1", 99001))
//        {
//            __print("client: fail to connect to server, exit");
//            return;
//        }
//
//        // send
//        {
//            //std::string strSend = "GET http://wwww.baidu.com/index.html HTTP/1.1\r\nAccept: text/html\r\n\r\n";
//            std::string strSend = "hello, i am client";
//            if (!api->tcpSocketSend(&s, (const byte_t*)strSend.c_str(), strSend.size()))
//            {
//                __print("client: fail to send data to server, exit");
//                return;
//            }
//        }
//
//        // recv
//        {
//            byte_t buf[2048];
//            size_t recvLen = 0;
//            if (!api->tcpSocketRecv(&s, buf, 2048, &recvLen))
//            {
//                __print("client: fail to recv data from server, exit");
//                return;
//            }
//
//            std::string recvData;
//            recvData.append((const char*)buf, recvLen);
//            __print("client: recv data from client, msg=%s", recvData.c_str());
//        }
//
//        api->closeSocket(&s);
//        __print("client: exit");
//
//
//
//        WSAWaitForMultipleEvents(1, &event, FALSE, WSA_INFINITE, FALSE);
//
//
//        __print("client: exit");
//    }
//
//    void stop()
//    {
//        WSASetEvent(event);
//        api->closeSocket(&s);
//    }
//
//
//    WSAEVENT event;
//    SocketApi* api;
//    socket_t s;
//};
//
//
//void __testSocketApi__testWsaAsyncEventSelectServerAndClient()
//{
//    __testSocketApi__testSocketServerRunnable sr;
//    Thread st(&sr);
//    st.start();
//
//    Thread::sleep(10);
//    __testSocketApi__testWsaAsyncEventSelectClientRunnable cr;
//    Thread ct(&cr);
//    ct.start();
//
//    __sleepAndPrintAndPauseConsole(1000, "app: press to exit-----------------");
//    cr.stop();
//    Thread::sleep(10);
//    sr.stop();
//}
//
//
//
//



void __testWin()
{
    
    //WSADATA wsaData;
    //WSAStartup(MAKEWORD(2, 2), &wsaData);
    
    //SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
    //if (s == INVALID_SOCKET)
    //{
    //    return;
    //}


    ////WSAEVENT event = WSACreateEvent();



    //fd_set readSet;
    //FD_ZERO(&readSet);
    //Mutex m;
    //FD_SET((SOCKET)m.internal(), &readSet);
    //int ret = select((SOCKET)m.internal() + 1, &readSet, NULL, NULL, NULL);
    //if (ret == SOCKET_ERROR)
    //{

    //}

}

#endif
