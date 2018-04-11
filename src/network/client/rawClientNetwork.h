//#ifndef S_RAW_CLIENT_NETWORK_H_
//#define S_RAW_CLIENT_NETWORK_H_
//#include "clientComm.h"
//#include "clientNetSpeedTester.h"
//SCLIENT_NAMESPACE_BEGIN
//
//
//
//
//
//class RawClientNetwork : public IMessageHandler
//{
//public:
//#define S_RAW_CLIENT_NETWORK_MSG_BEGIN 8274561
//	enum EMsgType
//	{
//		EMsgType_clientConnected = S_RAW_CLIENT_NETWORK_MSG_BEGIN + 1,
//		EMsgType_clientDisconnected = S_RAW_CLIENT_NETWORK_MSG_BEGIN + 2,
//		EMsgType_recvPackFromSvr = S_RAW_CLIENT_NETWORK_MSG_BEGIN + 3,
//		EMsgType_sendPackToSvrEnd = S_RAW_CLIENT_NETWORK_MSG_BEGIN + 4,
//	};
//
//	class SendPack : public Pack
//	{
//	public:
//		SendPack() { m_id = 0; }
//		uint64_t m_id;
//	};
//
//	class RecvPack : public Pack
//	{
//	public:
//		RecvPack() { m_id = 0; }
//		uint64_t m_id;
//	};
//
//	class SendPackEndMsg : public Message
//	{
//	public:
//		SendPackEndMsg() { m_msg_type = EMsgType_sendPackToSvrEnd; err_code = 0; }
//		~SendPackEndMsg() {}
//		virtual Message* clone() const { SendPackEndMsg* m = new SendPackEndMsg(); *m = *this; return m; }
//
//		int err_code;
//		SendPack m_send_pack;
//	};
//
//	class RecvPackMsg : public Message
//	{
//	public:
//		RecvPackMsg() { m_msg_type = EMsgType_recvPackFromSvr; }
//		virtual Message* clone() const { RecvPackMsg* m = new RecvPackMsg(); *m = *this; return m; }
//
//		RecvPack m_recv_pack;
//	};
//
//	class ClientConnectedMsg : public Message
//	{
//	public:
//		ClientConnectedMsg() { m_msg_type = EMsgType_clientConnected; m_client_sid = 0; }
//		virtual Message* clone() const { ClientConnectedMsg* m = new ClientConnectedMsg(); *m = *this; return m; }
//		socket_id_t m_client_sid;
//	};
//
//	class ClientDisconnectedMsg : public Message
//	{
//	public:
//		ClientDisconnectedMsg() { m_msg_type = EMsgType_clientDisconnected; m_client_sid = 0; }
//		virtual Message* clone() const { ClientDisconnectedMsg* m = new ClientDisconnectedMsg(); *m = *this; return m; }
//		socket_id_t m_client_sid;
//	};
//
//	class InitParam
//	{
//	public:
//		InitParam() { ; m_sapi = NULL; m_work_looper = NULL; m_notify_looper = NULL; m_notify_target = NULL; m_max_send_pack_count_in_queue = 1000; }
//
//		std::string m_svr_ip_or_name;
//		uint32_t m_svr_port;
//		ITcpSocketCallbackApi* m_sapi;
//		MessageLooper* m_work_looper;
//		MessageLooper* m_notify_looper;
//		void* m_notify_target;
//		size_t m_max_send_pack_count_in_queue;
//	};
//
//	RawClientNetwork();
//	~RawClientNetwork();
//	bool init(const InitParam& param);
//
//	bool connect();
//	void disconnect();
//	bool sendPackToSvr(SendPack* send_pack);
//	void cancelSendPackToSvr(uint64_t send_pack_id);
//
//	std::string getSvrIp();
//	uint32_t getSvrPort();
//
//
//private:
//	enum __EMsgType
//	{
//		__EMsgType_sendPacks,
//	};
//
//	virtual void onMessage(Message * msg, bool * isHandled);
//
//	void __onMsgTcpSocketClientConnected(ITcpSocketCallbackApi::ClientSocketConnectedMsg* msg);
//	void __onMsgTcpSocketClientDisconnected(ITcpSocketCallbackApi::ClientSocketDisconnectedMsg* msg);
//	void __onMsgTcpSocketClientSendDataEnd(ITcpSocketCallbackApi::ClientSocketSendDataEndMsg* msg);
//	void __onMsgTcpSocketClientRecvData(ITcpSocketCallbackApi::ClientSocketRecvDataMsg* msg);
//	void __onMsgSendPack(Message* msg);
//
//	void __doSendPack();
//	void __postMessageToSelf(Message* msg);
//	void __postMessageToTarget(Message* msg);
//	int __getSendPackIndexBySendPackId(uint64_t send_pack_id);
//
//
//	uint64_t m_pack_id_seed;
//	SimpleTcpPacker* m_packer;
//	Mutex m_mutex;
//	socket_id_t m_sid;
//	InitParam m_init_param;
//	bool m_is_connected;
//	bool m_is_sending;
//	std::vector<SendPack*> m_send_packs;
//	Binary m_recv_data;
//};
//
//
//
//
//SCLIENT_NAMESPACE_END
//#endif
