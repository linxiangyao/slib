#ifndef S_SOCKET_API_H_
#define S_SOCKET_API_H_
#include <vector>
#include <stdio.h>
#include "../comm/comm.h"
#include "../thread/threadLib.h"
#include "socketApiType.h"
S_NAMESPACE_BEGIN



bool initSocketLib();
void releaseSocketLib();





/*
block tcp function.
*/
class ITcpSocketBlockApi
{
public:
	static ITcpSocketBlockApi* newBlockApi();

    virtual ~ITcpSocketBlockApi() {}

    // create/close
    virtual bool openSocket(socket_id_t* s) = 0;
    virtual void closeSocket(socket_id_t s) = 0;

    // svr
    virtual bool bindAndListen(socket_id_t svr_socket, const std::string& svr_ip_or_name, int svr_port) = 0;
    virtual bool accept(socket_id_t svr_socket, socket_id_t* svr_tran_socket) = 0;

    // client
    virtual bool connect(socket_id_t cliet_socket, const std::string& svr_ip_or_name, int svr_port) = 0;
    virtual void disconnect(socket_id_t cliet_socket) = 0;

    // transport
    virtual bool send(socket_id_t s, const byte_t* data, size_t data_len) = 0;
    virtual bool recv(socket_id_t s, byte_t* buf, size_t buf_len, size_t* recvLen) = 0;
};








/*
none block tcp function.


NOTE:
ITcpSocketCallbackApi is thread safe.
init(MessageLooper* work_looper)
	1. if work_looper != null: ITcpSocketCallbackApi will run in the message looper thread which own the work_looper.
	2. if work_looper == null: ITcpSocketCallbackApi will create a message looper thead and run in the thread.
ITcpSocketCallbackApi will post message to callback_looper.
*/
class ITcpSocketCallbackApi
{
public:
	enum EMsgType
	{
		EMsgType_clientSocketConnected = -87634851,
		EMsgType_clientSocketDisconnected,
		EMsgType_clientSocketRecvData,
		EMsgType_clientSocketSendDataEnd,

		EMsgType_svrListenSocketStarted,
		EMsgType_svrListenSocketStopped,
		EMsgType_svrListenSocketAccepted,

		EMsgType_svrTranSocketRecvData,
		EMsgType_svrTranSocketSendDataEnd,
		EMsgType_svrTranSocketStopped,
	};

    class CreateClientSocketParam
    {
    public:
		CreateClientSocketParam() { m_callback_looper = NULL; m_callback_target = NULL; m_svr_port = 0; }

        MessageLooper* m_callback_looper;
		void* m_callback_target;
        std::string m_svr_ip_or_name;
        uint32_t m_svr_port;
    };

    class CreateSvrSocketParam
    {
    public:
		CreateSvrSocketParam() { m_callback_looper = NULL; m_callback_target = NULL; m_svr_port = 0; }

		MessageLooper* m_callback_looper;
		void* m_callback_target;
        std::string m_svr_ip_or_name;
		uint32_t m_svr_port;
    };

	class ClientSocketConnectedMsg : public Message
	{
	public:
		ClientSocketConnectedMsg() { m_msg_type = EMsgType_clientSocketConnected; m_client_sid = 0;  }
		Message* clone() const { ClientSocketConnectedMsg* msg = new ClientSocketConnectedMsg(); *msg = *this; return msg; }
		socket_id_t m_client_sid;
	};

	class ClientSocketDisconnectedMsg : public Message
	{
	public:
		ClientSocketDisconnectedMsg() { m_msg_type = EMsgType_clientSocketDisconnected; m_client_sid = 0; }
		Message* clone() const { ClientSocketDisconnectedMsg* msg = new ClientSocketDisconnectedMsg(); *msg = *this; return msg; }
		socket_id_t m_client_sid;
	};

	class ClientSocketSendDataEndMsg : public Message
	{
	public:
		ClientSocketSendDataEndMsg() { m_msg_type = EMsgType_clientSocketSendDataEnd; m_client_sid = 0; }
		Message* clone() const { ClientSocketSendDataEndMsg* msg = new ClientSocketSendDataEndMsg(); *msg = *this; return msg; }
		socket_id_t m_client_sid;
	};

