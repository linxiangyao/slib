#ifndef S_SOCKET_API_LINUX_H_
#define S_SOCKET_API_LINUX_H_
#include "../socketApi.h"
//#define S_OS_LINUX
#if defined(S_OS_LINUX) | defined(S_OS_MAC) | defined(S_OS_ANDROID)
S_NAMESPACE_BEGIN


 


class TcpSocketCallbackApi : public ITcpSocketCallbackApi, private IMessageLoopHandler
{
public:
    TcpSocketCallbackApi();
    virtual ~TcpSocketCallbackApi();
    
   // ITcpSocketCallbackApi
	virtual bool init(MessageLooper* work_looper = nullptr);

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
        socket_t m_socket;
        socket_id_t m_sid;
		socket_id_t m_ref_listen_sid;
        CreateClientSocketParam m_client_param;
		CreateSvrSocketParam m_svr_param;
    };


	// IMessageLoopHandler
	virtual void onMessage(Message * msg, bool* isHandled) override;
	virtual void onMessageTimerTick(uint64_t timer_id, void * user_data) override;

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
	__SocketCtx* __getClientCtxBySocket(socket_t s);
	__SocketCtx* __getSvrListenCtxById(socket_id_t sid);
	__SocketCtx* __getSvrListenCtxBySocket(socket_t s);
	__SocketCtx* __getSvrTranCtxById(socket_id_t sid);
	__SocketCtx* __getSvrTranCtxBySocket(socket_t s);
    




	MessageLooper* m_work_looper;
	MessageLoopThread* m_work_thread;
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
#endif
#endif //S_SOCKET_API_LINUX_H_

