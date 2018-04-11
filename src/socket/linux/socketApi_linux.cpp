#include "../../comm/comm.h"
//#define S_OS_LINUX
#if defined(S_OS_LINUX) | defined(S_OS_MAC) | defined(S_OS_ANDROID)
#include "socketApi_linux.h"
S_NAMESPACE_BEGIN




// public static funtion ------------------------------------------------------------------------
bool initSocketLib()
{
    return true;
}

void releaseSocketLib()
{
}




// inner static funtion ------------------------------------------------------------------------
static bool __bindAndListen(int s_svr, const std::string& svr_ip_or_name, int svr_port)
{
    uint32_t svrIp = 0;
	if (!SocketUtil::getIpByName(svr_ip_or_name.c_str(), &svrIp))
	{
		printf("fail to getIpByName!\n");
		return false;
	}
    
    struct sockaddr_in server_addr;
    SocketUtil::initAddr(&server_addr, svrIp, svr_port);
    
    int enable = 1;
    if (setsockopt(s_svr, SOL_SOCKET, SO_REUSEADDR, (char*)&enable, sizeof(enable)) != 0)
    {
        printf("bindAndListen setsockopt SO_REUSEADDR!\n");
        return false;
    }
    
    if (bind(s_svr, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in)) < 0)
    {
		printf("fail to bind!\n");
        return false;
    }
    
    if (listen(s_svr, 5) < 0)
    {
		printf("fail to listen!\n");
        return false;
    }
    
    return true;
}

static bool __accept(int svr_accept_socket, int* svr_trans_socket)
{
    char remote_ip_str[16];
    memset(remote_ip_str, 0, 16);
    uint16_t remote_port;
    
    struct sockaddr_in from_addr;
    memset(&from_addr, 0, sizeof(struct sockaddr_in));
    socklen_t addr_len = sizeof(sockaddr_in);
    
    *svr_trans_socket = ::accept(svr_accept_socket, (struct sockaddr*)&from_addr, &addr_len);
    if (*svr_trans_socket < 0)
    {
        return false;
    }
    char* from_ip_str = inet_ntoa(from_addr.sin_addr);
    memcpy(remote_ip_str, from_ip_str, strlen(from_ip_str));
    remote_port = ntohs(from_addr.sin_port);
    return true;
}

static bool __changeSocketToAsync(int s)
{
	int flags = fcntl(s, F_GETFL, 0);
	if (fcntl(s, F_SETFL, flags | O_NONBLOCK) < 0)
		return false;
	return true;
}

static bool __isSocketOk(int s)
{
	int err = 0;
	socklen_t err_len = sizeof(err);
	if (getsockopt(s, SOL_SOCKET, SO_ERROR, &err, &err_len) == -1)
		return false;
	if (err != 0)
		return false;
	return true;
}

static int s_max(int l, int r)
{
	return l > r ? l : r;
}




// socketUtil ------------------------------------------------------------------------
void SocketUtil::initAddr(struct sockaddr_in* addr, uint32_t ip, int port)
{
	memset(addr, 0, sizeof(struct sockaddr_in));
	addr->sin_family = AF_INET;
	addr->sin_addr.s_addr = ip;
	addr->sin_port = htons((short)port);
}

bool SocketUtil::connect(SOCKET client_socket, const std::string& svr_ip_or_name, int svr_port)
{
	uint32_t svr_ip = 0;
	if (!SocketUtil::getIpByName(svr_ip_or_name.c_str(), &svr_ip))
	{
		printf("fail to getIpByName!\n");
		return false;
	}

	struct sockaddr_in server_address;
	SocketUtil::initAddr(&server_address, svr_ip, svr_port);

	if (::connect(client_socket, (sockaddr*)&server_address, sizeof(server_address)) < 0)
	{
		if (errno == EINPROGRESS)
			return true;
		printf("fail to connect, ip=%s, port=%d!\n", svr_ip_or_name.c_str(), svr_port);
		return false;
	}
	return true;
}

bool SocketUtil::recv(SOCKET s, byte_t* buf, size_t buf_len, size_t* recv_len)
{
	*recv_len = 0;
	ssize_t ret = ::recv(s, (char*)buf, (int)buf_len, 0);
	if (ret < 0)
	{
		if (ret == EAGAIN)
			return true;
		return false;
	}
	else if (ret == 0)
	{
		return false;
	}

	*recv_len = ret;
	return true;
}

bool SocketUtil::send(SOCKET s, const byte_t* data, size_t data_len, size_t* real_send)
{
	ssize_t ret = ::send(s, (const char*)data, (int)data_len, 0);

	if (ret <= 0)
	{
		*real_send = 0;
		return false;
	}
	*real_send = ret;
	return true;
}

bool SocketUtil::send(SOCKET s, const byte_t* data, size_t data_len)
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

bool SocketUtil::getIpByName(const char* name, uint32_t* ip)
{
	if (ipToUint32(name, ip))
	{
		return true;
	}

	struct hostent* host = gethostbyname(name);
	if (host == NULL)
	{
		//        printf("gethostbyname error\n");
		return false;
	}

	// printf("official name: %s\n\n", host->h_name);
	//  printf("address length: %d bytes\n\n", host->h_length);


	//printf("host name alias: \n");
	//for (int i = 0; host->h_aliases[i]; i++) {
	//    printf("%s\n", host->h_aliases[i]);
	//}
	//
	//printf("\naddress list: \n");
	if (host->h_addr_list[0] == NULL)
		return false;

	*ip = ((struct in_addr *)host->h_addr_list[0])->s_addr;

	//for (int i = 0; host->h_addr_list[i]; i++) {
	//    struct in_addr * p = (struct in_addr *)host->h_addr_list[i];
	//    struct in_addr  a = *p;
	//    const char *ip = inet_ntoa(a);
	//    printf("\nip: %s  \n", ip);
	//}

	return true;
}

bool SocketUtil::ipToStr(uint32_t ip, std::string* ipStr)
{
	const char * sz = inet_ntoa(*((struct in_addr *)&ip));
	if (sz == NULL)
		return false;

	*ipStr = sz;
	return true;
}

bool SocketUtil::ipToUint32(const std::string& ipStr, uint32_t* ip)
{
	*ip = inet_addr(ipStr.c_str());
	if (INADDR_NONE == *ip)
		return false;
	return true;
}

bool SocketUtil::isValidSocket(socket_id_t s)
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
	for(SidToSocketMap::iterator it = m_sid_2_socket.begin(); it != m_sid_2_socket.end(); ++it)
	{
		close(it->second);
	}
}

bool TcpSocketBlockApi::openSocket(socket_id_t* sid)
{
    int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == -1)
        return false;

	ScopeMutex __l(m_mutex);
	*sid = ++m_sid_seed;
	m_sid_2_socket[*sid] = s;
    return true;
}

void TcpSocketBlockApi::closeSocket(socket_id_t sid)
{
    if(sid <= 0)
        return;
    
    ScopeMutex __l(m_mutex);
    std::map<int64_t, int>::iterator it = m_sid_2_socket.find(sid);
    if(it == m_sid_2_socket.end())
        return;
    
    int s = it->second;
    m_sid_2_socket.erase(it);
    close(s);
}

