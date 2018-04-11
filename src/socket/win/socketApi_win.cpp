#include "../../comm/comm.h"
#if defined(S_OS_WIN)
#include "socketApi_win.h"
#pragma  comment(lib, "ws2_32.lib") 
S_NAMESPACE_BEGIN




// public static funtion --------------------------------------------------------------------------------------
bool initSocketLib()
{
    WORD socketVersion = MAKEWORD(2, 2);
    WSADATA wsaData;
    if (WSAStartup(socketVersion, &wsaData) != 0)
    {
        return false;
    }
    return true;
}

void releaseSocketLib()
{
    WSACleanup();
}


// inner static funtion --------------------------------------------------------------------------------------
static bool __bindAndListen(SOCKET svr_socket, const std::string& svrIpOrName, int svrPort)
{
    uint32_t svrIp = 0;
    if (!SocketUtil::getIpByName(svrIpOrName.c_str(), &svrIp))
        return false;

    struct sockaddr_in serverAddress;
    SocketUtil::initAddr(&serverAddress, svrIp, svrPort);

    int enable = 1;
    if (setsockopt(svr_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&enable, sizeof(enable)) != 0)
    {
        slog_e("tcpSocketBindAndListen setsockopt SO_REUSEADDR !");
        return false;
    }

    if (bind(svr_socket, (LPSOCKADDR)&serverAddress, sizeof(sockaddr_in)) == SOCKET_ERROR)
    {
		slog_e("tcpSocketBindAndListen bind fail, error code : %d", WSAGetLastError());
        return false;
    }

    if (listen(svr_socket, 5) == SOCKET_ERROR)
    {
		slog_e("tcpSocketBindAndListen listen fail, error code : %d", WSAGetLastError());
        return false;
    }

    return true;
}

static bool __accept(SOCKET svrSocket, SOCKET* svrTransSocket)
{
    *svrTransSocket = INVALID_SOCKET;

    char remote_ip_str[16];
    memset(remote_ip_str, 0, 16);
    uint16_t remote_port;

    struct sockaddr_in from_addr;
    memset(&from_addr, 0, sizeof(struct sockaddr_in));
    int addr_len = sizeof(sockaddr_in);

    *svrTransSocket = accept(svrSocket, (struct sockaddr*)&from_addr, &addr_len);
    if (*svrTransSocket == INVALID_SOCKET)
    {
        //slog_e("tcpSocketAccept accept fail, error code : %d", WSAGetLastError());
        return false;
    }
    char* from_ip_str = inet_ntoa(from_addr.sin_addr);
    memcpy(remote_ip_str, from_ip_str, strlen(from_ip_str));
    remote_port = ntohs(from_addr.sin_port);

    return true;
}

static bool __changeSocketToAsync(SOCKET s)
{
    DWORD ul = 1;
    if (0 != ioctlsocket(s, FIONBIO, &ul))
        return false;
    return true;
}







// SocketUtil --------------------------------------------------------------------------------------
void SocketUtil::initAddr(struct sockaddr_in* addr, uint32_t ip, int port)
{
	memset(addr, 0, sizeof(sockaddr_in));
	addr->sin_family = AF_INET;
	addr->sin_addr.s_addr = ip;
	addr->sin_port = htons((short)port);
}

