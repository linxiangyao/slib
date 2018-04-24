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
	#define INVALID_SOCKET -1
#endif


typedef SOCKET socket_t;
typedef int64_t socket_id_t;


enum ETcpSocketType
{
    ETcpSocketType_client,
    ETcpSocketType_svr_listen,
    ETcpSocketType_svr_tran,
};


enum EIpType
{
	EIpType_none,
	EIpType_v4,
	EIpType_v6,
};


class Ip
{
public:
	Ip() { m_type = EIpType_none; }
	Ip(in_addr v4) { m_type = EIpType_v4; m_value.m_v4 = v4; }
	Ip(in6_addr v6) { m_type = EIpType_v6; m_value.m_v6 = v6; }

	EIpType m_type;
	union
	{
		in_addr m_v4;
		in6_addr m_v6;
	} m_value;
};







S_NAMESPACE_END
#endif //S_SOCKET_API_TYPE_H_