bool TcpSocketBlockApi::bindAndListen(socket_id_t svr_listen_sid, const std::string& svr_ip_or_name, int svr_port)
{
	if (svr_listen_sid <= 0)
	{
		printf("sid <= 0\n");
		return false;
	}

    int svr_listen_socket = 0;
	{
		ScopeMutex __l(m_mutex);
		if (!__getSocketBySid(svr_listen_sid, &svr_listen_socket))
		{
			printf("!__getSocketBySid\n");
			return false;
		}
	}
    
    if(!__bindAndListen(svr_listen_socket, svr_ip_or_name, svr_port))
	{
		printf("!__bindAndListen\n");
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
    *svr_tran_sid = 0;

    int svr_listen_socket = 0;
	{
		ScopeMutex __l(m_mutex);
		if (!__getSocketBySid(svr_listen_sid, &svr_listen_socket))
		{
			printf("!__getSocketBySid\n");
			return false;
		}
	}
    
    int svr_tran_socket = 0;
    if(!__accept(svr_listen_socket, &svr_tran_socket))
	{
		printf("!__accept\n");
		return false;
	}

	*svr_tran_sid = ++m_sid_seed;
	m_sid_2_socket[*svr_tran_sid] = svr_tran_socket;

    return true;
}

bool TcpSocketBlockApi::connect(socket_id_t client_sid, const std::string& svr_ip_or_name, int svr_port)
{
    if(client_sid <= 0)
        return false;

	int client_socket = 0;
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
    int client_socket = 0;
	{
		ScopeMutex __l(m_mutex);
		if (!__getSocketBySid(client_sid, &client_socket))
			return;
	}
    shutdown(client_socket, SHUT_RDWR);
}

bool TcpSocketBlockApi::send(socket_id_t sid, const byte_t* data, size_t data_len)
{
    if(sid == 0 || data == NULL || data_len == 0)
        return false;
    
    int s = 0;
	{
		ScopeMutex __l(m_mutex);
		if (!__getSocketBySid(sid, &s))
			return false;
	}

    while (true)
    {
        int64_t realSend = ::send(s, (const char*)data, (int)data_len, 0);
        if (realSend < 0)
            return false;

        if (realSend >= data_len)
            return true;

        data_len -= realSend;
    }

    return true;
}

bool TcpSocketBlockApi::recv(socket_id_t sid, byte_t* buf, size_t buf_len, size_t* recv_len)
{
    *recv_len = 0;
    int s = 0;
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



bool TcpSocketBlockApi::__getSocketBySid(int64_t sid, int* s)
{
    std::map<int64_t, int>::iterator it = m_sid_2_socket.find(sid);
    if(it == m_sid_2_socket.end())
        return false;

    *s = it->second;
    return true;
}










// TcpSocketCallbackApi --------------------------------------------------------------------------------------
// __ClientThreadRun --
class __ClientThreadRun : public IThreadRun
{
public:
	enum EMsgType
	{
		EMsgType_clientSocketConnected,
		EMsgType_clientSocketClosed,
		EMsgType_clientSocketSendDataEnd,
		EMsgType_clientSocketRecvData,
	};

	__ClientThreadRun(MessageLooper* notify_looper, void* notify_target)
	{
		m_notify_looper = notify_looper;
		m_notify_target = notify_target;
		m_is_exit = false;
		m_total_to_send_pack_count = 0;
		m_total_sent_pack_count = 0;
		m_pipe[0] = 0;
		m_pipe[1] = 0;
	}

	~__ClientThreadRun()
	{

	}

	void run()
	{
		sscope_d();
		__onRun();
		ScopeMutex __l(m_mutex);
		__release();
	}

	void stop()
	{
		ScopeMutex __l(m_mutex);
		if (m_is_exit)
			return;
		m_is_exit = true;
		__wakeup();
	}

	void postMsg_cmdConnectSvr(socket_t s, const std::string& svr_ip, uint32_t svr_port)
	{
		ScopeMutex __l(m_mutex);
		if (m_is_exit)
			return;
		bool is_need_wakup = m_msgs.size() == 0 && m_pipe[0] != 0;

		__Msg_cmd_connect* msg = new __Msg_cmd_connect();
		msg->m_socket = s;
		msg->m_svr_ip_or_name = svr_ip;
		msg->m_svr_port = svr_port;
		m_msgs.push_back(msg);

		if (is_need_wakup)
			__wakeup();
	}

	void postMsg_cmdDisconnectSvr(socket_t s)
	{
		ScopeMutex __l(m_mutex);
		if (m_is_exit)
			return;
		bool is_need_wakup = m_msgs.size() == 0 && m_pipe[0] != 0;

		__Msg_cmd_disconnect* msg = new __Msg_cmd_disconnect();
		msg->m_socket = s;
		m_msgs.push_back(msg);

		if (is_need_wakup)
			__wakeup();
	}

	void postMsg_cmdSendDataToSvr(socket_t s, const byte_t* data, size_t data_len)
	{
		ScopeMutex __l(m_mutex);
		if (m_is_exit)
			return;
		bool is_need_wakup = m_msgs.size() == 0 && m_pipe[0] != 0;

		Binary* bin = new Binary();
		bin->append(data, data_len);
		__Msg_cmd_sendData* msg = new __Msg_cmd_sendData();
		msg->m_socket = s;
		msg->m_data = bin;
		m_msgs.push_back(msg);

		if (is_need_wakup)
			__wakeup();
	}


private:
	enum __ESocketConnectState
	{
		__ESocketConnectState_none,
		__ESocketConnectState_connecting,
		__ESocketConnectState_connected,
		//__ESocketConnectState_disconnected,
	};

	class __SocketCtx
	{
	public:
		__SocketCtx() { m_sid = 0; m_socket = INVALID_SOCKET; m_connet_state = __ESocketConnectState_none; m_total_to_send_pack_count = 0; }
		~__SocketCtx() { if (m_socket != INVALID_SOCKET) ::close(m_socket); delete_and_erase_collection_elements(&m_send_datas); }
		socket_id_t m_sid;
		int m_socket;
		__ESocketConnectState m_connet_state;
		std::list<Binary*> m_send_datas;
		size_t m_total_to_send_pack_count;
	};

	enum __EMsgType
	{
		__EMsgType_cmd_connect,
		__EMsgType_cmd_disconnect,
		__EMsgType_cmd_sendData,
	};

	class __Msg
	{
	public:
		__Msg() {}
		virtual ~__Msg() {}

		__EMsgType m_msg_type;
		int m_socket;
	};

	class __Msg_cmd_connect : public __Msg
	{
	public:
		__Msg_cmd_connect() { m_msg_type = __EMsgType_cmd_connect; m_svr_port = 0; }
		~__Msg_cmd_connect() {}
		std::string m_svr_ip_or_name;
		uint32_t m_svr_port;
	};

	class __Msg_cmd_disconnect : public __Msg
	{
	public:
		__Msg_cmd_disconnect() { m_msg_type = __EMsgType_cmd_disconnect; }
		~__Msg_cmd_disconnect() {}
	};

	class __Msg_cmd_sendData : public __Msg
	{
	public:
		__Msg_cmd_sendData() { m_msg_type = __EMsgType_cmd_sendData; m_data = NULL; }
		~__Msg_cmd_sendData() { delete m_data; }
		Binary* m_data;
	};

	typedef std::list<__Msg*> __MsgList;



	void __onRun()
	{
		if (pipe(m_pipe) != 0)
		{
			slog_e("fail to pipe");
			return;
		}

		while (!m_is_exit)
		{
			// process msg -----
			__onProcessMsgs();
		
			// make fd -----
			int maxFd = 0;
			fd_set rdSet;
			FD_ZERO(&rdSet);
			fd_set wdSet;
			FD_ZERO(&wdSet);
			fd_set errSet;
			FD_ZERO(&errSet);

			FD_SET(m_pipe[0], &rdSet);
			maxFd = m_pipe[0];

			for (size_t i = 0; i < m_socket_ctxs.size(); ++i)
			{
				__SocketCtx* ctx = m_socket_ctxs[i];
				if (ctx->m_connet_state == __ESocketConnectState_connecting)
				{
					FD_SET(ctx->m_socket, &rdSet);
					FD_SET(ctx->m_socket, &wdSet);
					maxFd = s_max(maxFd, ctx->m_socket);
				}
				else if (ctx->m_connet_state == __ESocketConnectState_connected)
				{
					FD_SET(ctx->m_socket, &rdSet);
					if (ctx->m_send_datas.size() > 0)
					{
						FD_SET(ctx->m_socket, &wdSet);
					}
					maxFd = s_max(maxFd, ctx->m_socket);
				}
			}

			// select -----
			int res = select(maxFd + 1, &rdSet, &wdSet, &errSet, NULL);

			if (m_is_exit)
				return;

			// process msg -----
			__onProcessMsgs();
			

			// check fd ----
			if (res < 0)
			{
				slog_e("fail to select, err=%0", res);
				for (size_t i = 0; i < m_socket_ctxs.size(); ++i)
				{
					__SocketCtx* ctx = m_socket_ctxs[i];
					bool is_err_set = FD_ISSET(ctx->m_socket, &errSet);
					if (is_err_set)
					{
						__releaseSocketCtx(ctx);
						--i;
					}
				}
				continue;
			}

			if (FD_ISSET(m_pipe[0], &rdSet))
			{
				char b;
				::read(m_pipe[0], &b, 1);
				slog_v("recv signal");
			}


			for (size_t i = 0; i < m_socket_ctxs.size(); ++i)
			{
				__SocketCtx* ctx = m_socket_ctxs[i];
				bool is_wd_set = FD_ISSET(ctx->m_socket, &wdSet);
				bool is_rd_set = FD_ISSET(ctx->m_socket, &rdSet);
				if (!is_wd_set && !is_rd_set)
					continue;

				bool is_need_release = false;
				__onCtxFdSet(ctx, is_wd_set, is_rd_set, &is_need_release);
				if (is_need_release)
				{
					__releaseSocketCtx(ctx);
					--i;
				}
			}
		}
	}

	void __onProcessMsgs()
	{
		__MsgList msgs;
		{
			ScopeMutex __l(m_mutex);
			for (__MsgList::iterator it = m_msgs.begin(); it != m_msgs.end(); ++it)
			{
				msgs.push_back(*it);
			}
			m_msgs.clear();
		}

		for (__MsgList::iterator it = msgs.begin(); it != msgs.end(); ++it)
		{
			if ((*it)->m_msg_type == __EMsgType_cmd_connect)
			{
				__Msg_cmd_connect* msg = (__Msg_cmd_connect*)(*it);
				int index = __getSocketCtxIndexBySocket(msg->m_socket);
				if (index >= 0)
				{
					slog_w("a socket connect twice, SocketCallbackApi will not notify connected msg. this maybe make something err");
					continue;
				}

				__SocketCtx* ctx = new __SocketCtx();
				ctx->m_socket = msg->m_socket;
				m_socket_ctxs.push_back(ctx);

				if (!__changeSocketToAsync(ctx->m_socket))
				{
					slog_e("fail to __changeSocketToAsync");
					__releaseSocketCtx(ctx);
					continue;
				}

				slog_d("connect socket=%0, svrIp=%1, svrPort=%2", ctx->m_socket, msg->m_svr_ip_or_name, msg->m_svr_port);
				if (!SocketUtil::connect(ctx->m_socket, msg->m_svr_ip_or_name, msg->m_svr_port))
				{
					slog_e("fail to connect");
					__releaseSocketCtx(ctx);
					continue;
				}

				ctx->m_connet_state = __ESocketConnectState_connecting;
			}
			else if ((*it)->m_msg_type == __EMsgType_cmd_disconnect)
			{
				__Msg_cmd_disconnect* msg = (__Msg_cmd_disconnect*)(*it);
				int index = __getSocketCtxIndexBySocket(msg->m_socket);
				if (index < 0)
					continue;

				__SocketCtx* ctx = m_socket_ctxs[index];
				__releaseSocketCtx(ctx);
			}
			else
			{
				++m_total_to_send_pack_count;
				//slog_v("total_to_send=%0", m_total_to_send_pack_count);

				__Msg_cmd_sendData* msg = (__Msg_cmd_sendData*)(*it);
				int index = __getSocketCtxIndexBySocket(msg->m_socket);
				if (index < 0)
					continue;
					
				__SocketCtx* ctx = m_socket_ctxs[index];
				Binary* data = new Binary();
				data->attach(msg->m_data);
				ctx->m_send_datas.push_back(data);
				ctx->m_total_to_send_pack_count++;
			}
		}
		delete_and_erase_collection_elements(&msgs);
	}

	void __onCtxFdSet(__SocketCtx* ctx, bool is_wd_set, bool is_rd_set, bool* is_need_release)
	{
		if (ctx->m_connet_state == __ESocketConnectState_connecting)
		{
			bool is_connected = is_wd_set && __isSocketOk(ctx->m_socket);
			slog_v("is_connected=%0", is_connected);

			if (!is_connected)
			{
				*is_need_release = true;
				return;
			}

			ctx->m_connet_state = __ESocketConnectState_connected;
			__notifyMsg_clientSocketConnected(*ctx);
		}
		else if (ctx->m_connet_state == __ESocketConnectState_connected)
		{
			if (is_rd_set)
			{
				//slog_v("socket can read");
				byte_t buf[64 * 1024];
				while (true)
				{
					size_t recv_len = 0;
					if (!SocketUtil::recv(ctx->m_socket, buf, 64 * 1024, &recv_len))
					{
						slog_d("fail to recv, maybe socket closed, socket=%0", ctx->m_socket);
						*is_need_release = true;
						return;
					}

					if (recv_len > 0)
					{
						Binary bin;
						bin.copy(buf, recv_len);
						__notifyMsg_clientSocketRecvData(*ctx, bin.getData(), bin.getLen());
						bin.detach();
					}
					if (recv_len != 64 * 1024)
						break;
				}
			}

			if (is_wd_set)
			{
				//slog_v("socket can write");
				while (ctx->m_send_datas.size() > 0)
				{
					Binary* data = *(ctx->m_send_datas.begin());
					size_t real_sent = 0;
					if (!SocketUtil::send(ctx->m_socket, data->getData(), data->getLen(), &real_sent))
					{
						slog_d("fail to send, maybe socket closed, socket=%0", ctx->m_socket);
						*is_need_release = true;
						return;
					}

					//slog_v("send len=%0", real_sent);
					if (real_sent == data->getLen())
					{
						delete data;
						ctx->m_send_datas.erase(ctx->m_send_datas.begin());
						__notifyMsg_clientSocketSendDataEnd(*ctx);
					}
					else
					{
						data->shrinkBegin(real_sent);
						break;
					}
				}
			}
		}
		else
		{
			slog_e("err");
		}
	}




	void __release()
	{
		m_is_exit = true;
		delete_and_erase_collection_elements(&m_socket_ctxs);
		delete_and_erase_collection_elements(&m_msgs);
		if (m_pipe[0] != 0)
		{
			close(m_pipe[0]);
			close(m_pipe[1]);
			m_pipe[0] = 0;
			m_pipe[1] = 0;
		}
	}

	void __releaseSocketCtx(__SocketCtx* ctx)
	{
		__notifyMsg_clientSocketClosed(*ctx);
		delete_and_erase_vector_element_by_value(&m_socket_ctxs, ctx);
	}

	void __wakeup()
	{
		write(m_pipe[1], "0", 1);		
	}
	
	int __getSocketCtxIndexBySocket(int s)
	{
		for (size_t i = 0; i < m_socket_ctxs.size(); ++i)
		{
			if (m_socket_ctxs[i]->m_socket == s)
				return (int)i;
		}
		return -1;
	}

	void __notifyMsg_clientSocketConnected(const __SocketCtx& ctx)
	{
		Message* m = __newNotifyMsg(ctx);
		m->m_msg_type = EMsgType_clientSocketConnected;
		m_notify_looper->postMessage(m);
	}

	void __notifyMsg_clientSocketClosed(const __SocketCtx& ctx)
	{
		Message* m = __newNotifyMsg(ctx);
		m->m_msg_type = EMsgType_clientSocketClosed;
		m_notify_looper->postMessage(m);
	}

	void __notifyMsg_clientSocketSendDataEnd(const __SocketCtx& ctx)
	{
		Message* m = __newNotifyMsg(ctx);
		m->m_msg_type = EMsgType_clientSocketSendDataEnd;
		m_notify_looper->postMessage(m);
		++m_total_sent_pack_count;
		
		//ScopeMutex __l(m_mutex);
		//if (m_total_to_send_pack_count != m_total_sent_pack_count + ctx.m_send_datas.size() + m_msgs.size())
		//{
		//	size_t send_data_msg_count = 0;
		//	for (__MsgList::iterator it = m_msgs.begin(); it != m_msgs.end(); ++it)
		//	{
		//		if ((*it)->m_msg_type == __EMsgType_cmd_sendData)
		//			++send_data_msg_count;
		//	}
		//	
		//	if (m_total_sent_pack_count != m_total_sent_pack_count + ctx.m_send_datas.size() + send_data_msg_count)
		//	{
		//		slog_i("total_to_send=%0, ctx.total_to_send=%1, total_sent=%2, ctx.to_send_pack=%3, send_data_msg=%4"
		//			, m_total_to_send_pack_count, ctx.m_total_to_send_pack_count, m_total_sent_pack_count, ctx.m_send_datas.size(), send_data_msg_count);
		//		exit(-1);
		//	}
		//}
	}

	void __notifyMsg_clientSocketRecvData(const __SocketCtx& ctx, byte_t* data, size_t data_len)
	{
		Message* m = __newNotifyMsg(ctx);
		m->m_msg_type = EMsgType_clientSocketRecvData;
		m->m_args.setByteArrayAndAttachFrom("data", data, data_len);
		m_notify_looper->postMessage(m);
	}

	Message* __newNotifyMsg(const __SocketCtx& ctx)
	{
		Message* m = new Message();
		m->m_sender = this;
		m->m_target = m_notify_target;
		m->m_args.set("client_socket", ctx.m_socket);
		return m;
	}


	MessageLooper* m_notify_looper;
	void* m_notify_target;
	Mutex m_mutex;
	bool m_is_exit;
	__MsgList m_msgs;
	std::vector<__SocketCtx*> m_socket_ctxs;
	int m_pipe[2];
	size_t m_total_sent_pack_count;
	size_t m_total_to_send_pack_count;
};




// __SvrThreadRun --
class __SvrThreadRun : public IThreadRun
{
public:
	enum EMsgType
	{
		EMsgType_svrListenSocketListened = 1973690161,
		EMsgType_svrListenSocketClosed = 1973690162,
		EMsgType_svrListenSocketAccepted = 1973690163,
		EMsgType_svrTranSocketClosed = 1973690164,
		EMsgType_svrTranSocketRecvData = 1973690165,
		EMsgType_svrTranSocketSendDataEnd = 1973690166,
	};

	__SvrThreadRun(MessageLooper* notify_looper, void* notify_target)
	{
		m_notify_looper = notify_looper;
		m_notify_target = notify_target;
		m_is_exit = false;
		m_pipe[0] = 0;
		m_pipe[1] = 0;
	}

	~__SvrThreadRun()
	{

	}

	void run()
	{
		sscope_d();
		__onRun();
		ScopeMutex __l(m_mutex);
		__release();
	}

	void stop()
	{
		ScopeMutex __l(m_mutex);
		if (m_is_exit)
			return;
		m_is_exit = true;
		__wakeup();
	}

	void postMsg_cmdStartListenSocket(int svr_listen_socket, std::string& svr_ip, uint32_t svr_port)
	{
		ScopeMutex __l(m_mutex);
		bool is_need_wakup = m_msgs.size() == 0 && m_pipe[0] != 0;

		__Msg* m = new __Msg();
		m->m_msg_type = __EMsgType_cmdStartListenSocket;
		m->m_socket = svr_listen_socket;
		m->m_svr_ip = svr_ip;
		m->m_svr_port = svr_port;
		m_msgs.push_back(m);

		if(is_need_wakup)
			__wakeup();
	}

	void postMsg_cmdStopListenSocket(int svr_listen_socket)
	{
		ScopeMutex __l(m_mutex);
		bool is_need_wakup = m_msgs.size() == 0 && m_pipe[0] != 0;

		__Msg* m = new __Msg();
		m->m_msg_type = __EMsgType_cmdStopListenSocket;
		m->m_socket = svr_listen_socket;
		m_msgs.push_back(m);

		if (is_need_wakup)
			__wakeup();
	}

	void postMsg_cmdSendDataToClient(int svr_tran_socket, const byte_t* data, size_t data_len)
	{
		ScopeMutex __l(m_mutex);
		bool is_need_wakup = m_msgs.size() == 0 && m_pipe[0] != 0;

		__Msg* m = new __Msg();
		m->m_msg_type = __EMsgType_cmdSendDataToClient;
		m->m_socket = svr_tran_socket;
		m->m_data.copy(data, data_len);
		m_msgs.push_back(m);

		if (is_need_wakup)
			__wakeup();
	}

	void postMsg_cmdStopTranSocket(int svr_tran_socket)
	{
		ScopeMutex __l(m_mutex);
		bool is_need_wakup = m_msgs.size() == 0 && m_pipe[0] != 0;

		__Msg* m = new __Msg();
		m->m_msg_type = __EMsgType_cmdStopTranSocket;
		m->m_socket = svr_tran_socket;
		m_msgs.push_back(m);

		if (is_need_wakup)
			__wakeup();
	}


private:
#define SOCKET_API_SVR_RECV_BUF_SIZE (127 * 1024)
	enum __EMsgType
	{
		__EMsgType_cmdStartListenSocket,
		__EMsgType_cmdStopListenSocket,
		__EMsgType_cmdSendDataToClient,
		__EMsgType_cmdStopTranSocket,
	};

	class __Msg
	{
	public:
		__EMsgType m_msg_type;
		int m_socket;
		std::string m_svr_ip;
		uint32_t m_svr_port;
		Binary m_data;
	};

	typedef std::list<__Msg*> __MsgList;

	class __SvrListenSocketCtx
	{
	public:
		~__SvrListenSocketCtx() { if (m_socket != INVALID_SOCKET) ::close(m_socket); }
		int m_socket;
	};

	class __SvrTranSocketCtx
	{
	public:
		~__SvrTranSocketCtx() { if (m_socket != INVALID_SOCKET) ::close(m_socket); delete_and_erase_collection_elements(&m_send_datas); }
		int m_ref_listen_socket;
		int m_socket;
		std::vector<Binary*> m_send_datas;
	};



	void __onRun()
	{
		if (pipe(m_pipe) != 0)
		{
			slog_e("fail to pipe");
			return;
		}

		while (!m_is_exit)
		{
			// process msg
			__onProcessMsgs();
			

			// make fd -----
			int maxFd = 0;
			fd_set rdSet;
			FD_ZERO(&rdSet);
			fd_set wdSet;
			FD_ZERO(&wdSet);

			FD_SET(m_pipe[0], &rdSet);
			maxFd = m_pipe[0];

			for (size_t i = 0; i < m_listen_ctxs.size(); ++i)
			{
				__SvrListenSocketCtx* ctx = m_listen_ctxs[i];
				FD_SET(ctx->m_socket, &rdSet);
				FD_SET(ctx->m_socket, &wdSet);
				maxFd = s_max(maxFd, ctx->m_socket);
			}

			for (size_t i = 0; i < m_tran_ctxs.size(); ++i)
			{
				__SvrTranSocketCtx* ctx = m_tran_ctxs[i];
				FD_SET(ctx->m_socket, &rdSet);
				if (ctx->m_send_datas.size() > 0)
				{
					FD_SET(ctx->m_socket, &wdSet);
				}
				maxFd = s_max(maxFd, ctx->m_socket);
			}

			// select -----
			int res = select(maxFd + 1, &rdSet, &wdSet, NULL, NULL);
			if (res < 0)
			{
				if (m_is_exit)
					return;
				slog_e("fail to select, err=%0", res);
				return;
			}


			// check fd ----
			if (FD_ISSET(m_pipe[0], &rdSet))
			{
				char b;
				::read(m_pipe[0], &b, 1);
				slog_v("recv signal");
				if (m_is_exit)
					break;
			}

			for (size_t i = 0; i < m_listen_ctxs.size(); ++i)
			{
				__SvrListenSocketCtx* ctx = m_listen_ctxs[i];
				bool is_wd_set = FD_ISSET(ctx->m_socket, &wdSet);
				bool is_rd_set = FD_ISSET(ctx->m_socket, &rdSet);
				if (!is_wd_set && !is_rd_set)
					continue;

				int tran_socket = INVALID_SOCKET;
				if (!__accept(ctx->m_socket, &tran_socket))
				{
					slog_e("fail to accept");
					__releaseListenCtx(ctx);
					--i;
				}

				slog_v("accept socket");
				__SvrTranSocketCtx* tran_ctx = new __SvrTranSocketCtx();
				tran_ctx->m_socket = tran_socket;
				tran_ctx->m_ref_listen_socket = ctx->m_socket;
				m_tran_ctxs.push_back(tran_ctx);
				__notifyMsg_svrListenSocketAccepted(*tran_ctx);
			}

			for (size_t i = 0; i < m_tran_ctxs.size(); ++i)
			{
				__SvrTranSocketCtx* ctx = m_tran_ctxs[i];
				bool is_rd_set = FD_ISSET(ctx->m_socket, &rdSet);
				bool is_wd_set = FD_ISSET(ctx->m_socket, &wdSet);
				if (!is_wd_set && !is_rd_set)
					continue;

				if (!__onTranCtxFdSet(ctx, is_rd_set, is_wd_set))
				{
					__releaseTranCtx(ctx);
					--i;
				}
			}
		}
	}

	bool __onTranCtxFdSet(__SvrTranSocketCtx* ctx, bool is_rd_set, bool is_wd_set)
	{
		if (is_rd_set)
		{
			size_t once_recv_send_msg_count = 0;

			while (true)
			{
				size_t recv_len = 0;
				if (!SocketUtil::recv(ctx->m_socket, m_recv_buf, SOCKET_API_SVR_RECV_BUF_SIZE, &recv_len))
				{
					slog_d("fail to recv, maybe socket closed, socket=%0", ctx->m_socket);
					return false;
				}

				if (recv_len > 0)
				{
					++once_recv_send_msg_count;
					Binary bin;
					bin.copy(m_recv_buf, recv_len);
					__notifyMsg_svrTranSocketRecvData(*ctx, bin.getData(), bin.getLen());
					bin.detach();
				}
				if (recv_len != SOCKET_API_SVR_RECV_BUF_SIZE)
					break;
			}
		}

		if (is_wd_set)
		{
			//slog_v("tran socket can write");
			while (ctx->m_send_datas.size() > 0)
			{
				Binary* data = ctx->m_send_datas[0];
				size_t real_sent = 0;
				if (!SocketUtil::send(ctx->m_socket, data->getData(), data->getLen(), &real_sent))
				{
					slog_d("fail to send, maybe socket closed, socket=%0", ctx->m_socket);
					return false;
				}
				slog_v("socket=%0, send len=%1", ctx->m_socket, real_sent);

				if (real_sent == data->getLen())
				{
					delete_and_erase_vector_element_by_index(&ctx->m_send_datas, 0);
					__notifyMsg_svrTranSocketSendDataEnd(*ctx);
				}
				else
				{
					data->shrinkBegin(real_sent);
					break;
				}
			}
		}

		return true;
	}

	void __onProcessMsgs()
	{
		__MsgList msgs;
		{
			ScopeMutex __l(m_mutex);
			msgs = m_msgs;
			m_msgs.clear();
		}

		for (__MsgList::iterator it = msgs.begin(); it != msgs.end(); ++it)
		{
			__processMsg(*it);
		}


		delete_and_erase_collection_elements(&msgs);
	}



	void __processMsg(__Msg* msg)
	{
		if (msg->m_msg_type == __EMsgType_cmdStartListenSocket)
		{
			int index = __getSvrListenSocketCtxIndexBySocket(msg->m_socket);
			if (index >= 0)
			{
				slog_e("???");
				return;
			}

			__SvrListenSocketCtx* ctx = new __SvrListenSocketCtx();
			ctx->m_socket = msg->m_socket;
			m_listen_ctxs.push_back(ctx);

			if (!__changeSocketToAsync(ctx->m_socket))
			{
				slog_e("fail to __changeSocketToAsync, socket=%0", ctx->m_socket);
				__releaseListenCtx(ctx);
				return;
			}

			if (!__bindAndListen(ctx->m_socket, msg->m_svr_ip, msg->m_svr_port))
			{
				slog_e("fail to bind and listen, socket=%0", ctx->m_socket);
				__releaseListenCtx(ctx);
				return;
			}

			__notifyMsg_svrListenSocketListened(*ctx);
		}
		else if (msg->m_msg_type == __EMsgType_cmdStopListenSocket)
		{
			int index = __getSvrListenSocketCtxIndexBySocket(msg->m_socket);
			if (index < 0)
				return;

			__releaseListenCtx(m_listen_ctxs[index]);
		}
		else if (msg->m_msg_type == __EMsgType_cmdStopTranSocket)
		{
			int index = __getSvrTranSocketCtxIndexBySocket(msg->m_socket);
			if (index < 0)
				return;

			__releaseTranCtx(m_tran_ctxs[index]);
		}
		else if (msg->m_msg_type == __EMsgType_cmdSendDataToClient)
		{
			int index = __getSvrTranSocketCtxIndexBySocket(msg->m_socket);
			if (index < 0)
				return;

			__SvrTranSocketCtx* ctx = m_tran_ctxs[index];
			Binary* data = new Binary();
			data->attach(msg->m_data);
			ctx->m_send_datas.push_back(data);
		}
	}

	void __release()
	{
		m_is_exit = true;
		delete_and_erase_collection_elements(&m_msgs);
		delete_and_erase_collection_elements(&m_listen_ctxs);
		delete_and_erase_collection_elements(&m_tran_ctxs);
		if (m_pipe[0] != 0)
		{
			close(m_pipe[0]);
			close(m_pipe[1]);
			m_pipe[0] = 0;
			m_pipe[1] = 0;
		}
	}

	void __releaseListenCtx(__SvrListenSocketCtx* ctx)
	{
		__notifyMsg_svrListenSocketClosed(*ctx);
		delete_and_erase_vector_element_by_value(&m_listen_ctxs, ctx);
	}

	void __releaseTranCtx(__SvrTranSocketCtx* ctx)
	{
		__notifyMsg_svrTranSocketClosed(*ctx);
		delete_and_erase_vector_element_by_value(&m_tran_ctxs, ctx);
	}

	void __wakeup()
	{
		write(m_pipe[1], "0", 1);
	}

	int __getSvrListenSocketCtxIndexBySocket(int s)
	{
		for (size_t i = 0; i < m_listen_ctxs.size(); ++i)
		{
			__SvrListenSocketCtx* ctx = (__SvrListenSocketCtx*)m_listen_ctxs[i];
			if (ctx->m_socket == s)
				return (int)i;
		}
		return -1;
	}

	int __getSvrTranSocketCtxIndexBySocket(int s)
	{
		for (size_t i = 0; i < m_tran_ctxs.size(); ++i)
		{
			__SvrTranSocketCtx* ctx = (__SvrTranSocketCtx*)m_tran_ctxs[i];
			if (ctx->m_socket == s)
				return (int)i;
		}
		return -1;
	}

	void __notifyMsg_svrListenSocketListened(const __SvrListenSocketCtx& ctx)
	{
		Message* msg = new Message();
		msg->m_sender = this;
		msg->m_target = m_notify_target;
		msg->m_msg_type = EMsgType_svrListenSocketListened;
		msg->m_args.set("svr_listen_socket", ctx.m_socket);
		m_notify_looper->postMessage(msg);
	}

	void __notifyMsg_svrListenSocketClosed(const __SvrListenSocketCtx& ctx)
	{
		Message* msg = new Message();
		msg->m_sender = this;
		msg->m_target = m_notify_target;
		msg->m_msg_type = EMsgType_svrListenSocketClosed;
		msg->m_args.set("svr_listen_socket", ctx.m_socket);
		m_notify_looper->postMessage(msg);
	}

	void __notifyMsg_svrListenSocketAccepted(const __SvrTranSocketCtx& ctx)
	{
		Message* msg = new Message();
		msg->m_sender = this;
		msg->m_target = m_notify_target;
		msg->m_msg_type = EMsgType_svrListenSocketAccepted;
		msg->m_args.set("svr_listen_socket", ctx.m_ref_listen_socket);
		msg->m_args.set("svr_tran_socket", ctx.m_socket);
		m_notify_looper->postMessage(msg);
	}

	void __notifyMsg_svrTranSocketClosed(const __SvrTranSocketCtx& ctx)
	{
		Message* msg = new Message();
		msg->m_sender = this;
		msg->m_target = m_notify_target;
		msg->m_msg_type = EMsgType_svrTranSocketClosed;
		msg->m_args.set("svr_listen_socket", ctx.m_ref_listen_socket);
		msg->m_args.set("svr_tran_socket", ctx.m_socket);
		m_notify_looper->postMessage(msg);
	}

	void __notifyMsg_svrTranSocketRecvData(const __SvrTranSocketCtx& ctx, byte_t* data, size_t data_len)
	{
		Message* msg = new Message();
		msg->m_sender = this;
		msg->m_target = m_notify_target;
		msg->m_msg_type = EMsgType_svrTranSocketRecvData;
		msg->m_args.set("svr_listen_socket", ctx.m_ref_listen_socket);
		msg->m_args.set("svr_tran_socket", ctx.m_socket);
		msg->m_args.setByteArrayAndAttachFrom("data", data, data_len);
		m_notify_looper->postMessage(msg);
	}

	void __notifyMsg_svrTranSocketSendDataEnd(const __SvrTranSocketCtx& ctx)
	{
		Message* msg = new Message();
		msg->m_sender = this;
		msg->m_target = m_notify_target;
		msg->m_msg_type = EMsgType_svrTranSocketSendDataEnd;
		msg->m_args.set("svr_listen_socket", ctx.m_ref_listen_socket);
		msg->m_args.set("svr_tran_socket", ctx.m_socket);
		m_notify_looper->postMessage(msg);
	}
	


	MessageLooper* m_notify_looper;
	void* m_notify_target;
	Mutex m_mutex;
	bool m_is_exit;
	int m_pipe[2];
	__MsgList m_msgs;
	std::vector<__SvrListenSocketCtx*> m_listen_ctxs;
	std::vector<__SvrTranSocketCtx*> m_tran_ctxs;
	byte_t m_recv_buf[SOCKET_API_SVR_RECV_BUF_SIZE];
};





// init --
TcpSocketCallbackApi::TcpSocketCallbackApi()
{
	slog_d("new TcpSocketCallbackApi=%0", (uint64_t)this);
	m_sid_seed = 0;
	m_client_thread = NULL;
	m_svr_thread = NULL;
}

TcpSocketCallbackApi::~TcpSocketCallbackApi()
{
	slog_d("delete TcpSocketCallbackApi=%0", (uint64_t)this);
	ScopeMutex __l(m_mutex);
	m_work_looper->removeMsgHandler(this);
	if (m_client_thread != NULL)
	{
		m_client_thread->stopAndJoin();
		delete m_client_thread;
	}
	if (m_svr_thread != NULL)
	{
		m_svr_thread->stopAndJoin();
		delete m_svr_thread;
	}
}

bool TcpSocketCallbackApi::init(MessageLooper * work_looper)
{
	slog_d("init TcpSocketCallbackApi");
	ScopeMutex __l(m_mutex);
	m_work_looper = work_looper;
	m_work_looper->addMsgHandler(this);
	return true;
}


// client interface --
bool TcpSocketCallbackApi::createClientSocket(socket_id_t* client_sid, const CreateClientSocketParam& param)
{
    if(client_sid == NULL)
        return false;
	*client_sid = 0;

	if (param.m_callback_looper == NULL || param.m_callback_target == NULL || param.m_svr_ip_or_name.size() == 0 || param.m_svr_port == 0)
	{
		slog_e("fail to createClientSocket, param error");
		return false;
	}
 
    ScopeMutex __l(m_mutex);
	if (m_work_looper == NULL) // no init
		return false;
	if (!__initClientThread())
		return false;
    
    __SocketCtx* ctx = new __SocketCtx();
    ctx->m_sid = ++m_sid_seed;
    ctx->m_socket_type = ETcpSocketType_client;
    ctx->m_client_param = param;
	m_client_ctx_map[ctx->m_sid] = ctx;
    
    *client_sid =  ctx->m_sid;
	slog_d("createClientSocket ok, client_sid=%0", *client_sid);
    return true;
}

void TcpSocketCallbackApi::releaseClientSocket(socket_id_t client_sid)
{
	ScopeMutex __l(m_mutex);
	slog_d("release client_sid=%0", client_sid);
	__releaseClientSocket(client_sid);
}

bool TcpSocketCallbackApi::startClientSocket(socket_id_t client_sid)
{
	slog_v("startClientSocket client_sid=%0", client_sid);
	if (client_sid <= 0)
		return false;

	ScopeMutex __l(m_mutex);
	if (m_work_looper == NULL) // no init
		return false;
	if (!__initClientThread())
		return false;

	__SocketCtx* ctx = __getClientCtxById(client_sid);
	if (ctx == NULL)
		return false;
	if (ctx->m_socket != 0)
		return true;
	int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s <= 0)
	{
		slog_e("fail to create socket");
		return false;
	}
	ctx->m_socket = s;
	slog_d("startClientSocket ok, client_sid=%0, client_socket=%1", client_sid, ctx->m_socket);

	__ClientThreadRun* run = (__ClientThreadRun*)m_client_thread->getRun();
	run->postMsg_cmdConnectSvr(ctx->m_socket, ctx->m_client_param.m_svr_ip_or_name, ctx->m_client_param.m_svr_port);
	
	return true;
}

void TcpSocketCallbackApi::stopClientSocket(socket_id_t client_sid)
{
	ScopeMutex __l(m_mutex);
	slog_d("stopClientSocket cient_sid=%0", client_sid);
	__stopClientSocket(client_sid);
}

bool TcpSocketCallbackApi::sendDataFromClientSocketToSvr(socket_id_t client_sid, const byte_t* data, size_t data_len)
{
	ScopeMutex __l(m_mutex);
	slog_v("send data to svr client_sid=%0, data_len=%1", client_sid, data_len);
	__SocketCtx* ctx = __getClientCtxById(client_sid);
	if (ctx == NULL)
		return false;

	__ClientThreadRun* run = (__ClientThreadRun*)m_client_thread->getRun();
	run->postMsg_cmdSendDataToSvr(ctx->m_socket, data, data_len);
	return true;
}

std::string TcpSocketCallbackApi::getClientSocketSvrIp(socket_id_t client_sid)
{
	ScopeMutex __l(m_mutex);
	__SocketCtx* ctx = __getClientCtxById(client_sid);
	if (ctx == NULL)
		return "";
	return ctx->m_client_param.m_svr_ip_or_name;
}

uint32_t TcpSocketCallbackApi::getClientSocketSvrPort(socket_id_t client_sid)
{
	ScopeMutex __l(m_mutex);
	__SocketCtx* ctx = __getClientCtxById(client_sid);
	if (ctx == NULL)
		return 0;
	return ctx->m_client_param.m_svr_port;
}


// svr interface --
bool TcpSocketCallbackApi::createSvrListenSocket(socket_id_t* svr_listen_sid, const CreateSvrSocketParam& param)
{
	if (svr_listen_sid == NULL)
		return false;
	*svr_listen_sid = 0;

	ScopeMutex __l(m_mutex);
	if (m_work_looper == NULL) // no init
		return false;
	if (!__initSvrThread())
		return false;
	__SocketCtx* ctx = new __SocketCtx();
	ctx->m_sid = ++m_sid_seed;
	ctx->m_socket_type = ETcpSocketType_svr_listen;
	ctx->m_svr_param = param;
	m_svr_listen_ctx_map[ctx->m_sid] = ctx;

	*svr_listen_sid = ctx->m_sid;
	slog_d("createSvrListenSocket ok, svr_listen_sid=%0", *svr_listen_sid);
	return true;
}

void TcpSocketCallbackApi::releaseSvrListenSocket(socket_id_t svr_listen_sid)
{
	ScopeMutex __l(m_mutex);
	slog_d("releaseSvrListenSocket svr_listen_sid=%0", svr_listen_sid);
	__releaseSvrListenSocket(svr_listen_sid);
}

bool TcpSocketCallbackApi::startSvrListenSocket(socket_id_t svr_listen_sid)
{
	ScopeMutex __l(m_mutex);
	slog_v("startSvrListenSocket svr_listen_sid=%0", svr_listen_sid);
	if (!__initSvrThread())
		return false;
	__SocketCtx* ctx = __getSvrListenCtxById(svr_listen_sid);
	if (ctx == NULL)
	{
		slog_e("fail to find ctx");
		return false;
	}
	if (ctx->m_socket != INVALID_SOCKET)
		return true;

	int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s == INVALID_SOCKET)
	{
		slog_e("fail to create socket");
		return false;
	}
	ctx->m_socket = s;
	slog_d("startSvrListenSocket ok, svr_listen_sid=%0, svr_listen_socket=%1", svr_listen_sid, ctx->m_socket);

	__SvrThreadRun* run = (__SvrThreadRun*)m_svr_thread->getRun();
	run->postMsg_cmdStartListenSocket(ctx->m_socket, ctx->m_svr_param.m_svr_ip_or_name, ctx->m_svr_param.m_svr_port);
	return true;
}

