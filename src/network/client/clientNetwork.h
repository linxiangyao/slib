#ifndef S_CLIENT_NETWORK_H_
#define S_CLIENT_NETWORK_H_
#include "clientComm.h"
#include "../comm/dnsResolver.h"
#include "clientNetSpeedTester.h"
SCLIENT_NAMESPACE_BEGIN



/*
client network.

auto connect.
auto retry send pack.
provide timeout funtion.
provide priority funtion.
flow limity.
network speed test, choice fastest svr.



NOTE:
	one client network bind to one server. if there are two servers, there are two client networks.

	network is thread unsafe(for good perfromance).
	network should be called in m_work_looper thread(typical is main thread), and network will callback in the same thread.
	there may be many networks in one thread.

	one ITcpSocketCallbackApi may be used by many networks.
	ITcpSocketCallbackApi may be in the same thread of network, or may not be.
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
		ClientCgi() { m_send_pack = nullptr; m_recv_pack = nullptr; m_err_type = EErrType_ok; m_err_code = 0; m_start_ms = 0; m_end_ms = 0; }
		virtual ~ClientCgi() { delete m_send_pack; delete m_recv_pack; }

		virtual const ClientCgiInfo& getCgiInfo() const = 0;


		RecvPack*	getRecvPack() { return m_recv_pack; }
		SendPack*	getSendPack() { return m_send_pack; }
		EErrType	getErrType() { return m_err_type; }
		int			getErrCode() { return m_err_code; }
		bool		getIsSuccess() { return getErrType() == EErrType_ok && getErrCode() == 0; }
		uint64_t	getStartMs() { return m_start_ms; }
		uint64_t	getEndMs() { return m_end_ms; }

		void		setSendPack(SendPack* send_pack) { m_send_pack = send_pack; }
		void		setRecvPack(RecvPack* recv_pack) { m_recv_pack = recv_pack; onSetRecvPackEnd(); }
		void		setErrType(EErrType err_type) { m_err_code = err_type; }
		void		setErrCode(int err_code) { m_err_code = err_code; }
		void		setStartMs(uint64_t ms) { m_start_ms = ms; }
		void		setEndMs(uint64_t ms) { m_end_ms = ms; }


	protected:
		virtual void onSetRecvPackEnd() {}


	private:
		SendPack* m_send_pack;
		RecvPack* m_recv_pack;
		EErrType m_err_type;
		int m_err_code;
		uint64_t m_start_ms;
		uint64_t m_end_ms;
	};

	class ICallback
	{
	public:
		virtual ~ICallback() {}
		virtual void onClientNetwork_statred(ClientNetwork* network) = 0;
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
			m_work_looper = NULL;
			m_sapi = NULL;
			m_connect_interval_mss.push_back(1);
			m_connect_interval_mss.push_back(1*1000);
			m_connect_interval_mss.push_back(2 * 1000);
			m_connect_interval_mss.push_back(3 * 1000);
			m_connect_interval_mss.push_back(4 * 1000);
			m_connect_interval_mss.push_back(5 * 1000);
			m_is_repeat_last_connect_interval_ms = true;
			m_callback = nullptr;
			m_max_pack_count = 1000;
		}

		MessageLooper* m_work_looper;
		ICallback* m_callback;
		std::vector<SvrInfo> m_svr_infos;
		std::map<uint32_t, ClientCgiInfo> m_send_cmd_type_to_cgi_info_map;
		ITcpSocketCallbackApi* m_sapi;
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
		__EMsgType_sendPack,

		__EmsgType_onNetworkStateChanged,
		__EmsgType_onForgroundChanged,

		__EMsgType_onTcpSocketClientConnected,
		__EMsgType_onTcpSocketClientDisconnected,
		__EMsgType_onTcpSocketClientSendDataEnd,
		__EMsgType_onTcpSocketClientRecvData,
	};
	
	enum __EConnectState
	{
		__EConnectState_connecting,
		__EConnectState_connected,
		__EConnectState_disconnected,
	};

	class __CgiCtx
	{
	public:
		__CgiCtx() { m_is_sent = false; m_create_time = 0; m_cgi = nullptr; }
		~__CgiCtx() { }

		bool m_is_sent;
		uint64_t m_create_time;
		ClientCgi* m_cgi;
	};
	
	class __ClientCtx
	{
	public:
		__ClientCtx();
		~__ClientCtx();

		void addCgi(ClientCgi* cgi);
		void cancelCgi(ClientCgi* cgi);
		bool doSendPack();
		void checkTimeOutPacks();
		int getCgiIndexBySendPackId(uint64_t send_pack_id);
		size_t getCgiCountBySendPackCmdType(uint32_t send_pack_cmd_type);

		void onSendPackEnd();
		void onConnected();
		void onDisconnected();
		void onRecvPack(RecvPack* recv_pack);


		void __resetConnectState();
		void __markCgiDoneByIndex(int index, EErrType err_type, int err_code);
		int __getMaxPrioryCgiIndex();
		ClientCgiInfo* __getClientCgiInfoByRecvCmdType(uint32_t recv_cmd_type);
		int __getCgiIndexBySendPackSeq(uint64_t send_pack_seq);


		std::string m_svr_ip;
		uint32_t m_svr_port;
		socket_id_t m_sid;
		__EConnectState m_connect_state;
		int m_sending_cgi_index;
		Binary m_recv_data;
		std::vector<__CgiCtx*> m_cgi_ctxs;
		InitParam* m_init_param;
		ClientNetwork* m_network;
	};


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

	bool __isTimeToConnect();
	uint64_t __getConnectIntervalMs(size_t connect_count);
	int __getSvrInfoIndexBySvrIpAndPort(const std::string& svr_ip, uint32_t prot);
	ClientCgiInfo* __getClientCgiInfoBySendCmdType(uint32_t send_cmd_type);
	ClientCgiInfo* __getClientCgiInfoByRecvCmdType(uint32_t recv_cmd_type);



	InitParam m_init_param;
	bool m_is_running;
	uint64_t m_timer_id;

	ClientNetSpeedTester* m_speed_tester;
	std::map<std::string, ClientNetSpeedTester::TestResult> m_speed_test_results;
	bool m_is_testing_speed;

	std::vector<int32_t> m_connect_interval_mss;
	bool m_is_repeat_last_connect_interval_ms;
	uint64_t m_last_reconnect_time_ms;
	size_t m_connect_count;


	__ClientCtx m_client_ctx;
};



typedef ClientNetwork::ClientCgi ClientCgi;







SCLIENT_NAMESPACE_END
#endif

