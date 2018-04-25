#ifndef S_SOCKET_API_WIN_H_
#define S_SOCKET_API_WIN_H_
#include "../../comm/comm.h"
#if defined(S_OS_WIN)
#include <vector>
#include <stdio.h>
#include "../../thread/threadLib.h"
#include "../socketApi.h"
#define _WINSOCK_DEPRECATED_NO_WARNINGS
S_NAMESPACE_BEGIN




class TcpSocketCallbackApi : public ITcpSocketCallbackApi, private IMessageLoopHandler
{
public:
    TcpSocketCallbackApi();
	~TcpSocketCallbackApi();


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
	enum __EMsgType
	{
		__EMsgType_onClientConnected = -735141,
		__EMsgType_onClientDisconnected,
		__EMsgType_onClientRecvData,
		__EMsgType_onClientSendDataEnd,

		__EMsgType_onSvrListenSocketListened,
		__EMsgType_onSvrListenSocketAccepted,
		__EMsgType_onSvrListenSocketClosed,

		__EMsgType_onSvrTranSocketRecvData,
		__EMsgType_onSvrTranSocketSendDataEnd,
		__EMsgType_onSvrTransSocketClosed,
	};

    class __SocketCtx
    {
    public:
        __SocketCtx() {
            m_sid = 0;
			m_ref_listen_sid = 0;
            m_socket = INVALID_SOCKET;
        }

        socket_id_t m_sid;
        SOCKET m_socket;
        ETcpSocketType m_socket_type;
		socket_id_t m_ref_listen_sid; // svr_tran ref to listen_sid
        CreateClientSocketParam m_client_param;
        CreateSvrSocketParam m_svr_param;
    };

    class __ClientRun : public IThreadRun
    {
    public:
        __ClientRun(void* msg_target, MessageLooper* notify_looper, socket_id_t sid, SOCKET s, const std::string& svr_ip_or_name, int svr_port);

        virtual void run();
        virtual void stop();
        bool startSend(const byte_t* data, size_t len);

    private:
        void __run();
        bool __onSendEvent();
        bool __onReadEvent();
        void __clearSession();
		void __postMsgToTarget(Message* msg);
		void __postDisconnectedMsg();

        bool m_is_exit;
        std::mutex m_mutex;
        
        socket_id_t m_sid;
        SOCKET m_socket;
        MessageLooper* m_callback_looper;
        std::string m_svr_ip;
        int m_svr_port;

        WSAEVENT m_break_event;
        WSAEVENT m_socket_event;
		std::vector<Binary*> m_send_datas;
		void* m_msg_target;
    };

    class __SvrListenRun : public IThreadRun
    {
    public:
        __SvrListenRun(MessageLooper* notify_looper, void* notify_target, SOCKET s, socket_id_t sid, const std::string& svr_ip_or_name, int svr_port);

        virtual void run();
        virtual void stop();

    private:
        void __run();
		void __postMsgToTarget(Message* msg);

		bool m_is_exit;
		std::mutex m_mutex;
        SOCKET m_socket;
        socket_id_t m_sid;
        std::string m_svr_ip;
        int m_svr_port;
		MessageLooper* m_callback_looper;
		void* m_notify_target;
    };
    
    class __SvrTransRun : public IThreadRun
    {
    public:
        __SvrTransRun(SOCKET s, socket_id_t sid, MessageLooper* notify_looper, void* notify_target);

        virtual void run();
        virtual void stop();
        bool startSend(const byte_t* data, size_t len);

    private:
        void __run();
        bool __onSendEvent();
        bool __onReadEvent();
		void __postMsgToTarget(Message * msg);
        void __clearSession();

        bool m_is_exit;
        std::mutex m_mutex;

		MessageLooper* m_notify_looper;
		void* m_notify_target;
        SOCKET m_socket;
        socket_id_t m_sid;

        WSAEVENT m_break_event;
        WSAEVENT m_socket_event;
        Binary m_dataToSend;
        bool m_isSending;
#define SOCKET_API_SVR_RECV_BUF_SIZE (60 * 1024)
		byte_t m_recv_buf[SOCKET_API_SVR_RECV_BUF_SIZE];
    };

	// IMessageLoopHandler
	virtual void onMessage(Message * msg, bool* isHandled) override;
	virtual void onMessageTimerTick(uint64_t timer_id, void * user_data) override;
	
	void __onClientConnectedMsg(Message* msg);
	void __onClientDisconnectedMsg(Message* msg);
	void __onClientRecvDataMsg(Message* msg);
	void __onClientSendDataEndMsg(Message* msg);

	void __onSvrListenSocketListenedMsg(Message* msg);
	void __onSvrListenSocketAcceptedMsg(Message* msg);
	void __onSvrListenSocketClosedMsg(Message* msg);

	void __onSvrTranSocketRecvDataMsg(Message* msg);
	void __onSvrTranSocketSendDataEndMsg(Message* msg);
	void __onSvrTransSocketClosedMsg(Message* msg);


	void __stopSocketByCtx(__SocketCtx* ctx);
	void __stopClientSocket(socket_id_t client_sid);
	void __releaseClientSocket(socket_id_t client_sid);
	void __stopSvrListenSocket(socket_id_t svr_listen_sid);
	void __releaseSvrListenSocket(socket_id_t svr_listen_sid);
	void __stopSvrTranSocket(socket_id_t svr_tran_sid);
	void __releaseSvrTranSocket(socket_id_t svr_listen_sid);
	void __releaseThread(socket_id_t client_sid);
	void __postMsgToTarget(Message* msg, __SocketCtx * ctx);

	__SocketCtx* __getClientCtxById(socket_id_t sid);
	__SocketCtx* __getSvrListenCtxById(socket_id_t sid);
	__SocketCtx* __getSvrTranCtxById(socket_id_t sid);
    Thread* __getThreadById(socket_id_t sid);


    Mutex m_mutex;
	MessageLooper* m_work_looper;
	MessageLoopThread* m_work_thread;
	typedef std::map<socket_id_t, __SocketCtx*> CtxMap;
	CtxMap m_client_ctx_map;
	CtxMap m_svr_listen_ctx_map;
	CtxMap m_svr_tran_ctx_map;
    std::map<socket_id_t, Thread*> m_socket_thread_vector;
    int64_t m_sid_seed;
};




S_NAMESPACE_END
#endif //S_OS_WIN
#endif //S_SOCKET_API_WIN_H_