void TcpSocketCallbackApi::stopSvrListenSocket(socket_id_t svr_listen_sid)
{
	ScopeMutex __l(m_mutex);
	slog_d("stopSvrListenSocket svr_listen_sid=%0", svr_listen_sid);
	__stopSvrListenSocket(svr_listen_sid);
}

void TcpSocketCallbackApi::stopSvrTranSocket(socket_id_t svr_tran_sid)
{
	ScopeMutex __l(m_mutex);
	slog_d("stopSvrTranSocket svr_tran_sid=%0", svr_tran_sid);
	__stopSvrTranSocket(svr_tran_sid);
}

bool TcpSocketCallbackApi::sendDataFromSvrTranSocketToClient(socket_id_t svr_tran_sid, const byte_t* data, size_t data_len)
{
	ScopeMutex __l(m_mutex);
	slog_v("send data to client svr_tran_sid=%0, data_len=%1", svr_tran_sid, data_len);
	__SocketCtx* ctx = __getSvrTranCtxById(svr_tran_sid);
	if (ctx == NULL)
		return false;

	__SvrThreadRun* run = (__SvrThreadRun*)m_svr_thread->getRun();
	run->postMsg_cmdSendDataToClient(ctx->m_socket, data, data_len);
	return true;
}

std::string TcpSocketCallbackApi::getSvrListenSocketIp(socket_id_t svr_listen_sid)
{
	ScopeMutex __l(m_mutex);
	__SocketCtx* ctx = __getClientCtxById(svr_listen_sid);
	if (ctx == NULL)
		return "";
	return ctx->m_svr_param.m_svr_ip_or_name;
}