	class ClientSocketRecvDataMsg : public Message
	{
	public:
		ClientSocketRecvDataMsg() { m_msg_type = EMsgType_clientSocketRecvData; m_client_sid = 0; }
		Message* clone() const { ClientSocketRecvDataMsg* msg = new ClientSocketRecvDataMsg(); *msg = *this; return msg; }
		socket_id_t m_client_sid;
		Binary m_recv_data;
	};

	class SvrListenSocketStartedMsg : public Message
	{
	public:
		SvrListenSocketStartedMsg() { m_msg_type = EMsgType_svrListenSocketStarted; m_svr_listen_sid = 0;  }
		Message* clone() const { SvrListenSocketStartedMsg* msg = new SvrListenSocketStartedMsg(); *msg = *this; return msg; }
		socket_id_t m_svr_listen_sid;
	};

	class SvrListenSocketStoppedMsg : public Message
	{
	public:
		SvrListenSocketStoppedMsg() { m_msg_type = EMsgType_svrListenSocketStopped; m_svr_listen_sid = 0; }
		Message* clone() const { SvrListenSocketStoppedMsg* msg = new SvrListenSocketStoppedMsg(); *msg = *this; return msg; }
		socket_id_t m_svr_listen_sid;
	};

	class SvrListenSocketAcceptedMsg : public Message
	{
	public:
		SvrListenSocketAcceptedMsg() { m_msg_type = EMsgType_svrListenSocketAccepted; m_svr_listen_sid = 0; m_svr_trans_sid = 0; }
		Message* clone() const { SvrListenSocketAcceptedMsg* msg = new SvrListenSocketAcceptedMsg(); *msg = *this; return msg; }
		socket_id_t m_svr_listen_sid;
		socket_id_t m_svr_trans_sid;
	};

	class SvrTranSocketRecvDataMsg : public Message
	{
	public:
		SvrTranSocketRecvDataMsg() { m_msg_type = EMsgType_svrTranSocketRecvData; m_svr_listen_sid = 0; m_svr_trans_sid = 0;  }
		Message* clone() const { SvrTranSocketRecvDataMsg* msg = new SvrTranSocketRecvDataMsg(); *msg = *this; return msg; }
		socket_id_t m_svr_listen_sid;
		socket_id_t m_svr_trans_sid;
		Binary m_data;
	};

	class SvrTranSocketSendDataEndMsg : public Message
	{
	public:
		SvrTranSocketSendDataEndMsg() { m_msg_type = EMsgType_svrTranSocketSendDataEnd; m_svr_listen_sid = 0; m_svr_trans_sid = 0; }
		Message* clone() const { SvrTranSocketSendDataEndMsg* msg = new SvrTranSocketSendDataEndMsg(); *msg = *this; return msg; }
		socket_id_t m_svr_listen_sid;
		socket_id_t m_svr_trans_sid;
	};

	class SvrTranSocketStoppedMsg : public Message
	{
	public:
		SvrTranSocketStoppedMsg() { m_msg_type = EMsgType_svrTranSocketStopped; m_svr_listen_sid = 0; m_svr_trans_sid = 0; }
		Message* clone() const { SvrTranSocketStoppedMsg* msg = new SvrTranSocketStoppedMsg(); *msg = *this; return msg; }
		socket_id_t m_svr_listen_sid;
		socket_id_t m_svr_trans_sid;
	};
	


	virtual ~ITcpSocketCallbackApi() {}
	virtual bool init(MessageLooper* work_looper = nullptr) = 0;

    // client
    virtual bool createClientSocket(socket_id_t* client_sid, const CreateClientSocketParam& param) = 0;
	virtual void releaseClientSocket(socket_id_t client_sid) = 0;
    virtual bool startClientSocket(socket_id_t client_sid) = 0;
    virtual void stopClientSocket(socket_id_t client_sid) = 0;
    virtual bool sendDataFromClientSocketToSvr(socket_id_t client_sid, const byte_t* data, size_t data_len) = 0;
	virtual std::string getClientSocketSvrIp(socket_id_t client_sid) = 0;
	virtual uint32_t getClientSocketSvrPort(socket_id_t client_sid) = 0;

