#include "../src/comm/comm.h"
#if defined(S_OS_MAC)
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include "../src/socket/socketApi.h"
#include "testS.h"
using namespace std;
USING_NAMESPACE_S;





void __testMacGetHostByName()
{
//    const char* svrName = "www.baidu.com";
//    uint32_t ipAddr = 0;
//    if(!SocketUtil::getIpByName(svrName, &ipAddr))
//    {
//        std::string str = std::string("fail to get host by name, name=") + svrName;
//        __print("%s", str.c_str());
//        return;
//    }
//    
//    std::string ipStr;
//    SocketUtil::ipToStr(ipAddr, &ipStr);
//    
//    std::string str = std::string("get host by name ok, name=") + svrName + ", ipStr=" + ipStr;
//    __print("%s", str.c_str());
}

void __testMac()
{
    __testMacGetHostByName();
}



#endif

//void __testMacBlockSocket()
//{
//    const char* svrName = "http://www.baiud.com";
//    struct hostent *host;
//    struct sockaddr_in serv_addr;
//
//    if ((host = gethostbyname(svrName)) == NULL) {
//        printf("gethostbyname error\n");
//        exit(0);
//    }
//
//
//    struct sockaddr_in server_addr;
//    server_addr.sin_len = sizeof(struct sockaddr_in);
//    server_addr.sin_family = AF_INET; //Address families AF_INET
//    server_addr.sin_port = htons(11332);
//    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
//    bzero(&(server_addr.sin_zero),8);
//
//    int s = socket(AF_INET, SOCK_STREAM, 0);
//    if (s == -1) {
//        perror("socket error");
//        return;
//    }
//
//    if (connect(s, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in)) == 0)
//    {
//    }
//}

//void __testMacSocket()
//{
//    string ip = "127.0.0.1";
//    int port = 25698;
//    int c_fd, flags, ret;
//    struct sockaddr_in s_addr;
//    memset(&s_addr, 0, sizeof (s_addr));
//    s_addr.sin_family = AF_INET;
//    s_addr.sin_port = htons(port);
//    s_addr.sin_addr.s_addr = inet_addr(ip.c_str());
//
//    if ((c_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
//        perror("create socket fail.\n");
//        exit(0);
//    }
//    flags = fcntl(c_fd, F_GETFL, 0);
//    if (flags < 0) {
//        perror("get socket flags fail.\n");
//        return;
//    }
//
//    if (fcntl(c_fd, F_SETFL, flags | O_NONBLOCK) < 0) {
//        perror("set socket O_NONBLOCK fail.\n");
//        return;
//    }
//    ret = connect(c_fd, (struct sockaddr*) &s_addr, sizeof (struct sockaddr));
//    while (ret < 0) {
//        if (errno == EINPROGRESS) {
//            break;
//        } else {
//            perror("connect remote server fail.\n");
//            printf("%d\n", errno);
//            exit(0);
//        }
//    }
//}