uint32_t TcpSocketCallbackApi::getSvrListenSocketPort(socket_id_t svr_listen_sid)
{
	ScopeMutex __l(m_mutex);
	__SocketCtx* ctx = __getClientCtxById(svr_listen_sid);
	if (ctx == NULL)
		return 0;
	return ctx->m_svr_param.m_svr_port;
}







// private --
void TcpSocketCallbackApi::onMessage(Message* msg, bool* isHandled)
{
	if (msg->m_target != this)
		return;

	*isHandled = true;
	switch (msg->m_msg_type)
	{
	case __ClientThreadRun::EMsgType_clientSocketConnected:
		__onMsg_ClientSocketConnected(msg);
		break;
	case __ClientThreadRun::EMsgType_clientSocketRecvData:
		__onMsg_ClientSocketRecvData(msg);
		break;
	case __ClientThreadRun::EMsgType_clientSocketSendDataEnd:
		__onMsg_ClientSocketSendDataEnd(msg);
		break;
	case __ClientThreadRun::EMsgType_clientSocketClosed:
		__onMsg_ClientSocketClosed(msg);
		break;

	case __SvrThreadRun::EMsgType_svrListenSocketListened:
		__onMsg_SvrListenSocketListened(msg);
		break;
	case __SvrThreadRun::EMsgType_svrListenSocketAccepted:
		__onMsg_SvrListenSocketAccepted(msg);
		break;
	case __SvrThreadRun::EMsgType_svrListenSocketClosed:
		__onMsg_SvrListenSocketClosed(msg);
		break;

	case __SvrThreadRun::EMsgType_svrTranSocketRecvData:
		__onMsg_SvrTranSocketRecvData(msg);
		break;
	case __SvrThreadRun::EMsgType_svrTranSocketSendDataEnd:
		__onMsg_SvrTranSocketSendDataEnd(msg);
		break;
	case __SvrThreadRun::EMsgType_svrTranSocketClosed:
		__onMsg_SvrTransSocketClosed(msg);
		break;
	default:
		break;
	}
}

