#ifndef S_CLIENT_NETWORK_H_
#define S_CLIENT_NETWORK_H_
#include "clientComm.h"
#include "clientNetSpeedTester.h"
SCLIENT_NAMESPACE_BEGIN



/*
client network.

dns.
network speed test, choice fastest svr.
auto connect.
auto retry send pack.
provide timeout funtion.
provide priority funtion.
flow limity.



NOTE:
	1. client network & server
	one sever may have many ips and ports.
	one client network bind to one server. if there are two servers, there are two client networks. 

	2. thread model:
	network is thread unsafe(for good perfromance).
	network should run in one thread, that means: network should be called in m_work_looper thread(typical is main thread), and network will callback in the same thread.
	there may be many networks in one thread.

	3. the cgi order is uncertain. 
	although the network send cgis one by one(e.g. send a_req, then send b_req), but the order of cgi resp is uncertain(e.g. recv b_resp, then recv a_resp), and sometimes can't recv resp.

	4. client network & TcpSocketCallbackApi
	one TcpSocketCallbackApi may be used by many networks.
	TcpSocketCallbackApi may be in the same thread of network, or may not be.



example:
	you should pack the send data and unpack the recv data.
	you should inherit ClientCgi, and define cgi info.


	ClientNetwork* g_netwrok = nullptr;
	StPacker* g_packer = nullptr; // pack/unpack
	int g_send_seq = 0;
	uint64_t g_send_pack_id = 0;


	ClientNetwork::SendPack* newSendPackAndPack(uint32_ send_cmd_type, const byte_t* send_body, size_t send_body_len)
	{
		ClientNetwork::SendPack* send_pack = g_network->newSendPack(++g_send_pack_id, send_cmd_type, ++g_send_seq);

		StPacker::Pack p;
		p.m_head.m_cmd_type = send_pack->m_send_cmd_type;
		p.m_head.m_err = 0;
		p.m_head.m_seq = send_pack->m_send_seq;
		p.m_head.m_uin = 0;
		memcpy(p.m_head.m_session_id_bin, m_session_id_bin, 16);
		p.m_body.attach((byte_t*)send_body, send_body_len);
		g_packer->packToBin(p, &(send_pack->m_send_whole_pack_bin));
		p.m_body.detach();

		return send_pack;
	}


	class __ClientCgi_SayHello : public ClientCgi
	{
	public:
		static const ClientCgiInfo & s_getCgiInfo()
		{
			s_cgi_info.m_cgi_type = EClientCgiType_c2sReq_s2cResp;
			s_cgi_info.m_send_cmd_type = 10001;
			s_cgi_info.m_recv_cmd_type = 20001;
			return s_cgi_info;
		}

		virtual const ClientCgiInfo & getCgiInfo() const { return s_getCgiInfo(); }

		bool initSendPack()
		{
			m_c2s_req_body = "hello";
			ClientNetwork::SendPack* send_pack = newSendPackAndPack(getCgiInfo().m_send_cmd_type, (const byte_t*)m_c2s_req_body.c_str(), m_c2s_req_body.size());
			setSendPack(send_pack);
		}

		std::string m_c2s_req_body;
		std::string m_s2c_resp_body;


	private:
		virtual void onSetRecvPackEnd()
		{
			ClientNetwork::RecvPack* recv_pack = getRecvPack();
			StPacker::Pack* st_pack = (StPacker::Pack*)recv_pack->m_recv_ext;
			m_s2c_resp_body = (const char*)st_pack->m_body.getData();
		}

		static ClientCgiInfo s_cgi_info;
	};



	class MyApp : public  ClientNetwork::ICallback
	{
	public:
		MyApp()
		{
			...
			__ClientCgi_SayHello* cgi = new __ClientCgi_SayHello();
			cgi->initSendPack();
			g_network->startCgi(cgi);
		}

		void onClientNetwork_cgiDone(ClientNetwork* network, ClientCgi* cgi)
		{
			if(cgi->getCgiInfo().m_send_cmd_type == 10001)
			{
				__ClientCgi_SayHello* c = (__ClientCgi_SayHello*)cgi;
				printf("recv %s\n", c->m_s2c_resp_body.c_str());
			}
		}
	}
*/
class ClientNetwork : public IMessageHandler, public IMessageTimerHandler
{
public:
	enum EErrType
	{
		EErrType_ok = 0,
		EErrType_local = 1,
		EErrType_server = 2,
	};
	
	enum ELocalErrCode
	{ 
		ELocalErrCode_sendPackTimeOutErr = 1,
		ELocalErrCode_sendPackSysErr = 2,
	};

