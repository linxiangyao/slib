#ifndef S_SOCKET_API_TYPE_H_
#define S_SOCKET_API_TYPE_H_
#include "../comm/comm.h"
#include "../util/stringUtil.h"
#if defined(S_OS_LINUX) | defined(S_OS_MAC) | defined(S_OS_ANDROID)
	#include <vector>
	#include <stdio.h>
	#include <errno.h>
	#include <string.h>
	#include <stdlib.h>

	#include <netdb.h>
	#include <arpa/inet.h>
	#include <sys/ioctl.h>
	#include <fcntl.h>
	#include <stdio.h>
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <unistd.h>
	#include <sys/shm.h>
#endif
S_NAMESPACE_BEGIN


#ifdef WIN32
#else
	#define SOCKET int
#endif

typedef SOCKET socket_t;
typedef int64_t socket_id_t;
typedef int64_t socket_select_event_id_t;

enum ETcpSocketType
{
    ETcpSocketType_client,
    ETcpSocketType_svr_listen,
    ETcpSocketType_svr_tran,
};


enum ETcpSocketThreadMode
{
    ETcpSocketThreadMode_singleThread,
    ETcpSocketThreadMode_shareThread,
    ETcpSocketThreadMode_threadPool,
};


class TcpSocketThreadConfig
{
public:
    TcpSocketThreadConfig() { m_thread_mode = ETcpSocketThreadMode_singleThread; m_thread_pool_thread_num = 0; }
    
    ETcpSocketThreadMode m_thread_mode;
    std::string m_share_thread_name;
    std::string m_thread_pool_name;
    size_t m_thread_pool_thread_num;
};


S_NAMESPACE_END
#endif //S_SOCKET_API_TYPE_H_