void TcpSocketCallbackApi::__onMsg_ClientSocketConnected(Message* msg)
{
	ScopeMutex __l(m_mutex);
	int client_socket = msg->m_args.getInt32("client_socket");
	__SocketCtx* ctx = __getClientCtxBySocket(client_socket);
	if (ctx == NULL)
		return;

	ClientSocketConnectedMsg* m = new ClientSocketConnectedMsg();
	m->m_client_sid = ctx->m_sid;
	__postMsgToTarget(m, ctx);
	slog_v("on client connected msg, client_sid=%0, client_socket=%1", ctx->m_sid, ctx->m_socket);
}

void TcpSocketCallbackApi::__onMsg_ClientSocketRecvData(Message* msg)
{
	ScopeMutex __l(m_mutex);
	int client_socket = msg->m_args.getInt32("client_socket");
	__SocketCtx* ctx = __getClientCtxBySocket(client_socket);
	if (ctx == NULL)
		return;
	
	ClientSocketRecvDataMsg* m = new ClientSocketRecvDataMsg();
	msg->m_args.getByteArrayAndDetachTo("data", &m->m_recv_data);
	m->m_client_sid = ctx->m_sid;
	__postMsgToTarget(m, ctx);
	slog_v("on client recv data msg, client_sid=%0, len=%1", ctx->m_sid, m->m_recv_data.getLen());
}

