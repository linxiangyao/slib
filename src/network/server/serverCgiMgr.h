#ifndef S_SERVER_CGI_MGR_H_
#define S_SERVER_CGI_MGR_H_
#include "serverCgi.h"
#include <set>
SSVR_NAMESPACE_BEGIN



class ServerCgiMgr : public IMessageLoopHandler, public ServerNetwork::ICallback
{
#define ServerCgiMgr_MSG_TYPE_BEGIN 815657

public:
	typedef ServerCgi::RecvPack RecvPack;

	enum EMsgType
	{
		EMsgType_sessionTimeOut = ServerCgiMgr_MSG_TYPE_BEGIN + 1,
	};

	class ICallback
	{
	public:
		virtual ~ICallback() {}
		virtual void onServerCgiMgr_sessionCreated(socket_id_t sid, session_id_t ssid) = 0;
		virtual void onServerCgiMgr_sessionClosed(socket_id_t sid, session_id_t ssid) = 0;
		virtual void onServerCgiMgr_recvC2sReqPack(std::unique_ptr<RecvPack>* recv_pack_ptr) = 0;
		virtual void onServerCgiMgr_recvC2sNotifyPack(std::unique_ptr<RecvPack>* recv_pack_ptr) = 0;
	};

	class InitParam
	{
	public:
		InitParam() { m_callback = NULL; m_network = NULL; m_work_looper = NULL; m_session_time_out_second = 60; }

		ICallback* m_callback;
		MessageLooper* m_work_looper;
		ServerNetwork* m_network;
		int m_session_time_out_second;
		std::vector<ServerCgiInfo> m_cgi_infos;
	};


	ServerCgiMgr();
	~ServerCgiMgr();
	bool init(const InitParam& param);

	void closeSession(session_id_t ssid);
	void refreashSessionLife(session_id_t ssid);
	bool startCgi(ServerCgi* cgi);
	void stopCgi(ServerCgi* cgi);



private:
	class __SessionCtx
	{
	public:
		__SessionCtx() { m_refresh_life_time_ms = 0; }
		~__SessionCtx() { }

		session_id_t m_ssid;
		socket_id_t m_sid;
		uint64_t m_refresh_life_time_ms;
	};
	typedef std::map<session_id_t, __SessionCtx*> SessionIdToSessionMap;


	virtual void onServerNetwork_clientConnected(ServerNetwork * network, socket_id_t sid) override;
	virtual void onServerNetwork_clientDisconnected(ServerNetwork * network, socket_id_t sid) override;
	virtual void onServerNetwork_sendPackToClientEnd(ServerNetwork * network, socket_id_t sid, ServerNetwork::EErrCode err_code, uint64_t send_pack_id, ServerNetwork::RecvPack * recv_pack) override;
	virtual void onServerNetwork_recvPackFromClient(ServerNetwork * network, socket_id_t sid, ServerNetwork::RecvPack * recv_pack) override;
	void onMessage(Message* msg, bool* isHandled) override;
	void onMessageTimerTick(uint64_t timer_id, void* user_data) override;

	__SessionCtx* __createSession(session_id_t ssid, socket_id_t sid);
	int __getCgiInfoIndexByRecvCmdType(uint32_t cmd_type);
	int __getCgiInfoIndexSendCmdType(uint32_t cmd_type);
	int __getCgiIndexBySendPackId(uint64_t send_pack_id);

	
	InitParam m_init_param;
	SessionIdToSessionMap m_session_ctx_map;
	std::vector<ServerCgi*> m_cgis;
	uint64_t m_timer_id;
};




SSVR_NAMESPACE_END
#endif