	enum EConnectState
	{
		EConnectState_connecting = 1,
		EConnectState_connected = 2,
		EConnectState_disconnecting = 3,
		EConnectState_disconnected = 4,
	};

	class SendPack
	{
	private:
		SendPack() { m_send_pack_id = 0; m_send_cmd_type = 0; m_send_seq = 0; }
		friend class ClientNetwork;

	public:
		std::string toOverviewString() const
		{
			return std::string() + "send_pack_id=" + StringUtil::toString(m_send_pack_id) 
				+ ", send_cmd_type=" + StringUtil::toString(m_send_cmd_type) 
				+ ", send_seq=" + StringUtil::toString(m_send_seq)
				+ ", len=" + StringUtil::toString(m_send_whole_pack_bin.getLen());
		}

		uint64_t m_send_pack_id;
		uint32_t m_send_cmd_type;
		uint32_t m_send_seq;
		Binary m_send_whole_pack_bin;	// include pack head and body, you should set this value.
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
		//RecvPack* detach() { RecvPack* p = new RecvPack(); *p = *this; this->m_recv_user_data = NULL; }

		uint32_t m_recv_cmd_type;
		uint32_t m_recv_seq;
		IRecvPackExt* m_recv_ext;
	};

	class UnpackResult
	{
	public:
		UnpackResult() { m_result_type = EUnpackResultType_fail; m_recv_cmd_type = 0; m_recv_seq = 0; m_recv_ext = NULL; m_unpack_raw_data_len = 0; }
		EUnpackResultType m_result_type;
		uint32_t m_recv_cmd_type;
		uint32_t m_recv_seq;
		IRecvPackExt* m_recv_ext;
		size_t m_unpack_raw_data_len;
	};

	class IUnpacker
	{
	public:
		virtual ~IUnpacker() {}
		virtual void unpackClientRecvPack(const byte_t* raw_data, size_t raw_data_len, UnpackResult* result) = 0;
	};

	class ClientCgi
	{
	public:
		ClientCgi() { m_send_pack = nullptr; m_recv_pack = nullptr; m_err_type = EErrType_ok; m_err_code = 0; m_start_ms = 0; m_end_ms = 0; m_max_try_count = 1; }
		virtual ~ClientCgi() { delete m_send_pack; delete m_recv_pack; }

		virtual const ClientCgiInfo& getCgiInfo() const = 0;


		RecvPack*	getRecvPack() { return m_recv_pack; }
		SendPack*	getSendPack() { return m_send_pack; }
		EErrType	getErrType() { return m_err_type; }
		int			getErrCode() { return m_err_code; }
		bool		getIsSuccess() { return getErrType() == EErrType_ok && getErrCode() == 0; }
		uint64_t	getStartMs() { return m_start_ms; }
		uint64_t	getEndMs() { return m_end_ms; }
		uint32_t	getMaxTryCount() { return m_max_try_count; }

		void		setSendPack(SendPack* send_pack);
		void		setRecvPack(RecvPack* recv_pack);
		void		setErrType(EErrType err_type) { m_err_code = err_type; }
		void		setErrCode(int err_code) { m_err_code = err_code; }
		void		setStartMs(uint64_t ms) { m_start_ms = ms; }
		void		setEndMs(uint64_t ms) { m_end_ms = ms; }
		void		setMaxTryCount(uint32_t max_try_count) { m_max_try_count = max_try_count; }


	protected:
		virtual void onSetRecvPackEnd() {}


	private:
		SendPack* m_send_pack;
		RecvPack* m_recv_pack;
		EErrType m_err_type;
		int m_err_code;
		uint64_t m_start_ms;
		uint64_t m_end_ms;
		uint32_t m_max_try_count;
	};

	class ICallback
	{
	public:
		virtual ~ICallback() {}
		virtual void onClientNetwork_started(ClientNetwork* network) = 0;
		virtual void onClientNetwork_stopped(ClientNetwork* network) = 0;
		virtual void onClientNetwork_connectStateChanged(ClientNetwork* network, EConnectState state) = 0;
		virtual void onClientNetwork_recvS2cPushPack(ClientNetwork* network, std::unique_ptr<RecvPack>* recv_pack) = 0;
		virtual void onClientNetwork_recvS2cReqPack(ClientNetwork* network, std::unique_ptr<RecvPack>* recv_pack) = 0;
		virtual void onClientNetwork_cgiDone(ClientNetwork* network, ClientCgi* cgi) = 0;
	};