void TcpSocketCallbackApi::__onMsg_ClientSocketSendDataEnd(Message* msg)
{
	ScopeMutex __l(m_mutex);
	int client_socket = msg->m_args.getInt32("client_socket");
	__SocketCtx* ctx = __getClientCtxBySocket(client_socket);
	if (ctx == NULL)
		return;

	ClientSocketSendDataEndMsg* m = new ClientSocketSendDataEndMsg();
	m->m_client_sid = ctx->m_sid;
	__postMsgToTarget(m, ctx);
	slog_v("on client send data end msg ok, client_sid=%0", ctx->m_sid);
}

void TcpSocketCallbackApi::__onMsg_ClientSocketClosed(Message* msg)
{
	ScopeMutex __l(m_mutex);
	int client_socket = msg->m_args.getInt32("client_socket");
	__SocketCtx* ctx = __getClientCtxBySocket(client_socket);
	if (ctx == NULL) // assert!
		return;
	if (ctx->m_socket == INVALID_SOCKET) // assert!
		return;
	ctx->m_socket = INVALID_SOCKET;

	ClientSocketDisconnectedMsg* m = new ClientSocketDisconnectedMsg();
	m->m_client_sid = ctx->m_sid;
	__postMsgToTarget(m, ctx);
	slog_v("on client disconnected msg, client_sid=%0, client_socket=%1", ctx->m_sid, ctx->m_socket);
}

