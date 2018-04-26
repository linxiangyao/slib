#ifndef S_SERVER_NETWORK_H_
#define S_SERVER_NETWORK_H_
#include <vector>
#include "serverComm.h"
SSVR_NAMESPACE_BEGIN




/*
server network.

server network bind one listen port.
if client is connected/disconnected, network will post connected/disconnected message.
if client send pack to server ok, server network will post recv_pack_from_client message.
if server send pack to client ok. server network will post send_pack_to_client_end message.




NOTE:
	one server network bind to one port. if there are two ports, there are two server networks.

	network is thread unsafe(for good perfromance).
	network should be called in m_work_looper thread(typical is main thread), and network will callback in the same thread.
	there may be many networks in one thread.

	one ITcpSocketCallbackApi may be used by many networks.
	ITcpSocketCallbackApi may be in the same thread of network, or may not be.
*/
class ServerNetwork : public IMessageHandler, public IMessageTimerHandler
{
public:
	enum EErrCode
	{
		EErrCode_ok,
		EErrCode_system,
	};

	class SendPack
	{
	private:
		SendPack() { m_send_pack_id = 0; m_sid = 0; m_send_cmd_type = 0; m_send_seq = 0; }
		friend class ServerNetwork;

	public:
		std::string toOverviewString() const
		{
			return std::string()
				+ "send_pack_id=" + StringUtil::toString(m_send_pack_id)
				+ ", sid=" + StringUtil::toString(m_sid) 
				+ ", ssid=" + m_ssid.toString()
				+ ", send_cmd_type=" + StringUtil::toString(m_send_cmd_type) 
				+ ", send_seq=" + StringUtil::toString(m_send_seq)
				+ ", len=" + StringUtil::toString(m_send_whole_pack_bin.getLen());
		}

		uint64_t m_send_pack_id;
		socket_id_t m_sid;
		session_id_t m_ssid;
		uint32_t m_send_cmd_type;
		uint32_t m_send_seq;
		Binary m_send_whole_pack_bin; // raw data which include pack head and body. it wll be sent to socket.
	};

	class IRecvPackExt
	{
	public:
		virtual ~IRecvPackExt() {}
	};

	class RecvPack
	{
	public:
		RecvPack() { m_recv_ext = NULL; }
		~RecvPack() { delete m_recv_ext; }

		std::string toOverviewString() const
		{
			return std::string()
				+ "sid=" + StringUtil::toString(m_sid)
				+ ", ssid=" + m_ssid.toString()
				+ ", recv_cmd_type=" + StringUtil::toString(m_recv_cmd_type)
				+ ", recv_seq=" + StringUtil::toString(m_recv_seq)
				+ ", recv_ext=" + StringUtil::toString((uint64_t)m_recv_ext);
		}

		socket_id_t m_sid;
		session_id_t m_ssid;
		uint32_t m_recv_cmd_type;
		uint32_t m_recv_seq;
		IRecvPackExt* m_recv_ext; // user data, same with UnpackResult.m_recv_ext
	};

	class UnpackResult
	{
	public:
		UnpackResult() { m_result_type = EUnpackResultType_fail; m_recv_cmd_type = 0; m_recv_seq = 0; m_recv_ext = NULL; m_unpack_raw_data_len = 0; }
		EUnpackResultType m_result_type;
		uint32_t m_recv_cmd_type;
		uint32_t m_recv_seq;
		session_id_t m_ssid;
		size_t m_unpack_raw_data_len;
		IRecvPackExt* m_recv_ext; // user data, same with RecvPack.m_recv_ext
	};

	class IUnpacker
	{
	public:
		virtual ~IUnpacker() {}
		virtual void unpackServerRecvPack(const byte_t* raw_data, size_t raw_data_len, UnpackResult* result) = 0;
	};

	class ServerCgi
	{
	public:
		ServerCgi()
		{
			m_send_pack = nullptr;
			m_recv_pack = nullptr;
			m_start_ms = 0;
			m_end_ms = 0;
			m_err_code = ServerNetwork::EErrCode_ok;
		}

		virtual ~ServerCgi()
		{
			delete m_send_pack;
			delete m_recv_pack;
		}

		// to implement
		virtual const ServerCgiInfo& getServerCgiInfo() const = 0;

		RecvPack*	getRecvPack() { return m_recv_pack; }
		SendPack*	getSendPack() { return m_send_pack; }
		EErrCode	getErrCode() { return m_err_code; }
		uint64_t	getStartMs() { return m_start_ms; }
		uint64_t	getEndMs() { return m_end_ms; }
		bool		getIsCgiSuccess() { return m_err_code == ServerNetwork::EErrCode_ok; }
		session_id_t getSessionId();

		void setErrCode(EErrCode err_code) { m_err_code = err_code; }
		void setStartMs(uint64_t ms) { m_start_ms = ms; }
		void setEndMs(uint64_t ms) { m_end_ms = ms; }
		void setSendPack(SendPack* send_pack);
		void setRecvPack(RecvPack* recv_pack);


	protected:
		// to override
		virtual void onSetRecvPackEnd() {}


