#include "socketApi.h"
S_NAMESPACE_BEGIN



// SocketUtil ------------------------------------------------------------------------
bool SocketUtil::bindAndListen(socket_t s, const std::string& svr_ip_or_name, int svr_port)
{
	struct sockaddr* addr = nullptr;
	int addr_len = 0;
	struct sockaddr_in addr_v4;
	struct sockaddr_in6 addr_v6;
	if (!__getIpByNameAndIinitAddr(svr_ip_or_name, svr_port, &addr_v4, &addr_v6, &addr, &addr_len))
	{
		slog_e("SocketUtil::bindAndListen fail __getIpByNameAndIinitAddr");
		return false;
	}

	int enable = 1;
	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char*)&enable, sizeof(enable)) != 0)
	{
		slog_e("SocketUtil::bindAndListen fail to setsockopt SO_REUSEADDR! error code : %0", getErr());
		return false;
	}

	if (bind(s, addr, addr_len) < 0)
	{
		slog_e("SocketUtil::bindAndListen fail to bind! error code : %0", getErr());
		return false;
	}

	if (listen(s, 5) < 0)
	{
		slog_e("SocketUtil::bindAndListen fail to listen! error code : %0", getErr());
		return false;
	}

	return true;
}

bool SocketUtil::accept(socket_t svr_accept_socket, socket_t* svr_trans_socket)
{
#ifdef S_OS_WIN
	*svr_trans_socket = INVALID_SOCKET;
#else
	*svr_trans_socket = 0;
#endif // S_OS_WIN

	char remote_ip_str[16];
	memset(remote_ip_str, 0, 16);
	uint16_t remote_port;

	struct sockaddr_in from_addr;
	memset(&from_addr, 0, sizeof(struct sockaddr_in));
	socklen_t addr_len = sizeof(sockaddr_in);

	*svr_trans_socket = ::accept(svr_accept_socket, (struct sockaddr*)&from_addr, &addr_len);
	if (*svr_trans_socket < 0)
	{
		slog_e("SocketUtil::accept fail to accept");
		return false;
	}
	char* from_ip_str = inet_ntoa(from_addr.sin_addr);
	memcpy(remote_ip_str, from_ip_str, strlen(from_ip_str));
	remote_port = ntohs(from_addr.sin_port);
	return true;
}

bool SocketUtil::connect(socket_t client_socket, const std::string& svr_ip_or_name, int svr_port)
{
	struct sockaddr* addr = nullptr;
	int addr_len = 0;
	struct sockaddr_in addr_v4;
	struct sockaddr_in6 addr_v6;
	if (!__getIpByNameAndIinitAddr(svr_ip_or_name, svr_port, &addr_v4, &addr_v6, &addr, &addr_len))
	{
		slog_e("SocketUtil::connect fail __getIpByNameAndIinitAddr");
		return false;
	}

	if (::connect(client_socket, addr, addr_len) < 0)
	{
		int err_code = getErr();
#ifdef S_OS_WIN
		if(err_code == WSAEWOULDBLOCK)
#else
		if (err_code == EINPROGRESS)
#endif // S_OS_WIN
			return true;

		slog_e("SocketUtil::connect fail to connect, ip=%0, port=%1!", svr_ip_or_name.c_str(), svr_port);
		return false;
	}
	return true;
}

bool SocketUtil::send(socket_t s, const byte_t* data, size_t data_len)
{
	while (true)
	{
		size_t real_send = 0;
		bool isOk = send(s, data, data_len, &real_send);
		if (!isOk)
			return false;
		if (real_send >= data_len) {
			return true;
		}

		data += real_send;
		data_len -= real_send;
	}
	return true;
}

bool SocketUtil::getIpByName(const char* name, std::vector<Ip>* ips)
{
	if (ips == nullptr)
		return false;
	ips->clear();

	if (name == nullptr)
		return false;

	Ip ip;
	if (strToIp(name, &ip))
	{
		ips->push_back(ip);
		return true;
	}

	addrinfo addr_hint;
	memset(&addr_hint, 0, sizeof(addr_hint));
	addr_hint.ai_family = AF_INET;
	addr_hint.ai_socktype = SOCK_STREAM;

	addrinfo* addr_result = NULL;
	if (getaddrinfo(name, NULL, &addr_hint, &addr_result) != 0)
	{
		slog_e("SocketUtil::getIpByName fail to getaddrinfo %0", name);
		return false;
	}

	char ip_str[100];
	for (addrinfo* a = addr_result; a != NULL; a = a->ai_next)
	{
		if (a->ai_family == AF_INET)
		{
			in_addr ip_v4 = ((sockaddr_in*)a->ai_addr)->sin_addr;
			ips->push_back(Ip(ip_v4));

			if (inet_ntop(AF_INET, &ip_v4, ip_str, sizeof(ip_str)) != nullptr)
				slog_d("%0 ip=%1", name, ip_str);
		}
		else if (a->ai_family == AF_INET6)
		{
			in6_addr ip_v6 = ((sockaddr_in6*)a->ai_addr)->sin6_addr;
			ips->push_back(Ip(ip_v6));

			if (inet_ntop(AF_INET6, &ip_v6, ip_str, sizeof(ip_str)) != nullptr)
				slog_d("%0 ip=%1", name, ip_str);
		}
	}

	return true;
}