    // svr listen
    virtual bool createSvrListenSocket(socket_id_t* svr_listen_sid, const CreateSvrSocketParam& param) = 0;
	virtual void releaseSvrListenSocket(socket_id_t svr_listen_sid) = 0;
    virtual bool startSvrListenSocket(socket_id_t svr_listen_sid) = 0;
    virtual void stopSvrListenSocket(socket_id_t svr_listen_sid) = 0;
	virtual std::string getSvrListenSocketIp(socket_id_t svr_listen_sid) = 0;
	virtual uint32_t getSvrListenSocketPort(socket_id_t svr_listen_sid) = 0;

    // svr tran
    virtual void stopSvrTranSocket(socket_id_t svr_tran_sid) = 0;
    virtual bool sendDataFromSvrTranSocketToClient(socket_id_t svr_tran_sid, const byte_t* data, size_t data_len) = 0;
};




// util
class SocketUtil
{
public:
	static bool bindAndListen(socket_t s, const std::string& svr_ip_or_name, int svr_port);
	static bool accept(socket_t svr_accept_socket, socket_t* svr_trans_socket);
	static bool connect(socket_t client_socket, const std::string& svr_ip_or_name, int svr_port);
	static bool recv(socket_t s, byte_t* buf, size_t buf_len, size_t* recv_len);
	static bool send(socket_t s, const byte_t* data, size_t data_len, size_t* real_send);
	static bool send(socket_t s, const byte_t* data, size_t data_len);
	static void closeSocket(socket_t s);
	static bool changeSocketToAsync(socket_t s);
    static bool getIpByName(const char* name, std::vector<Ip>* ips); // will block
	static int getErr();


	static bool ipToStr(const sockaddr* addr, std::string* ip_str);
	static bool ipToStr(Ip ip, std::string* ip_str);
    static bool ipv4ToStr(in_addr ip_v4, std::string* ip_str);
	static bool ipv6ToStr(in6_addr ip_v6, std::string* ip_str);
	static bool strToIp(const std::string& ip_str, Ip* ip);
    static bool strToIpv4(const std::string& ip_str, in_addr* ip_v4);
	static bool strToIpv6(const std::string& ip_str, in6_addr* ip_v6);
    static uint16_t hToNs(uint16_t s);
    static uint32_t hToNl(uint32_t l);
    static uint16_t nToHs(uint16_t s);
    static uint32_t nToHl(uint32_t l);
    static bool isValidSocketId(socket_id_t s);

private:
	static bool __getIpByNameAndIinitAddr(const std::string& name, int port, sockaddr_storage* addr, int* addr_len);
	static void __initAddrV4(struct sockaddr_in* addr, in_addr ip, int port);
	static void __initAddrV6(struct sockaddr_in6* addr, in6_addr ip, int port);
};



// block api ----------------------------------------------------------------------------------------------------------
class TcpSocketBlockApi : public ITcpSocketBlockApi
{
public:
	TcpSocketBlockApi();
	~TcpSocketBlockApi();

	// create/close
	bool openSocket(socket_id_t* s);
	void closeSocket(socket_id_t s);

	// svr
	bool bindAndListen(socket_id_t svr_listen_sid, const std::string& svr_ip_or_name, int svr_port);
	bool accept(socket_id_t svr_listen_sid, socket_id_t* svr_tran_sid);

	// client
	bool connect(socket_id_t sid, const std::string& svr_ip_or_name, int svr_port);
	void disconnect(socket_id_t sid);

	// transport
	bool send(socket_id_t sid, const byte_t* data, size_t data_len);
	bool recv(socket_id_t sid, byte_t* buf, size_t buf_len, size_t* recv_len);




private:
	bool __getSocketBySid(socket_id_t sid, socket_t* socket);

	typedef std::map<socket_id_t, socket_t> SidToSocketMap;
	SidToSocketMap m_sid_2_socket;
	socket_id_t m_sid_seed;
	Mutex m_mutex;
};





S_NAMESPACE_END
#endif