void TcpSocketCallbackApi::__onMsg_SvrListenSocketListened(Message* msg)
{
	slog_v("recv msg EMsgType_svrListenSocketListened");
	ScopeMutex __l(m_mutex);
	int svr_listen_socket = msg->m_args.getInt32("svr_listen_socket");
	__SocketCtx* ctx = __getSvrListenCtxBySocket(svr_listen_socket);
	if (ctx == NULL)
		return;

	SvrListenSocketStartedMsg* m = new SvrListenSocketStartedMsg();
	m->m_svr_listen_sid = ctx->m_sid;
	__postMsgToTarget(m, ctx);
	slog_v("on svr listened msg ok, svr_listen_sid=%0, socket=%1", ctx->m_sid, ctx->m_socket);
}

void TcpSocketCallbackApi::__onMsg_SvrListenSocketAccepted(Message* msg)
{
	slog_v("recv msg EMsgType_svrListenSocketAccepted");
	ScopeMutex __l(m_mutex);
	int svr_listen_socket = msg->m_args.getInt32("svr_listen_socket");
	int svr_tran_socket = msg->m_args.getUint32("svr_tran_socket");
	__SocketCtx* ctx_listen = __getSvrListenCtxBySocket(svr_listen_socket);
	if (ctx_listen == NULL)
		return;

	__SocketCtx* ctx = new __SocketCtx();
	ctx->m_sid = ++m_sid_seed;
	ctx->m_socket_type = ETcpSocketType_svr_tran;
	ctx->m_socket = svr_tran_socket;
	ctx->m_svr_param = ctx_listen->m_svr_param;
	ctx->m_ref_listen_sid = ctx_listen->m_sid;
	m_svr_tran_ctx_map[ctx->m_sid] = ctx;

	slog_d("on svr listen socket accepted msg ok, listen_sid=%0, listen_socket=%1, tran_sid=%2, tran_socket=%3", ctx_listen->m_sid, ctx_listen->m_socket, ctx->m_sid, ctx->m_socket);
	SvrListenSocketAcceptedMsg* m = new SvrListenSocketAcceptedMsg();
	m->m_svr_listen_sid = ctx->m_ref_listen_sid;
	m->m_svr_trans_sid = ctx->m_sid;
	__postMsgToTarget(m, ctx);
	return;
}

void TcpSocketCallbackApi::__onMsg_SvrListenSocketClosed(Message* msg)
{
	slog_v("recv msg EMsgType_svrListenSocketClosed");
	ScopeMutex __l(m_mutex);
	int svr_listen_socket = msg->m_args.getInt32("svr_listen_socket");
	__SocketCtx* ctx = __getSvrListenCtxBySocket(svr_listen_socket);
	if (ctx == NULL)
		return;
	SvrListenSocketStoppedMsg* m = new SvrListenSocketStoppedMsg();
	m->m_svr_listen_sid = ctx->m_sid;
	slog_v("on svr listen socket closed msg ok, listen_sid=%0", ctx->m_sid);
	__postMsgToTarget(m, ctx);
}