	class SvrInfo
	{
	public:
		SvrInfo() { m_svr_port = 0; }
		std::string m_svr_ip_or_name;
		uint32_t m_svr_port;
	};
	
	class InitParam
	{
	public:
		InitParam()
		{ 
			m_work_looper = nullptr;
			m_callback = nullptr;
			m_sapi = nullptr;
			m_unpacker = nullptr;
			m_dns_resolver = nullptr;
			m_connect_interval_mss.push_back(1);
			m_connect_interval_mss.push_back(1*1000);
			m_connect_interval_mss.push_back(2 * 1000);
			m_connect_interval_mss.push_back(3 * 1000);
			m_connect_interval_mss.push_back(4 * 1000);
			m_connect_interval_mss.push_back(5 * 1000);
			m_is_repeat_last_connect_interval_ms = true;
			m_max_pack_count = 1000;
		}

		MessageLooper* m_work_looper;
		ICallback* m_callback;
		ITcpSocketCallbackApi* m_sapi;
		DnsResolver* m_dns_resolver;
		std::vector<SvrInfo> m_svr_infos;
		std::map<uint32_t, ClientCgiInfo> m_send_cmd_type_to_cgi_info_map;
		IUnpacker* m_unpacker;
		std::vector<int32_t> m_connect_interval_mss;
		bool m_is_repeat_last_connect_interval_ms;
		uint32_t m_max_pack_count;
	};




public:
	ClientNetwork();
	~ClientNetwork();
	bool init(const InitParam& param);

	bool start();
	void stop();

	SendPack* newSendPack(uint64_t send_pack_id, uint32_t send_cmd_type, uint32_t send_seq); // if send_seq == -1, network will generate send_seq
	bool startCgi(ClientCgi* cgi);
	void stopCgi(ClientCgi* cgi);







private:
	enum __EMsgType
	{
		__EMsgType_sendPack = 19837811,
		__EMsgType_notifyStarted,
		__EMsgType_notifyStopped,
		__EMsgType_notifyConectStateChanged,
		__EMsgType_notifyRecvS2cPushPack,
		__EMsgType_notifyRecvS2cReqPack,
		__EMsgType_notifyCgiDone,
	};

	enum __EConnectState
	{
		__EConnectState_connecting,
		__EConnectState_connected,
		__EConnectState_disconnected,

	};

	class __Msg_notifyConectStateChanged;
	class __Msg_notifyRecvS2cPushPack;
	class __Msg_notifyRecvS2cReqPack;
	class __Msg_notifyCgiDone;
	class __CgiCtx;
	class __ClientCtx;
	



	virtual void onMessage(Message * msg, bool* isHandled) override;
	virtual void onMessageTimerTick(uint64_t timer_id, void* user_data) override;
	void __onMsgSendPack(Message* msg);
	void __onMsgTcpSocketClientConnected(ITcpSocketCallbackApi::ClientSocketConnectedMsg* msg);
	void __onMsgTcpSocketClientDisconnected(ITcpSocketCallbackApi::ClientSocketDisconnectedMsg* msg);
	void __onMsgTcpSocketClientSendDataEnd(ITcpSocketCallbackApi::ClientSocketSendDataEndMsg* msg);
	void __onMsgTcpSocketClientRecvData(ITcpSocketCallbackApi::ClientSocketRecvDataMsg* msg);
	void __onMsgNetSpeedTestStart(Message* msg);
	void __onMsgNetSpeedTestEnd(Message* msg);
	void __onMsgNetSpeedTestResultUpdate(Message* msg);



	bool __doConnectTcpSvr();
	bool __doTestSvrSpeed();
	void __postSendPackMsgToSelf();
	void __postMsgToSelf(Message* msg);
	void __notifyStarted();
	void __notifyStopped();
	int __getSvrInfoIndexBySvrIpAndPort(const std::string& svr_ip, uint32_t prot);
	ClientCgiInfo* __getClientCgiInfoBySendCmdType(uint32_t send_cmd_type);
	ClientCgiInfo* __getClientCgiInfoByRecvCmdType(uint32_t recv_cmd_type);



	InitParam m_init_param;
	bool m_is_running;
	uint64_t m_timer_id;
	DnsResolver* m_dns_resolver;
	ClientNetSpeedTester* m_speed_tester;
	bool m_is_testing_speed;
	__ClientCtx* m_client_ctx;
};



typedef ClientNetwork::ClientCgi ClientCgi;







SCLIENT_NAMESPACE_END
#endif