bool SocketUtil::ipToStr(Ip ip, std::string * ip_str)
{
	if (ip.m_type == EIpType_v4)
		return ipv4ToStr(ip.m_value.m_v4, ip_str);
	else
		return ipv6ToStr(ip.m_value.m_v6, ip_str);
}

bool SocketUtil::ipv4ToStr(in_addr ip_v4, std::string* ip_str)
{
	if (ip_str == nullptr)
		return false;
	*ip_str = "";

	char buf[100];
	if (inet_ntop(AF_INET, &ip_v4, buf, 100) == NULL)
		return false;

	*ip_str = buf;
	return true;
}

bool SocketUtil::ipv6ToStr(in6_addr ip_v6, std::string * ip_str)
{
	if (ip_str == nullptr)
		return false;
	*ip_str = "";

	char buf[100];
	if (inet_ntop(AF_INET6, &ip_v6, buf, 100) == NULL)
		return false;

	*ip_str = buf;
	return true;
}

bool SocketUtil::strToIp(const std::string & ip_str, Ip * ip)
{ 
	if (strToIpv4(ip_str, &ip->m_value.m_v4))
	{
		ip->m_type = EIpType_v4;
		return true;
	}

	if (strToIpv6(ip_str, &ip->m_value.m_v6))
	{
		ip->m_type = EIpType_v6;
		return true;
	}

	return false;
}

bool SocketUtil::strToIpv4(const std::string& ip_str, in_addr* ip_v4)
{
	return inet_pton(AF_INET, ip_str.c_str(), ip_v4) == 1;
}

bool SocketUtil::strToIpv6(const std::string & ip_str, in6_addr * ip_v6)
{
	return inet_pton(AF_INET6, ip_str.c_str(), ip_v6) == 1;
}

bool SocketUtil::isValidSocketId(socket_id_t s)
{
	return s > 0;
}

uint16_t SocketUtil::hToNs(uint16_t s)
{
	return htons(s);
}

uint32_t SocketUtil::hToNl(uint32_t l)
{
	return htonl(l);
}

uint16_t SocketUtil::nToHs(uint16_t s)
{
	return ntohs(s);
}

uint32_t SocketUtil::nToHl(uint32_t l)
{
	return ntohl(l);
}

bool SocketUtil::__getIpByNameAndIinitAddr(const std::string & name, int port, sockaddr_in * addr_v4, sockaddr_in6 * addr_v6, sockaddr ** addr, int * addr_len)
{
	// get ip
	std::vector<Ip> ips;
	if (!SocketUtil::getIpByName(name.c_str(), &ips))
	{
		slog_e("SocketUtil::__getIpByNameAndIinitAddr fail to getIpByName.");
		return false;
	}
	if (ips.size() == 0 || ips[0].m_type == EIpType_none)
	{
		slog_e("SocketUtil::__getIpByNameAndIinitAddr fail, ips.size() == 0 || ips[0].m_type == EIpType_none.");
		return false;
	}
	const Ip& ip = ips[0];

	// init addr
	if (ip.m_type == EIpType_v4)
	{
		SocketUtil::__initAddrV4(addr_v4, ip.m_value.m_v4, port);
		*addr = (sockaddr*)addr_v4;
		*addr_len = sizeof(sockaddr_in);
	}
	else
	{
		SocketUtil::__initAddrV6(addr_v6, ip.m_value.m_v6, port);
		*addr = (sockaddr*)addr_v6;
		*addr_len = sizeof(sockaddr_in6);
	}
	return true;
}

void SocketUtil::__initAddrV4(struct sockaddr_in* addr, in_addr ip, int port)
{
	memset(addr, 0, sizeof(sockaddr_in));
	addr->sin_family = AF_INET;
	addr->sin_addr = ip;
	addr->sin_port = htons((short)port);
}

void SocketUtil::__initAddrV6(sockaddr_in6 * addr, in6_addr ip, int port)
{
	memset(addr, 0, sizeof(sockaddr_in6));
	addr->sin6_family = AF_INET;
	addr->sin6_addr = ip;
	addr->sin6_port = htons((short)port);
}








// TcpSocketBlockApi ------------------------------------------------------------------------
ITcpSocketBlockApi* ITcpSocketBlockApi::newBlockApi()
{
	return new TcpSocketBlockApi();
}

TcpSocketBlockApi::TcpSocketBlockApi()
{
	m_sid_seed = 0;
}

TcpSocketBlockApi::~TcpSocketBlockApi()
{
	for (SidToSocketMap::iterator it = m_sid_2_socket.begin(); it != m_sid_2_socket.end(); ++it)
	{
		SocketUtil::closeSocket(it->second);
	}
}

