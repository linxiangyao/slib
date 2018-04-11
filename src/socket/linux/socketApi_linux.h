#ifndef S_SOCKET_API_LINUX_H_
#define S_SOCKET_API_LINUX_H_
#include "../../comm/comm.h"
//#define S_OS_LINUX
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
#include "../../thread/threadLib.h"
#include "../socketApi.h"
S_NAMESPACE_BEGIN


 

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
    bool __getSocketBySid(int64_t socket_id, int* socket);

	typedef std::map<int64_t, int> SidToSocketMap;
	SidToSocketMap m_sid_2_socket;
    int64_t m_sid_seed;
    Mutex m_mutex;
};





// callback api ----------------------------------------------------------------------------------------------------------
//class __ClientThreadRunMsg_CmdConnectSvr;
//class __ClientThreadRunMsg_SendDataToSvr;
//class __ClientThreadRun;
#define INVALID_SOCKET 0

class TcpSocketCallbackApi : public ITcpSocketCallbackApi, private IMessageHandler
{
public:
    TcpSocketCallbackApi();
    virtual ~TcpSocketCallbackApi();
    
   // ITcpSocketCallbackApi
	virtual bool init(MessageLooper* work_looper);

	// client
	virtual bool createClientSocket(socket_id_t* client_sid, const CreateClientSocketParam& param);
	virtual void releaseClientSocket(socket_id_t client_sid);
	virtual bool startClientSocket(socket_id_t client_sid);
	virtual void stopClientSocket(socket_id_t client_sid);
	virtual bool sendDataFromClientSocketToSvr(socket_id_t client_sid, const byte_t* data, size_t data_len);
	virtual std::string getClientSocketSvrIp(socket_id_t client_sid);
	virtual uint32_t getClientSocketSvrPort(socket_id_t client_sid);

	// svr listen
	virtual bool createSvrListenSocket(socket_id_t* svr_listen_sid, const CreateSvrSocketParam& param);
	virtual void releaseSvrListenSocket(socket_id_t svr_listen_sid);
	virtual bool startSvrListenSocket(socket_id_t svr_listen_sid);
	virtual void stopSvrListenSocket(socket_id_t svr_listen_sid);
	virtual std::string getSvrListenSocketIp(socket_id_t svr_listen_sid);
	virtual uint32_t getSvrListenSocketPort(socket_id_t svr_listen_sid);

	// svr tran
	virtual void stopSvrTranSocket(socket_id_t svr_tran_sid);
	virtual bool sendDataFromSvrTranSocketToClient(socket_id_t svr_tran_sid, const byte_t* data, size_t data_len);

    
private:
	class __SocketCtx
    {
    public:
		__SocketCtx() { m_socket = INVALID_SOCKET; m_sid = 0; m_ref_listen_sid = 0; }
        ~__SocketCtx() { }
        
        ETcpSocketType m_socket_type;
        int m_socket;
        socket_id_t m_sid;
		socket_id_t m_ref_listen_sid;
        CreateClientSocketParam m_client_param;
		CreateSvrSocketParam m_svr_param;
    };


	// IMessageHandler
	virtual void onMessage(Message * msg, bool* isHandled);

	void __onMsg_ClientSocketConnected(Message* msg);
	void __onMsg_ClientSocketRecvData(Message* msg);
	void __onMsg_ClientSocketSendDataEnd(Message* msg);
	void __onMsg_ClientSocketClosed(Message* msg);

	void __onMsg_SvrListenSocketListened(Message* msg);
	void __onMsg_SvrListenSocketAccepted(Message* msg);
	void __onMsg_SvrListenSocketClosed(Message* msg);

	void __onMsg_SvrTranSocketRecvData(Message* msg);
	void __onMsg_SvrTranSocketSendDataEnd(Message* msg);
	void __onMsg_SvrTransSocketClosed(Message* msg);