	private:
		SendPack* m_send_pack;
		RecvPack* m_recv_pack;
		ServerNetwork::EErrCode m_err_code;
		uint64_t m_start_ms;
		uint64_t m_end_ms;
	};

	class ICallback
	{
	public:
		virtual ~ICallback() {}
		virtual void onServerNetwork_clientConnected(ServerNetwork* network, socket_id_t sid) = 0;
		virtual void onServerNetwork_clientDisconnected(ServerNetwork* network, socket_id_t sid) = 0;
		virtual void onServerNetwork_recvC2sReqPack(ServerNetwork* network, std::unique_ptr<RecvPack>* recv_pack_ptr) = 0;
		virtual void onServerNetwork_recvC2sNotifyPack(ServerNetwork* network, std::unique_ptr<RecvPack>* recv_pack_ptr) = 0;
		virtual void onServerNetwork_cgiDone(ServerNetwork* network, ServerCgi* cgi) = 0;
	};

	class InitParam
	{
	public:
		InitParam() { m_sapi = NULL; m_work_looper = NULL; m_callback = NULL; m_svr_port = 0; m_unpacker = NULL; m_cgi_time_out_ms = 30 * 1000; }

		ITcpSocketCallbackApi* m_sapi;
		MessageLooper* m_work_looper;
		ICallback* m_callback;
		std::string m_svr_ip_or_name;
		uint32_t m_svr_port;
		IUnpacker* m_unpacker;
		std::map<uint32_t, ServerCgiInfo> m_send_cmd_type_to_cgi_info_map;
		uint32_t m_cgi_time_out_ms;
	};




	ServerNetwork();
	~ServerNetwork();
	bool init(const InitParam& init_param);

	bool start();
	void stop();

	SendPack* newSendPack(socket_id_t sid, session_id_t ssid, uint32_t send_cmd_type, uint32_t send_seq);
	bool startCgi(ServerCgi* cgi);
	void stopCgi(ServerCgi* cgi);
	void disconnectClient(socket_id_t client_sid);






private:
	enum __EMsgType
	{
		__EMsgType_sendPack,
	};

	class __CgiCtx
	{
	public:
		__CgiCtx() { m_is_cancel = false; m_is_sent = false; m_cgi = nullptr; }
		
		bool m_is_cancel;
		bool m_is_sent;
		ServerCgi* m_cgi;
	};
	
	class __Client
	{
	public:
		__Client(socket_id_t sid, InitParam* init_param, ServerNetwork* network);
		~__Client();

		void addCgi(ServerCgi* cgi);
		void stopCgi(ServerCgi* cgi);
		bool doSendPack();
		void checkTimeOutCgis();
		int getCgiIndexBySendPackId(uint64_t send_pack_id);
		void onRecvPack(RecvPack* recv_pack);
		void onSendDataEnd();

		void __markCgiDoneByIndex(int cgi_indx, EErrCode err_code);
		int __getCgiIndexBySendPackSeq(uint32_t send_pack_seq);
		ServerCgiInfo* __getServerCgiInofoByRecvCmdType(uint32_t recv_cmd_type);

		socket_id_t m_sid;
		InitParam* m_init_param;
		ServerNetwork* m_network;
		std::vector<__CgiCtx*> m_cgi_ctxs;
		int m_sending_index;
		Binary m_recv_data;
	};

	typedef std::map<socket_id_t, __Client*> SidToClientMap;



	virtual void onMessage(Message * msg, bool* isHandled) override;
	virtual void onMessageTimerTick(uint64_t timer_id, void* user_data) override;
	void __onSvrStartedMsg(ITcpSocketCallbackApi::SvrListenSocketStartedMsg* msg);
	void __onSvrStoppedMsg(ITcpSocketCallbackApi::SvrListenSocketStoppedMsg* msg);
	void __onSvrAcceptMsg(ITcpSocketCallbackApi::SvrListenSocketAcceptedMsg* msg);
	void __onSvrTranStoppedMsg(ITcpSocketCallbackApi::SvrTranSocketStoppedMsg* msg);
	void __onSvrTranRecvDataMsg(ITcpSocketCallbackApi::SvrTranSocketRecvDataMsg* msg);
	void __onSvrTranSendDataEndMsg(ITcpSocketCallbackApi::SvrTranSocketSendDataEndMsg* msg);
	void __onSendPackMsg();

	void __stop();
	void __stopClient(socket_id_t client_sid);
	void __stopAllClients();
	void __checkTimeOutCgis();
	void __doSendPacks();
	void __postSendPackMsgToSelf();
	__Client* __getClientBySid(socket_id_t sid);
	ServerCgiInfo* __getServerCgiInofoBySendCmdType(uint32_t send_cmd_type);


	static uint64_t s_send_pack_id_seed;
	InitParam m_init_param;
	SidToClientMap m_sid_to_client_map;
	bool m_is_running;
	socket_id_t m_listen_sid;
	uint64_t m_timer_id;
};



typedef ServerNetwork::ServerCgi ServerCgi;






SSVR_NAMESPACE_END
#endif