bool TcpSocketBlockApi::openSocket(socket_id_t* sid)
{
	socket_t s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s == INVALID_SOCKET)
		return false;

	ScopeMutex __l(m_mutex);
	*sid = ++m_sid_seed;
	m_sid_2_socket[*sid] = s;
	return true;
}

void TcpSocketBlockApi::closeSocket(socket_id_t sid)
{
	if (sid <= 0)
		return;

	ScopeMutex __l(m_mutex);
	auto it = m_sid_2_socket.find(sid);
	if (it == m_sid_2_socket.end())
		return;

	socket_t s = it->second;
	m_sid_2_socket.erase(it);
	SocketUtil::closeSocket(s);
}

bool TcpSocketBlockApi::bindAndListen(socket_id_t svr_listen_sid, const std::string& svr_ip_or_name, int svr_port)
{
	if (svr_listen_sid <= 0)
	{
		printf("sid <= 0\n");
		return false;
	}

	socket_t svr_listen_socket = INVALID_SOCKET;
	{
		ScopeMutex __l(m_mutex);
		if (!__getSocketBySid(svr_listen_sid, &svr_listen_socket))
		{
			printf("!__getSocketBySid\n");
			return false;
		}
	}

	if (!SocketUtil::bindAndListen(svr_listen_socket, svr_ip_or_name, svr_port))
	{
		printf("!SocketUtil::bindAndListen\n");
		return false;
	}

	return true;
}

bool TcpSocketBlockApi::accept(socket_id_t svr_listen_sid, socket_id_t* svr_tran_sid)
{
	if (svr_listen_sid <= 0 || svr_tran_sid == NULL)
	{
		printf("svr_listen_sid <= 0 || svr_tran_sid == NULL\n");
		return false;
	}
	*svr_tran_sid = INVALID_SOCKET;

	socket_t svr_listen_socket = INVALID_SOCKET;
	{
		ScopeMutex __l(m_mutex);
		if (!__getSocketBySid(svr_listen_sid, &svr_listen_socket))
		{
			printf("!__getSocketBySid\n");
			return false;
		}
	}

	socket_t svr_tran_socket = INVALID_SOCKET;
	if (!SocketUtil::accept(svr_listen_socket, &svr_tran_socket))
	{
		printf("!SocketUtil::accept\n");
		return false;
	}

	*svr_tran_sid = ++m_sid_seed;
	m_sid_2_socket[*svr_tran_sid] = svr_tran_socket;

	return true;
}

bool TcpSocketBlockApi::connect(socket_id_t client_sid, const std::string& svr_ip_or_name, int svr_port)
{
	if (client_sid <= 0)
		return false;

	socket_t client_socket = 0;
	{
		ScopeMutex __l(m_mutex);
		if (!__getSocketBySid(client_sid, &client_socket))
		{
			printf("!__getSocketBySid\n");
			return false;
		}
	}

	bool ret = SocketUtil::connect(client_socket, svr_ip_or_name, svr_port);
	return ret;
}

void TcpSocketBlockApi::disconnect(socket_id_t client_sid)
{
	socket_t client_socket = INVALID_SOCKET;
	{
		ScopeMutex __l(m_mutex);
		if (!__getSocketBySid(client_sid, &client_socket))
			return;
	}

#ifdef S_OS_WIN
	shutdown(client_socket, SD_BOTH);
#else
	shutdown(client_socket, SHUT_RDWR);
#endif // S_OS_WIN

}

bool TcpSocketBlockApi::send(socket_id_t sid, const byte_t* data, size_t data_len)
{
	if (sid == 0 || data == NULL || data_len == 0)
		return false;

	socket_t s = 0;
	{
		ScopeMutex __l(m_mutex);
		if (!__getSocketBySid(sid, &s))
			return false;
	}


	while (true)
	{
	    size_t real_send = 0;
	    bool is_ok = SocketUtil::send(s, data, data_len, &real_send);
	    if (!is_ok)
	        return false;

	    if (real_send >= data_len)
	        return true;
	    
	    data_len -= real_send;
	}
	return true;
}

bool TcpSocketBlockApi::recv(socket_id_t sid, byte_t* buf, size_t buf_len, size_t* recv_len)
{
	*recv_len = 0;
	socket_t s = 0;
	{
		ScopeMutex __l(m_mutex);
		if (!__getSocketBySid(sid, &s))
			return false;
	}

	int64_t ret = ::recv(s, (char*)buf, (int)buf_len, 0);
	if (ret > 0)
	{
		*recv_len = ret;
		return true;
	}
	else if (ret == 0)
	{
		printf("recv disconnected\n");
		return false;
	}
	else
	{
		return false;
	}
}

bool TcpSocketBlockApi::__getSocketBySid(socket_id_t sid, socket_t* s)
{
	auto it = m_sid_2_socket.find(sid);
	if (it == m_sid_2_socket.end())
		return false;

	*s = it->second;
	return true;
}




S_NAMESPACE_END