	bool __initClientThread();
	bool __initSvrThread();
	void __stopClientSocket(socket_id_t client_sid);
	void __releaseClientSocket(socket_id_t client_sid);
	void __stopSvrListenSocket(socket_id_t svr_listen_sid);
	void __releaseSvrListenSocket(socket_id_t svr_listen_sid);
	void __stopSvrTranSocket(socket_id_t svr_tran_sid);
	void __releaseSvrTranSocket(socket_id_t svr_listen_sid);

	void __postMsgToTarget(Message* msg, __SocketCtx * ctx);
	__SocketCtx* __getClientCtxById(socket_id_t sid);
	__SocketCtx* __getClientCtxBySocket(int s);
	__SocketCtx* __getSvrListenCtxById(socket_id_t sid);
	__SocketCtx* __getSvrListenCtxBySocket(int s);
	__SocketCtx* __getSvrTranCtxById(socket_id_t sid);
	__SocketCtx* __getSvrTranCtxBySocket(int s);
    




	MessageLooper* m_work_looper;
    Mutex m_mutex;
	int64_t m_sid_seed;
	typedef std::map<socket_id_t, __SocketCtx*> CtxMap;
	CtxMap m_client_ctx_map;
	CtxMap m_svr_listen_ctx_map; 
	CtxMap m_svr_tran_ctx_map;
	IThread* m_client_thread;
	IThread* m_svr_thread;
};




S_NAMESPACE_END
#endif //defined(S_OS_LINUX) | defined(S_OS_MAC)
#endif //S_SOCKET_API_LINUX_H_















//std::map<int64_t, __SocketCtx*> m_sid_2_socket;
//std::vector<Thread*> m_acceptThreads;
//std::vector<Thread*> m_transThreads;
//std::vector<Thread*> m_clientThreads;
//   
//   class __SocketPair
//   {
//   public:
//       int64_t m_sid;
//       int m_socket;
//   };
//   
//   
//   
//   class __AcceptRun : public IThreadRun
//   {
//   public:
//       __AcceptRun(TcpSocketCallbackApi* api);
//       virtual void run();
//       virtual void stop();
//       void addAcceptSocket(const __SocketPair& s);
//       bool sendDataToClient(socket_id_t client_sid, const byte_t* data, size_t data_len);
//       
//       
//   private:
//       bool m_is_exit;
//       Mutex m_mutex;
//       std::vector<__SocketPair> m_sockets;
//       TcpSocketCallbackApi* m_api;
//       int m_pipe[2];
//   };
//   
//   
//   
//   class __ClientRun : public IThreadRun
//   {
//   public:
//       virtual void run();
//       virtual void stop();
//       bool addClientSocket(const __SocketPair& s);
//       bool sendDataToSvr(socket_id_t client_sid, const byte_t* data, size_t data_len);
//       
//       
//   private:
//       enum __EClientConnectState
//       {
//           __EClientConnectState_none,
//           __EClientConnectState_connecting,
//           __EClientConnectState_connected,
//       };
//       
//       class __ClientCtx
//       {
//       public:
//           __ClientCtx() { m_sid = 0; m_socket = 0; m_connect_state = __EClientConnectState_none; }
//           
//           int64_t m_sid;
//           int m_socket;
//           std::string m_svr_ip_or_name;
//           int m_svr_port;
//           __EClientConnectState m_connect_state;
//           Binary m_data_to_send;
//       };
//       
//       
//       bool m_is_exit;
//       Mutex m_mutex;
//       std::vector<__ClientCtx*> m_clients;
//       int m_pipe[2];
//   };
//   
//   
//   
//   class __TransRun : public IThreadRun
//   {
//   public:
//       virtual void run();
//       virtual void stop();
//       
//       bool addClientSocket(const __SocketPair& s);
//       bool sendDataToSvr(socket_id_t client_sid, const byte_t* data, size_t data_len);
//       
//       
//   private:
//       bool m_is_exit;
//       Mutex m_mutex;
//       std::vector<__SocketPair> m_sockets;
//       int m_pipe[2];
//   };
//   
//   
//   
//   
//   
//   void __onAcceptClient(socket_id_t accept_sid, int trans_socket);
//   __SocketCtx* __getSocketCtxById(int64_t socket_id);
//   void __addSocketCtx(__SocketCtx* ctx);
//