void TcpSocketCallbackApi::__onMsg_SvrTranSocketRecvData(Message* msg)
{
	slog_v("recv msg EMsgType_svrTranSocketRecvData");
	ScopeMutex __l(m_mutex);
	int svr_tran_socket = msg->m_args.getInt32("svr_tran_socket");
	__SocketCtx* ctx = __getSvrTranCtxBySocket(svr_tran_socket);
	if (ctx == NULL)
		return;
	SvrTranSocketRecvDataMsg* m = new SvrTranSocketRecvDataMsg();
	msg->m_args.getByteArrayAndDetachTo("data", &m->m_data);
	m->m_svr_listen_sid = ctx->m_ref_listen_sid;
	m->m_svr_trans_sid = ctx->m_sid;
	__postMsgToTarget(m, ctx);
	slog_v("on svr tran socket recv data msg ok, svr_tran_sid=%0, len=%1", ctx->m_sid, m->m_data.getLen());
}

void TcpSocketCallbackApi::__onMsg_SvrTranSocketSendDataEnd(Message* msg)
{
	slog_v("recv msg EMsgType_svrTranSocketSendDataEnd");
	ScopeMutex __l(m_mutex);
	int svr_tran_socket = msg->m_args.getInt32("svr_tran_socket");
	__SocketCtx* ctx = __getSvrTranCtxBySocket(svr_tran_socket);
	if (ctx == NULL)
		return;
	SvrTranSocketSendDataEndMsg* m = new SvrTranSocketSendDataEndMsg();
	m->m_svr_listen_sid = ctx->m_ref_listen_sid;
	m->m_svr_trans_sid = ctx->m_sid;
	__postMsgToTarget(m, ctx);
	slog_v("on svr tran socket send data end msg ok, svr_tran_sid=%0", ctx->m_sid);
}

void TcpSocketCallbackApi::__onMsg_SvrTransSocketClosed(Message* msg)
{
	slog_v("recv msg EMsgType_svrTranSocketClosed");
	ScopeMutex __l(m_mutex);
	int svr_tran_socket = msg->m_args.getInt32("svr_tran_socket");
	__SocketCtx* ctx = __getSvrTranCtxBySocket(svr_tran_socket);
	if (ctx == NULL)
		return;
	SvrTranSocketStoppedMsg* m = new SvrTranSocketStoppedMsg();
	m->m_svr_listen_sid = ctx->m_ref_listen_sid;
	m->m_svr_trans_sid = ctx->m_sid;
	__postMsgToTarget(m, ctx);
	__releaseSvrTranSocket(ctx->m_sid);
	slog_v("on svr tran socket closed msg ok, svr_tran_sid=%0", ctx->m_sid);
}



bool TcpSocketCallbackApi::__initClientThread()
{
	if (m_client_thread != NULL)
		return true;
	m_client_thread = new Thread(new __ClientThreadRun(m_work_looper, this));
	if (!m_client_thread->start())
	{
		slog_e("fail to start client thread");
		m_client_thread->stopAndJoin();
		delete m_client_thread;
		return false;
	}
	return true;
}

bool TcpSocketCallbackApi::__initSvrThread()
{
	if (m_svr_thread != NULL)
		return true;
	m_svr_thread = new Thread(new __SvrThreadRun(m_work_looper, this));
	if (!m_svr_thread->start())
	{
		slog_e("fail to start svr thread");
		m_svr_thread->stopAndJoin();
		delete m_svr_thread;
		return false;
	}
	return true;
}

void TcpSocketCallbackApi::__stopClientSocket(socket_id_t client_sid)
{
	__SocketCtx* ctx = __getClientCtxById(client_sid);
	if (ctx == nullptr)
		return;
	__ClientThreadRun* run = (__ClientThreadRun*)m_client_thread->getRun();
	run->postMsg_cmdDisconnectSvr(ctx->m_socket);
	ctx->m_socket = INVALID_SOCKET;
}

void TcpSocketCallbackApi::__releaseClientSocket(socket_id_t client_sid)
{
	__stopClientSocket(client_sid);
	delete_and_erase_map_element_by_key(&m_client_ctx_map, client_sid);
}

void TcpSocketCallbackApi::__stopSvrListenSocket(socket_id_t svr_listen_sid)
{
	__SocketCtx* ctx = __getSvrListenCtxById(svr_listen_sid);
	if (ctx == nullptr)
		return;
	__SvrThreadRun* run = (__SvrThreadRun*)m_svr_thread->getRun();
	run->postMsg_cmdStopListenSocket(ctx->m_socket);
	ctx->m_socket = INVALID_SOCKET;
}

void TcpSocketCallbackApi::__releaseSvrListenSocket(socket_id_t svr_listen_sid)
{
	__stopSvrListenSocket(svr_listen_sid);
	delete_and_erase_map_element_by_key(&m_svr_listen_ctx_map, svr_listen_sid);
}

void TcpSocketCallbackApi::__stopSvrTranSocket(socket_id_t svr_tran_sid)
{
	__SocketCtx* ctx = __getSvrTranCtxById(svr_tran_sid);
	if (ctx == nullptr)
		return;
	__SvrThreadRun* run = (__SvrThreadRun*)m_svr_thread->getRun();
	run->postMsg_cmdStopTranSocket(ctx->m_socket);
	ctx->m_socket = INVALID_SOCKET;
}

void TcpSocketCallbackApi::__releaseSvrTranSocket(socket_id_t svr_tran_sid)
{
	__stopSvrTranSocket(svr_tran_sid);
	delete_and_erase_map_element_by_key(&m_svr_tran_ctx_map, svr_tran_sid);
}

void TcpSocketCallbackApi::__postMsgToTarget(Message* msg, __SocketCtx * ctx)
{
	msg->m_sender = this;
	if (ctx->m_socket_type == ETcpSocketType_client)
	{
		msg->m_target = ctx->m_client_param.m_callback_target;
		//msg->m_args.setUint64("client_sid", ctx->m_sid);
		ctx->m_client_param.m_callback_looper->postMessage(msg);
	}
	else if (ctx->m_socket_type == ETcpSocketType_svr_listen)
	{
		msg->m_target = ctx->m_svr_param.m_callback_target;
		//msg->m_args.setUint64("svr_listen_sid", ctx->m_sid);
		ctx->m_svr_param.m_callback_looper->postMessage(msg);
	}
	else if (ctx->m_socket_type == ETcpSocketType_svr_tran)
	{
		msg->m_target = ctx->m_svr_param.m_callback_target;
		//msg->m_args.setUint64("svr_listen_sid", ctx->m_ref_listen_sid);
		//msg->m_args.setUint64("svr_tran_sid", ctx->m_sid);
		ctx->m_svr_param.m_callback_looper->postMessage(msg);
	}
}

TcpSocketCallbackApi::__SocketCtx* TcpSocketCallbackApi::__getClientCtxById(socket_id_t sid)
{
	return get_map_element_by_key(m_client_ctx_map, sid);
}

TcpSocketCallbackApi::__SocketCtx* TcpSocketCallbackApi::__getClientCtxBySocket(int s)
{
	for (CtxMap::iterator it = m_client_ctx_map.begin(); it != m_client_ctx_map.end(); ++it)
	{
		if (it->second->m_socket == s)
			return it->second;
	}
	return NULL;
}

TcpSocketCallbackApi::__SocketCtx* TcpSocketCallbackApi::__getSvrListenCtxById(socket_id_t sid)
{
	return get_map_element_by_key(m_svr_listen_ctx_map, sid);
}

TcpSocketCallbackApi::__SocketCtx* TcpSocketCallbackApi::__getSvrListenCtxBySocket(int s)
{
	for (CtxMap::iterator it = m_svr_listen_ctx_map.begin(); it != m_svr_listen_ctx_map.end(); ++it)
	{
		if (it->second->m_socket == s)
			return it->second;
	}
	return NULL;
}

TcpSocketCallbackApi::__SocketCtx* TcpSocketCallbackApi::__getSvrTranCtxById(socket_id_t sid)
{
	return get_map_element_by_key(m_svr_tran_ctx_map, sid);
}

TcpSocketCallbackApi::__SocketCtx* TcpSocketCallbackApi::__getSvrTranCtxBySocket(int s)
{
	for (CtxMap::iterator it = m_svr_tran_ctx_map.begin(); it != m_svr_tran_ctx_map.end(); ++it)
	{
		if (it->second->m_socket == s)
			return it->second;
	}
	return NULL;
}




S_NAMESPACE_END
#endif