bool SocketUtil::connect(SOCKET client_socket, const std::string& svrIpOrName, int svrPort)
{
	uint32_t svrIp = 0;
	if (!SocketUtil::getIpByName(svrIpOrName.c_str(), &svrIp))
		return false;

	struct sockaddr_in serverAddress;
	SocketUtil::initAddr(&serverAddress, svrIp, svrPort);

	if (::connect(client_socket, (sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR)
	{
		if (WSAGetLastError() == WSAEWOULDBLOCK)
			return true;
		return false;
	}
	return true;
}

bool SocketUtil::recv(SOCKET s, byte_t* buf, size_t bufLen, size_t* recvLen)
{
	*recvLen = 0;
	int ret = ::recv(s, (char*)buf, (int)bufLen, 0);
	if (ret > 0)
	{
		*recvLen = ret;
		return true;
	}
	else if (ret == 0)
	{
		//?
		//slog_i("tcpSocketRecv disconnected");
		return false;
	}
	else
	{
		if (WSAGetLastError() == WSAEWOULDBLOCK)
			return true;
		//slog_e("tcpSocketRecv recv failed, error code : %d", WSAGetLastError());
		return false;
	}

	return true;
}

bool SocketUtil::send(SOCKET s, const byte_t* data, size_t data_len, size_t* real_send)
{
	int ret = ::send(s, (const char*)data, (int)data_len, 0);
	if (ret <= 0)
	{
		*real_send = 0;
		if (WSAGetLastError() == WSAEWOULDBLOCK)
			return true;
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
		slog_w("gethostbyname error");
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
	//return s.m_socket != INVALID_SOCKET;
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






// SocketBlockApi --------------------------------------------------------------------------------------
ITcpSocketBlockApi* ITcpSocketBlockApi::newBlockApi()
{
	return new TcpSocketBlockApi();
}

TcpSocketBlockApi::TcpSocketBlockApi()
{
    m_sid = 0;
}

TcpSocketBlockApi::~TcpSocketBlockApi()
{
	for (__SocketCtxMap::iterator it = m_client_ctxs.begin(); it != m_client_ctxs.end(); ++it)
	{
		closeSocket(it->second->m_socket);
	}
	delete_and_erase_collection_elements(&m_client_ctxs);
}

bool TcpSocketBlockApi::openSocket(socket_id_t* sid)
{
    if (sid == NULL)
        return false;
    *sid = 0;

    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET)
        return false;

    ScopeMutex __l(m_mutex);
    __SocketCtx* ctx = new __SocketCtx();
    ctx->m_sid = ++m_sid;
    ctx->m_socket = s;
    m_client_ctxs[ctx->m_sid] = ctx;

	*sid = ctx->m_sid;
    return true;
}

void TcpSocketBlockApi::closeSocket(socket_id_t sid)
{
    if (sid <= 0)
        return;

    ScopeMutex __l(m_mutex);
    std::map<socket_id_t, __SocketCtx*>::iterator it = m_client_ctxs.find(sid);
    if (it == m_client_ctxs.end())
        return;

    ::closesocket(it->second->m_socket);
    delete it->second;
    m_client_ctxs.erase(it);
}

bool TcpSocketBlockApi::bindAndListen(socket_id_t svr_listen_sid, const std::string& svr_ip_or_name, int svr_port)
{
    __SocketCtx ctx;
	{
		ScopeMutex __l(m_mutex);
		if (!__getClientCtxById(svr_listen_sid, &ctx))
			return false;
	}

    if (!__bindAndListen(ctx.m_socket, svr_ip_or_name, svr_port))
        return false;

    return true;
}

bool TcpSocketBlockApi::accept(socket_id_t svr_listen_sid, socket_id_t* svr_tran_socket_id)
{
    __SocketCtx ctx;
	{
		ScopeMutex __l(m_mutex);
		if (!__getClientCtxById(svr_listen_sid, &ctx))
			return false;
	}

    SOCKET svr_listen_socket = ctx.m_socket;
    SOCKET svr_tran_socket = INVALID_SOCKET;
    if (!__accept(svr_listen_socket, &svr_tran_socket))
        return false;

    ScopeMutex __l(m_mutex);
    __SocketCtx* client_ctx = new __SocketCtx();
    client_ctx->m_sid = ++m_sid;
    client_ctx->m_socket = svr_tran_socket;
    m_client_ctxs[client_ctx->m_sid] = client_ctx;
    *svr_tran_socket_id = client_ctx->m_sid;
    return true;
}

bool TcpSocketBlockApi::connect(socket_id_t client_sid, const std::string& svrIpOrName, int svrPort)
{
    __SocketCtx ctx;
	{
		ScopeMutex __l(m_mutex);
		if (!__getClientCtxById(client_sid, &ctx))
			return false;
	}

    if (!SocketUtil::connect(ctx.m_socket, svrIpOrName, svrPort))
        return false;
    return true;
}

void TcpSocketBlockApi::disconnect(socket_id_t sid)
{
	ScopeMutex __l(m_mutex);
    __SocketCtx ctx;
	{
		ScopeMutex __l(m_mutex);
		if (!__getClientCtxById(sid, &ctx))
			return;
	}

    shutdown(ctx.m_socket, SD_BOTH);
}

bool TcpSocketBlockApi::send(socket_id_t sid, const byte_t* data, size_t data_len)
{
    __SocketCtx ctx;
	{
		ScopeMutex __l(m_mutex);
		if (!__getClientCtxById(sid, &ctx))
			return false;
	}

    while (true)
    {
        size_t real_send = 0;
        bool isOk = SocketUtil::send(ctx.m_socket, data, data_len, &real_send);
        if (!isOk)
            return false;
        if (real_send >= data_len) {
            return true;
        }
    
        data_len -= real_send;
    }
    return true;
}

bool TcpSocketBlockApi::recv(socket_id_t sid, byte_t* buf, size_t bufLen, size_t* recv_len)
{
    __SocketCtx ctx;
	{
		ScopeMutex __l(m_mutex);
		if (!__getClientCtxById(sid, &ctx))
			return false;
	}

    bool ret = SocketUtil::recv(ctx.m_socket, buf, bufLen, recv_len);
    return ret;
}



bool TcpSocketBlockApi::__getClientCtxById(socket_id_t sid, __SocketCtx* ctx)
{
    if (sid <= 0)
        return NULL;
    std::map<socket_id_t, __SocketCtx*>::iterator it = m_client_ctxs.find(sid);
    if (it == m_client_ctxs.end())
        return NULL;
    *ctx = *it->second;
    return true;
}




















// TcpSocketCallbackApi --------------------------------------------------------------------------------------

// static function --
static bool __createCompleteIoHandle(HANDLE* h_io)
{
    HANDLE h = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
    if (h == NULL)
        return false;

    *h_io = h;
    return true;
}

static bool __bindCompleteIoHandleAndSocket(SOCKET s, HANDLE h_io, void* user_data)
{
    if (CreateIoCompletionPort((HANDLE)s, h_io, (ULONG_PTR)user_data, 0) == NULL)
        return false;
}

static bool __createAndBindIoHandleToSocket(HANDLE* h_io, SOCKET s, void* user_data)
{
    if (!__createCompleteIoHandle(h_io))
        return false;

    if (!__bindCompleteIoHandleAndSocket(s, *h_io, user_data))
        return false;

    return true;
}

static bool __waitSocketEvent(WSAEVENT break_event, WSAEVENT socket_event, int* index)
{
    *index = -1;
    WSAEVENT arrayEvent[WSA_MAXIMUM_WAIT_EVENTS];
    arrayEvent[0] = break_event;
    arrayEvent[1] = socket_event;
    int eventNum = 2;

    // only one event will be selected
    int dwIndex = WSAWaitForMultipleEvents(eventNum, arrayEvent, FALSE, WSA_INFINITE, FALSE);
    if (WSA_WAIT_FAILED == dwIndex || WSA_WAIT_TIMEOUT == dwIndex)
    {
        return false;
    }

    *index = dwIndex - WSA_WAIT_EVENT_0;
    return true;
}

static bool __createSocketEventAndObserverSocket(WSAEVENT* socket_event, SOCKET s, int event_flag)
{
    WSAEVENT e = WSACreateEvent();
    if (e == WSA_INVALID_EVENT)
        return false;
    if (SOCKET_ERROR == WSAEventSelect(s, e, event_flag))
    {
        WSACloseEvent(e);
        return false;
    }

    *socket_event = e;
    return true;
}


// new/delete --
TcpSocketCallbackApi::TcpSocketCallbackApi()
{
	slog_d("new TcpSocketCallbackApi=%0", (uint64_t)this);
    m_sid_seed = 0;
	m_work_looper = nullptr;
	m_work_thread = nullptr;
}

TcpSocketCallbackApi::~TcpSocketCallbackApi()
{
	slog_d("delete TcpSocketCallbackApi=%0", (uint64_t)this);

	ScopeMutex __l(m_mutex);

	while (m_client_ctx_map.size() > 0)
	{
		__releaseClientSocket(m_client_ctx_map.begin()->second->m_sid);
	}
	while (m_svr_listen_ctx_map.size() > 0)
	{
		__releaseSvrListenSocket(m_svr_listen_ctx_map.begin()->second->m_sid);
	}
	while (m_svr_tran_ctx_map.size() > 0)
	{
		__releaseSvrTranSocket(m_svr_tran_ctx_map.begin()->second->m_sid);
	}

	if (m_work_thread != NULL)
	{
		m_work_thread->stopAndJoin();
		delete m_work_thread;
	}
	else
	{
		m_work_looper->removeMsgHandler(this);
	}
}

bool TcpSocketCallbackApi::init(MessageLooper * work_looper)
{
	slog_d("init TcpSocketCallbackApi");
	ScopeMutex __l(m_mutex);
	if (m_work_looper != nullptr)
	{
		slog_w("m_work_looper != nullptr");
		return true;
	}

	if (work_looper == nullptr)
	{
		m_work_thread = new MessageLoopThread(this, false);
		m_work_looper = m_work_thread->getLooper();
		if (!m_work_thread->start())
		{
			slog_e("fail to start work_thread");
			return false;
		}
	}
	else
	{
		m_work_looper = work_looper;
		m_work_looper->addMsgHandler(this);
	}

	return true;
}


// client interface --
bool TcpSocketCallbackApi::createClientSocket(socket_id_t* client_sid, const CreateClientSocketParam& param)
{
    if (client_sid == NULL)
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
    __SocketCtx* ctx = new __SocketCtx();
    ctx->m_sid = ++m_sid_seed;
    ctx->m_socket_type = ETcpSocketType_client;
    ctx->m_client_param = param;
    m_client_ctx_map[ctx->m_sid] = ctx;

    *client_sid = ctx->m_sid;

	slog_d("createClientSocket ok, client_sid=%0", *client_sid);
    return true;
}

void TcpSocketCallbackApi::releaseClientSocket(socket_id_t client_sid)
{
	ScopeMutex __l(m_mutex);
	slog_d("relese client_sid=%0", client_sid);
	__releaseClientSocket(client_sid);
}

bool TcpSocketCallbackApi::startClientSocket(socket_id_t client_sid)
{
    ScopeMutex __l(m_mutex);
	slog_d("startClientSocket cient_sid=%0", client_sid);
	__SocketCtx* ctx = __getClientCtxById(client_sid);
	if (ctx == NULL)
		return false;
	if (ctx->m_socket != INVALID_SOCKET)
		return true;

    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET)
        return false;
	ctx->m_socket = s;

    __ClientRun* r = new __ClientRun(this, m_work_looper, ctx->m_sid, ctx->m_socket, ctx->m_client_param.m_svr_ip_or_name, ctx->m_client_param.m_svr_port);
    Thread* t = new Thread(r);
	if (!t->start())
	{
		t->stopAndJoin();
		delete r;
		delete t;
		closesocket(ctx->m_socket);
		ctx->m_socket = INVALID_SOCKET;
		return false;
	}

	m_socket_thread_vector[ctx->m_sid] = t;
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
	slog_v("send data to svr client_sid=%0", client_sid);
	__SocketCtx* ctx = __getClientCtxById(client_sid);
	if (ctx == NULL)
		return false;

    Thread* t = __getThreadById(client_sid);
    if (t == NULL)
        return false;
    if (ctx->m_socket_type == ETcpSocketType_client)
    {
        __ClientRun* r = (__ClientRun*)t->getRun();
        if (!r->startSend(data, data_len))
            return false;
    }
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


// server interface --
bool TcpSocketCallbackApi::createSvrListenSocket(socket_id_t* svr_listen_sid, const CreateSvrSocketParam& param)
{
    if (svr_listen_sid == NULL)
        return false;
    *svr_listen_sid = 0;
	    
    ScopeMutex __l(m_mutex);
	if (m_work_looper == NULL) // no init
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
	slog_d("startSvrListenSocket svr_listen_sid=%0", svr_listen_sid);
	__SocketCtx* ctx = __getSvrListenCtxById(svr_listen_sid);
	if (ctx == NULL)
		return false;
	if (ctx->m_socket != INVALID_SOCKET)
		return true;

	SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s == INVALID_SOCKET)
		return false;
	ctx->m_socket = s;

	__SvrListenRun* r = new __SvrListenRun(m_work_looper, this, ctx->m_socket, ctx->m_sid, ctx->m_svr_param.m_svr_ip_or_name, ctx->m_svr_param.m_svr_port);
	Thread* t = new Thread(r);
	if (!t->start())
	{
		t->stopAndJoin();
		delete r;
		delete t;
		closesocket(ctx->m_socket);
		ctx->m_socket = INVALID_SOCKET;
		return false;
	}

	slog_d("startSvrListenSocket svr_listen_sid=%0 ok", svr_listen_sid);
	m_socket_thread_vector[ctx->m_sid] = t;
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
	__releaseSvrTranSocket(svr_tran_sid);
}

bool TcpSocketCallbackApi::sendDataFromSvrTranSocketToClient(socket_id_t svr_tran_sid, const byte_t* data, size_t data_len)
{
    ScopeMutex __l(m_mutex);
	slog_v("send data to client svr_tran_sid=%0, len=%1", svr_tran_sid, data_len);
	__SocketCtx* ctx = __getSvrTranCtxById(svr_tran_sid);
	if (ctx == NULL)
		return false;

    Thread* t = __getThreadById(svr_tran_sid);
    if (t == NULL)
        return false;   
	__SvrTransRun* r = (__SvrTransRun*)t->getRun();
	if (!r->startSend(data, data_len))
		return false;
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



//  private --
void TcpSocketCallbackApi::onMessage(Message* msg, bool* isHandled)
{
	if (msg->m_target != this)
		return;

	*isHandled = true;
	if (msg->m_sender == this)
	{

	}
	else if (msg->m_target == this)
	{
		switch (msg->m_msg_type)
		{
		case __EMsgType_onClientConnected: __onClientConnectedMsg(msg); break;
		case __EMsgType_onClientDisconnected: __onClientDisconnectedMsg(msg); break;
		case __EMsgType_onClientRecvData: __onClientRecvDataMsg(msg); break;
		case __EMsgType_onClientSendDataEnd: __onClientSendDataEndMsg(msg); break;

		case __EMsgType_onSvrListenSocketListened: __onSvrListenSocketListenedMsg(msg); break;
		case __EMsgType_onSvrListenSocketAccepted: __onSvrListenSocketAcceptedMsg(msg); break;
		case __EMsgType_onSvrListenSocketClosed: __onSvrListenSocketClosedMsg(msg); break;

		case __EMsgType_onSvrTranSocketRecvData: __onSvrTranSocketRecvDataMsg(msg); break;
		case __EMsgType_onSvrTranSocketSendDataEnd: __onSvrTranSocketSendDataEndMsg(msg); break;
		case __EMsgType_onSvrTransSocketClosed: __onSvrTransSocketClosedMsg(msg); break;
		}
	}
}

void TcpSocketCallbackApi::onMessageTimerTick(uint64_t timer_id, void * user_data)
{
}

void TcpSocketCallbackApi::__onClientConnectedMsg(Message * msg)
{
	ScopeMutex __l(m_mutex);
	socket_id_t client_sid = msg->m_args.getUint64("client_sid");
	__SocketCtx* ctx = __getClientCtxById(client_sid);
	if (ctx == NULL)
		return;

	ClientSocketConnectedMsg* m = new ClientSocketConnectedMsg();
	m->m_client_sid = client_sid;
	__postMsgToTarget(m, ctx);
	slog_v("on client connected msg ok, client_sid=%0", client_sid);
}

void TcpSocketCallbackApi::__onClientDisconnectedMsg(Message * msg)
{
	ScopeMutex __l(m_mutex);
	socket_id_t client_sid = msg->m_args.getUint64("client_sid");
	__SocketCtx* ctx = __getClientCtxById(client_sid); 
	if (ctx == NULL) // assert!
		return;
	if (ctx->m_socket == INVALID_SOCKET) // assert!
		return;
	ctx->m_socket = INVALID_SOCKET;

	__releaseThread(client_sid);

	ClientSocketDisconnectedMsg* m = new ClientSocketDisconnectedMsg();
	m->m_client_sid = client_sid;
	__postMsgToTarget(m, ctx);
	slog_v("on client disconnected msg ok, client_sid=%0", client_sid);
}

void TcpSocketCallbackApi::__onClientRecvDataMsg(Message * msg)
{
	ScopeMutex __l(m_mutex);
	socket_id_t client_sid = msg->m_args.getUint64("client_sid");
	__SocketCtx* ctx = __getClientCtxById(client_sid);
	if (ctx == NULL)
		return;
	
	ClientSocketRecvDataMsg* m = new ClientSocketRecvDataMsg();
	msg->m_args.getByteArrayAndDetachTo("data", &m->m_recv_data);
	m->m_client_sid = client_sid;
	__postMsgToTarget(m, ctx);
	slog_v("on client recv data msg ok, client_sid=%0, len=%1", client_sid, m->m_recv_data.getLen());
}

void TcpSocketCallbackApi::__onClientSendDataEndMsg(Message * msg)
{
	ScopeMutex __l(m_mutex);
	socket_id_t client_sid = msg->m_args.getUint64("client_sid");
	__SocketCtx* ctx = __getClientCtxById(client_sid);
	if (ctx == NULL)
		return;

	ClientSocketSendDataEndMsg* m = new ClientSocketSendDataEndMsg();
	m->m_client_sid = client_sid;
	__postMsgToTarget(m, ctx);
	slog_v("on client send data end msg ok, client_sid=%0", client_sid);
}

void TcpSocketCallbackApi::__onSvrListenSocketListenedMsg(Message * msg)
{
	ScopeMutex __l(m_mutex);
	socket_id_t svr_listen_sid = msg->m_args.getUint64("svr_listen_sid");
	__SocketCtx* ctx = __getSvrListenCtxById(svr_listen_sid);
	if (ctx == NULL)
		return;

	SvrListenSocketStartedMsg* m = new SvrListenSocketStartedMsg();
	m->m_svr_listen_sid = svr_listen_sid;
	__postMsgToTarget(m, ctx);
	slog_v("on svr listened msg ok, svr_listen_sid=%0", svr_listen_sid);
}

void TcpSocketCallbackApi::__onSvrListenSocketAcceptedMsg(Message * msg)
{
	ScopeMutex __l(m_mutex);
	socket_id_t svr_listen_sid = msg->m_args.getUint64("svr_listen_sid");
	SOCKET svr_tran_socket = msg->m_args.getUint32("svr_tran_socket");
	__SocketCtx* ctx_listen = __getSvrListenCtxById(svr_listen_sid);
	if (ctx_listen == NULL)
		return;

	__SocketCtx* ctx = new __SocketCtx();
	ctx->m_sid = ++m_sid_seed;
	ctx->m_socket_type = ETcpSocketType_svr_tran;
	ctx->m_socket = svr_tran_socket;
	ctx->m_svr_param = ctx_listen->m_svr_param;
	ctx->m_ref_listen_sid = svr_listen_sid;
	m_svr_tran_ctx_map[ctx->m_sid] = ctx;

	__SvrTransRun* r = new __SvrTransRun(ctx->m_socket, ctx->m_sid, m_work_looper, this);
	Thread* t = new Thread(r);
	m_socket_thread_vector[ctx->m_sid] = t;
	if (!t->start())
	{
		__releaseSvrTranSocket(svr_listen_sid);
		return;
	}

	slog_d("on svr listen socket accepted msg ok, listen_sid=%0, listen_socket=%1, tran_sid=%2, tran_socket=%3", ctx_listen->m_sid, ctx_listen->m_socket, ctx->m_sid, ctx->m_socket);
	SvrListenSocketAcceptedMsg* m = new SvrListenSocketAcceptedMsg();
	m->m_svr_listen_sid = svr_listen_sid;
	m->m_svr_trans_sid = ctx->m_sid;
	__postMsgToTarget(m, ctx);
	return;
}

void TcpSocketCallbackApi::__onSvrListenSocketClosedMsg(Message * msg)
{
	ScopeMutex __l(m_mutex);
	socket_id_t svr_listen_sid = msg->m_args.getUint64("svr_listen_sid");
	__SocketCtx* ctx = __getSvrListenCtxById(svr_listen_sid);
	if (ctx == NULL)
		return;
	SvrListenSocketStoppedMsg* m = new SvrListenSocketStoppedMsg();
	m->m_svr_listen_sid = svr_listen_sid;
	slog_v("on svr listen socket closed msg ok, listen_sid=%0", svr_listen_sid);
	__postMsgToTarget(m, ctx);
}

void TcpSocketCallbackApi::__onSvrTranSocketRecvDataMsg(Message * msg)
{
	ScopeMutex __l(m_mutex);
	socket_id_t svr_tran_sid = msg->m_args.getUint64("svr_tran_sid");
	__SocketCtx* ctx = __getSvrTranCtxById(svr_tran_sid);
	if (ctx == NULL)
		return;
	SvrTranSocketRecvDataMsg* m = new SvrTranSocketRecvDataMsg();
	msg->m_args.getByteArrayAndDetachTo("data", &m->m_data);
	m->m_svr_listen_sid = ctx->m_ref_listen_sid;
	m->m_svr_trans_sid = svr_tran_sid;
	__postMsgToTarget(m, ctx);
	slog_v("on svr tran socket recv data msg ok, svr_tran_sid=%0, len=%1", svr_tran_sid, m->m_data.getLen());
}

void TcpSocketCallbackApi::__onSvrTranSocketSendDataEndMsg(Message * msg)
{
	ScopeMutex __l(m_mutex);
	socket_id_t svr_tran_sid = msg->m_args.getUint64("svr_tran_sid");
	__SocketCtx* ctx = __getSvrTranCtxById(svr_tran_sid);
	if (ctx == NULL)
		return;
	SvrTranSocketSendDataEndMsg* m = new SvrTranSocketSendDataEndMsg();
	m->m_svr_listen_sid = ctx->m_ref_listen_sid;
	m->m_svr_trans_sid = ctx->m_sid;
	__postMsgToTarget(m, ctx);
	slog_v("on svr tran socket send data end msg ok, svr_tran_sid=%0", svr_tran_sid);
}

void TcpSocketCallbackApi::__onSvrTransSocketClosedMsg(Message * msg)
{
	ScopeMutex __l(m_mutex);
	socket_id_t svr_tran_sid = msg->m_args.getUint64("svr_tran_sid");
	__SocketCtx* ctx = __getSvrTranCtxById(svr_tran_sid);
	if (ctx == NULL)
		return;
	SvrTranSocketStoppedMsg* m = new SvrTranSocketStoppedMsg();
	m->m_svr_listen_sid = ctx->m_ref_listen_sid;
	m->m_svr_trans_sid = ctx->m_sid;
	__postMsgToTarget(m, ctx);
	__releaseSvrTranSocket(svr_tran_sid);
	slog_v("on svr tran socket closed msg ok, svr_tran_sid=%0", svr_tran_sid);
}




void TcpSocketCallbackApi::__stopSocketByCtx(__SocketCtx* ctx)
{
	if (ctx == NULL)
		return;
	if (ctx->m_socket == INVALID_SOCKET)
		return;

	::closesocket(ctx->m_socket);
	ctx->m_socket = INVALID_SOCKET;
	__releaseThread(ctx->m_sid);
}

void TcpSocketCallbackApi::__stopClientSocket(socket_id_t client_sid)
{
	__SocketCtx* ctx = __getClientCtxById(client_sid);
	__stopSocketByCtx(ctx);
}

void TcpSocketCallbackApi::__releaseClientSocket(socket_id_t client_sid)
{
	__stopClientSocket(client_sid);
	delete_and_erase_map_element_by_key(&m_client_ctx_map, client_sid);
}

void TcpSocketCallbackApi::__stopSvrListenSocket(socket_id_t svr_listen_sid)
{
	__SocketCtx* ctx = __getSvrListenCtxById(svr_listen_sid); 
	__stopSocketByCtx(ctx);
}

void TcpSocketCallbackApi::__releaseSvrListenSocket(socket_id_t svr_listen_sid)
{
	__stopSvrListenSocket(svr_listen_sid);
	delete_and_erase_map_element_by_key(&m_svr_listen_ctx_map, svr_listen_sid);
}

void TcpSocketCallbackApi::__stopSvrTranSocket(socket_id_t svr_tran_sid)
{
	__SocketCtx* ctx = __getSvrTranCtxById(svr_tran_sid);
	__stopSocketByCtx(ctx);
}

void TcpSocketCallbackApi::__releaseSvrTranSocket(socket_id_t svr_tran_sid)
{
	__stopSvrTranSocket(svr_tran_sid);
	delete_and_erase_map_element_by_key(&m_svr_tran_ctx_map, svr_tran_sid);
}

void TcpSocketCallbackApi::__releaseThread(socket_id_t sid)
{
	std::map<socket_id_t, Thread*>::iterator it = m_socket_thread_vector.find(sid);
	if (it == m_socket_thread_vector.end())
		return;
	Thread* t = it->second;
	m_socket_thread_vector.erase(sid);
	t->stopAndJoin();
	delete t;
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
	std::map<socket_id_t, __SocketCtx*>::iterator it = m_client_ctx_map.find(sid);
	if (it == m_client_ctx_map.end())
		return NULL;
	return it->second;
	//for (size_t i = 0; i < m_ctxs.size(); ++i)
	//{
	//	if (m_ctxs[i]->m_sid == sid)
	//		return (int)i;
	//}
	//return -1;
}

TcpSocketCallbackApi::__SocketCtx * TcpSocketCallbackApi::__getSvrListenCtxById(socket_id_t sid)
{
	std::map<socket_id_t, __SocketCtx*>::iterator it = m_svr_listen_ctx_map.find(sid);
	if (it == m_svr_listen_ctx_map.end())
		return NULL;
	return it->second;
}

TcpSocketCallbackApi::__SocketCtx * TcpSocketCallbackApi::__getSvrTranCtxById(socket_id_t sid)
{
	std::map<socket_id_t, __SocketCtx*>::iterator it = m_svr_tran_ctx_map.find(sid);
	if (it == m_svr_tran_ctx_map.end())
		return NULL;
	return it->second;
}

Thread* TcpSocketCallbackApi::__getThreadById(socket_id_t sid)
{
    if (sid <= 0)
        return NULL;
    std::map<socket_id_t, Thread*>::iterator it = m_socket_thread_vector.find(sid);
    if (it == m_socket_thread_vector.end())
        return NULL;
    return it->second;
}
















// __ClientRun --
TcpSocketCallbackApi::__ClientRun::__ClientRun(void* msg_target, MessageLooper* notify_looper, socket_id_t sid, SOCKET s, const std::string& svr_ip_or_name, int svr_port)
{
    m_socket = s;
    m_sid = sid;
	m_msg_target = msg_target;
	m_callback_looper = notify_looper;
    m_svr_ip_or_name = svr_ip_or_name;
    m_svr_port = svr_port;

    m_is_exit = false;
	m_socket_event = WSA_INVALID_EVENT;
	m_break_event = WSA_INVALID_EVENT;
}

void TcpSocketCallbackApi::__ClientRun::run()
{
    __run();

	__postDisconnectedMsg();

	__clearSession();
	m_is_exit = true;
}

void TcpSocketCallbackApi::__ClientRun::stop()
{
    ScopeMutex __l(m_mutex);
    if (m_is_exit)
        return;
    m_is_exit = true;

    closesocket(m_socket);
    WSASetEvent(m_break_event);
}

bool TcpSocketCallbackApi::__ClientRun::startSend(const byte_t* data, size_t len)
{
    ScopeMutex __l(m_mutex);
    if (m_is_exit)
        return false;
    if (data == NULL || len == 0)
        return false;

	Binary* bin = new Binary();
	bin->copy(data, len);
    m_send_datas.push_back(bin);
    WSASetEvent(m_break_event);
    return true;
}

void TcpSocketCallbackApi::__ClientRun::__run()
{
	m_break_event = WSACreateEvent();
	if (m_break_event == WSA_INVALID_EVENT)
		return;

    if (!SocketUtil::connect(m_socket, m_svr_ip_or_name, m_svr_port))
        return;

	Message* msg = new Message();
	msg->m_msg_type = __EMsgType_onClientConnected;
	__postMsgToTarget(msg);

	{
		ScopeMutex __l(m_mutex);
		//if (!__changeSocketToAsync(m_socket))
		//	return;

		if (!__createSocketEventAndObserverSocket(&m_socket_event, m_socket, FD_WRITE | FD_READ | FD_CLOSE))
			return;
	}

    while (true)
    {
        if (!__onReadEvent())
            return;
        if (!__onSendEvent())
            return;

        int index = -1;
        if (!__waitSocketEvent(m_break_event, m_socket_event, &index))
            return;

        if (m_is_exit)
            return;

        // send
        if (index == 0)
        {
            WSAResetEvent(m_break_event);
            __onSendEvent();
        }
        else if (index == 1)
        {
            WSANETWORKEVENTS ne;
            if (WSAEnumNetworkEvents(m_socket, m_socket_event, &ne) != 0)
                return;
        }

        //    if (ne.lNetworkEvents & FD_WRITE)
        //    {
        //        if (!__onSendEvent())
        //            return;
        //    }
        //    else if (ne.lNetworkEvents & FD_READ)
        //    {
        //        if (!__onReadEvent())
        //            return;
        //    }
        //    else if (ne.lNetworkEvents & FD_CLOSE)
        //    {
        //        return;
        //    }
        //}
        //else
        //{
        //    // error
        //    return; 
        //}
    }
}

bool TcpSocketCallbackApi::__ClientRun::__onSendEvent()
{
    ScopeMutex __l(m_mutex);
    if (m_send_datas.size() == 0)
        return true;

	while (m_send_datas.size() > 0)
	{
		Binary* data_to_send = m_send_datas[0];

		size_t real_send = 0;
		while (true)
		{
			size_t cur_real_send = 0;
			if (!SocketUtil::send(m_socket, data_to_send->getData() + real_send, data_to_send->getLen() - real_send, &cur_real_send))
				return false;
			real_send += cur_real_send;
			if (cur_real_send == 0)
				break;
			if (real_send == data_to_send->getLen())
				break;
		}
		if (real_send == 0) // should wait
			return true;

		if (data_to_send->getLen() == real_send)
		{
			delete_and_erase_vector_element_by_index(&m_send_datas, 0);
			Message* msg = new Message();
			msg->m_msg_type = __EMsgType_onClientSendDataEnd;
			__postMsgToTarget(msg);
			slog_d("__ClientRun send data ok, len=%0", real_send);
		}
		else
		{
			data_to_send->shrinkBegin(real_send);
			break;
		}
	}
    return true;
}

bool TcpSocketCallbackApi::__ClientRun::__onReadEvent()
{
    while (true)
    {
		byte_t buf[64 * 1024];
		size_t recv_len = 0;
        if (!SocketUtil::recv(m_socket, buf, 64 * 1024, &recv_len))
            return false;

		if (recv_len > 0)
		{
			Message* msg = new Message();
			msg->m_msg_type = __EMsgType_onClientRecvData;
			msg->m_args.setByteArrayAndCopyFrom("data", buf, recv_len);
			__postMsgToTarget(msg);
			slog_d("__ClientRun recv data ok, len=%0", recv_len);
		}

		if (recv_len != 64 * 1024) // should wait
			return true;
    }
    return true;
}

void TcpSocketCallbackApi::__ClientRun::__postMsgToTarget(Message* msg)
{
	msg->m_sender = this;
	msg->m_target = m_msg_target;
	msg->m_args.setUint64("client_sid", m_sid);
	m_callback_looper->postMessage(msg);
}

void TcpSocketCallbackApi::__ClientRun::__postDisconnectedMsg()
{
	Message* msg = new Message();
	msg->m_msg_type = __EMsgType_onClientDisconnected;
	__postMsgToTarget(msg);
}

void TcpSocketCallbackApi::__ClientRun::__clearSession()
{
    if (m_sid == 0)
        return;
    m_sid = 0;

	m_msg_target = NULL;
    m_callback_looper = NULL;
    closesocket(m_socket);
    m_socket = 0;
    WSACloseEvent(m_break_event);
    m_break_event = NULL;
    WSACloseEvent(m_socket_event);
    m_socket_event = NULL;
	delete_and_erase_collection_elements(&m_send_datas);
}




// __SvrListenRun --
TcpSocketCallbackApi::__SvrListenRun::__SvrListenRun(MessageLooper* notify_looper, void* notify_target, SOCKET s, socket_id_t sid, const std::string& svr_ip_or_name, int svr_port)
{
	m_callback_looper = notify_looper;
	m_notify_target = notify_target;
    m_socket = s;
    m_sid = sid;
    m_svr_ip_or_name = svr_ip_or_name;
    m_svr_port = svr_port;
	m_is_exit = false;
}

void TcpSocketCallbackApi::__SvrListenRun::run()
{
	slog_d("run sid=%0, enter -->", m_sid);
    __run();

	{
		ScopeMutex __l(m_mutex);
		if (!m_is_exit)
		{
			m_is_exit = true;
			::closesocket(m_socket);
			m_socket = INVALID_SOCKET;
		}
	}

	Message* msg = new Message();
	msg->m_msg_type = EMsgType_svrListenSocketStopped;
	__postMsgToTarget(msg);
	slog_d("run sid=%0, exit <--", m_sid);
}

void TcpSocketCallbackApi::__SvrListenRun::stop()
{
	slog_d("stop, sid=%0", m_sid);
	ScopeMutex __l(m_mutex);
	if (m_is_exit)
		return;
    m_is_exit = true;

    ::closesocket(m_socket);
	m_socket = INVALID_SOCKET;
}

void TcpSocketCallbackApi::__SvrListenRun::__run()
{
	Message* msg = new Message();
	msg->m_msg_type = EMsgType_svrListenSocketStarted;
	//slog_d("__postMsgToTarget EMsgType_svrListenSocketStarted begin");
	__postMsgToTarget(msg);
	//slog_d("__postMsgToTarget EMsgType_svrListenSocketStarted end");

    if (!__bindAndListen(m_socket, m_svr_ip_or_name, m_svr_port))
        return;

    while (true)
    {
        SOCKET svr_tran_socket;
		if (!__accept(m_socket, &svr_tran_socket))
			return;
		if (m_is_exit)
			return;

		slog_d("__SvrListenRun accept socket, listen_sid=%0", m_sid);
		Message* msg = new Message();
		msg->m_msg_type = __EMsgType_onSvrListenSocketAccepted;
		msg->m_args.setUint64("svr_tran_socket", svr_tran_socket);
		__postMsgToTarget(msg);
    }
}

void TcpSocketCallbackApi::__SvrListenRun::__postMsgToTarget(Message * msg)
{
	msg->m_sender = this;
	msg->m_target = m_notify_target;
	msg->m_args.setUint64("svr_listen_sid", m_sid);
	m_callback_looper->postMessage(msg);
}





// __SvrTransRun --
TcpSocketCallbackApi::__SvrTransRun::__SvrTransRun(SOCKET s, socket_id_t sid, MessageLooper* notify_looper, void* notify_target)
{
    m_socket = s;
    m_sid = sid;
	m_notify_looper = notify_looper;
	m_notify_target = notify_target;

    m_is_exit = false;
    m_isSending = false;
}

void TcpSocketCallbackApi::__SvrTransRun::run()
{
	slog_d("__SvrTransRun::run enter, sid=%0", m_sid);
    __run();

    ScopeMutex __l(m_mutex);
	Message* msg = new Message();
	msg->m_msg_type = EMsgType_svrTranSocketStopped;
	__postMsgToTarget(msg);
	slog_d("__SvrTransRun::run exit, sid=%0", m_sid);
    __clearSession();
    m_is_exit = true;
}

void TcpSocketCallbackApi::__SvrTransRun::stop()
{
    ScopeMutex __l(m_mutex);
    if (m_is_exit)
        return;
    m_is_exit = true;

	slog_d("__SvrTransRun::stop, sid=%0", m_sid);
    closesocket(m_socket);
    WSASetEvent(m_break_event);
}

bool TcpSocketCallbackApi::__SvrTransRun::startSend(const byte_t* data, size_t len)
{
    ScopeMutex __l(m_mutex);
    if (m_is_exit)
        return false;
    if (data == NULL || len == 0)
        return false;
    if (m_isSending)
        return false;
    m_isSending = true;

    m_dataToSend.copy(data, len);
    WSASetEvent(m_break_event);
    return true;
}

void TcpSocketCallbackApi::__SvrTransRun::__run()
{
    m_socket_event = WSA_INVALID_EVENT;

    m_break_event = WSACreateEvent();
    if (m_break_event == WSA_INVALID_EVENT)
        return;
    
    if (!__changeSocketToAsync(m_socket))
        return;

    if (!__createSocketEventAndObserverSocket(&m_socket_event, m_socket, FD_WRITE | FD_READ | FD_CLOSE))
        return;

    while (true)
    {
        if (!__onReadEvent())
            return;
        if (!__onSendEvent())
            return;

        int index = -1;
        if (!__waitSocketEvent(m_break_event, m_socket_event, &index))
            return;

        if (m_is_exit)
            return;

        // send
        if (index == 0)
        {
            WSAResetEvent(m_break_event);
            __onSendEvent();
        }
        else if (index == 1)
        {
            WSANETWORKEVENTS ne;
            if (WSAEnumNetworkEvents(m_socket, m_socket_event, &ne) != 0)
                return;
        }
    }
}

bool TcpSocketCallbackApi::__SvrTransRun::__onSendEvent()
{
    ScopeMutex __l(m_mutex);
    if (m_dataToSend.getLen() == 0)
        return true;
	size_t real_send = 0;
    if (!SocketUtil::send(m_socket, m_dataToSend.getData(), m_dataToSend.getLen(), &real_send))
        return false;
    if (real_send == 0) // should wait
        return true;

    m_dataToSend.shrinkBegin(real_send);
    if (m_dataToSend.getLen() == 0)
    {
        m_isSending = false;

		Message* msg = new Message();
		msg->m_msg_type = __EMsgType_onSvrTranSocketSendDataEnd;
		__postMsgToTarget(msg);
    }
    return true;
}

bool TcpSocketCallbackApi::__SvrTransRun::__onReadEvent()
{
    while (true)
    {
		byte_t buf[64 * 1024];
		size_t recv_len = 0;
		if (!SocketUtil::recv(m_socket, buf, 64 * 1024, &recv_len))
			return false;

		if (recv_len > 0) 
		{
			Message* msg = new Message();
			msg->m_msg_type = __EMsgType_onSvrTranSocketRecvData;
			msg->m_args.setByteArrayAndCopyFrom("data", buf, recv_len);
			__postMsgToTarget(msg);
		}

		if (recv_len < 64 * 1024) // should wait
		{
			return true;			
		}
    }
    return true;
}

void TcpSocketCallbackApi::__SvrTransRun::__postMsgToTarget(Message * msg)
{
	msg->m_sender = this;
	msg->m_target = m_notify_target;
	msg->m_args.setUint64("svr_tran_sid", m_sid);
	m_notify_looper->postMessage(msg);
}

void TcpSocketCallbackApi::__SvrTransRun::__clearSession()
{
    if (m_sid == 0)
        return;
    m_sid = 0;

	m_notify_looper = NULL;
	m_notify_target = NULL;
    closesocket(m_socket);
    m_socket = 0;
    WSACloseEvent(m_break_event);
    m_break_event = NULL;
    WSACloseEvent(m_socket_event);
    m_socket_event = NULL;
    m_isSending = false;
}



S_NAMESPACE_END
#endif

