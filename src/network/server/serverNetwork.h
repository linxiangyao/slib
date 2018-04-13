#ifndef S_SERVER_NETWORK_H_
#define S_SERVER_NETWORK_H_
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
		Binary m_send_whole_pack_bin; // include pack head and body
		//uint32_t m_expect_resp_seq;
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

	class ICallback
	{
	public:
		virtual ~ICallback() {}
		virtual void onServerNetwork_clientConnected(ServerNetwork* network, socket_id_t sid) = 0;
		virtual void onServerNetwork_clientDisconnected(ServerNetwork* network, socket_id_t sid) = 0;
		virtual void onServerNetwork_sendPackToClientEnd(ServerNetwork* network, socket_id_t sid, EErrCode err_code, uint64_t send_pack_id, RecvPack* recv_pack) = 0;
		virtual void onServerNetwork_recvPackFromClient(ServerNetwork* network, socket_id_t sid, RecvPack* recv_pack) = 0;
	};

	class InitParam
	{
	public:
		InitParam() { m_sapi = NULL; m_work_looper = NULL; m_callback = NULL; m_svr_port = 0; m_unpacker = NULL; }

		ITcpSocketCallbackApi* m_sapi;
		MessageLooper* m_work_looper;
		ICallback* m_callback;
		std::string m_svr_ip_or_name;
		uint32_t m_svr_port;
		IUnpacker* m_unpacker;
		std::map<uint32_t, ServerCgiInfo> m_send_cmd_type_to_cgi_info_map;
	};




	ServerNetwork();
	~ServerNetwork();
	bool init(const InitParam& init_param);

	bool start();
	void stop();

	SendPack* newSendPack(socket_id_t sid, session_id_t ssid, uint32_t send_cmd_type, uint32_t send_seq);
	bool sendPackToClient(const SendPack& send_pack);
	void cancelSendPackToClient(socket_id_t client_sid, uint64_t send_pack_id);
	void disconnectClient(socket_id_t client_sid);






private:
	enum __EMsgType
	{
		__EMsgType_sendPack,
	};

	class __SendPackInfo
	{
	public:
		__SendPackInfo() { m_send_pack_start_time_ms = 0; m_is_cancel = false; m_is_sent = false; m_cgi_info = NULL; }
		
		uint64_t m_send_pack_start_time_ms;
		bool m_is_cancel;
		bool m_is_sent;
		SendPack m_pack;
		ServerCgiInfo* m_cgi_info;
	};
	
	class __Client
	{
	public:
		__Client() { m_sid = 0; m_sending_index = -1; }
		int getSpiIndexBySendPackId(uint64_t send_pack_id);
		int getSpiIndexBySeq(uint32_t seq);

		socket_id_t m_sid;
		std::vector<__SendPackInfo*> m_send_pack_infos;
		int m_sending_index;
		Binary m_recv_data;
	};

	typedef std::map<socket_id_t, __Client*> SidToClientMap;


	// IMessageHandler
	virtual void onMessage(Message * msg, bool* isHandled);
	// IMessageTimerHandler
	virtual void onMessageTimerTick(uint64_t timer_id, void* user_data);

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
	void __checkTimeOutSendPacks();
	void __doSendPacks();
	bool __doSendClientPacks(__Client* c);

	void __postSendPackMsgToSelf();
	void __postClientConnectedMsgToTarget(socket_id_t client_sid);
	void __postClientDisconnectedMsgToTarget(socket_id_t client_sid);
	__Client* __getClientBySid(socket_id_t sid);
	ServerCgiInfo* __getServerCgiInofoBySendCmdType(uint32_t send_cmd_type);



	static uint64_t s_send_pack_id_seed;
	InitParam m_init_param;
	SidToClientMap m_sid_to_client_map;
	bool m_is_running;
	socket_id_t m_listen_sid;
	uint64_t m_timer_id;
};



SSVR_NAMESPACE_END
#endif

