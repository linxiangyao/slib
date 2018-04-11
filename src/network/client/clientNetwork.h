#ifndef S_CLIENT_NETWORK_H_
#define S_CLIENT_NETWORK_H_
#include "clientComm.h"
#include "clientNetSpeedTester.h"
SCLIENT_NAMESPACE_BEGIN




/*

one network bind to one server. if there are two servers, there are two networks.

auto connect.
auto retry send pack.
provide timeout funtion.
provide priority funtion.
flow limity.
network speed test, choice fastest svr.



note:
for good perfromance, network has no lock and is thread unsafe.
all the functions should be called in m_work_looper thread(typical is main thread), 
and will be callbacked in work thread too.
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

	class ICallback
	{
	public:
		virtual ~ICallback() {}
		virtual void onClientNetworkStatred(ClientNetwork* network) = 0;
		virtual void onClientNetworkStopped(ClientNetwork* network) = 0;
		virtual void onClientNetworkConnectStateChanged(ClientNetwork* network, EConnectState state) = 0;
		virtual void onClientNetworkSendPackEnd(ClientNetwork* network, EErrType err_type, int err_code, uint64_t send_pack_id, std::unique_ptr<RecvPack>* recv_pack) = 0;
		virtual void onClientNetworkRecvPack(ClientNetwork* network, std::unique_ptr<RecvPack>* recv_pack) = 0;
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
		}

		MessageLooper* m_work_looper;
		ICallback* m_callback;
		std::vector<SvrInfo> m_svr_infos;
		std::map<uint32_t, ClientCgiInfo> m_send_cmd_type_to_cgi_info_map;
		ITcpSocketCallbackApi* m_sapi;
		IUnpacker* m_unpacker;
		std::vector<int32_t> m_connect_interval_mss;
		bool m_is_repeat_last_connect_interval_ms;
	};





public:
	ClientNetwork();
	~ClientNetwork();
	bool init(const InitParam& param);

	bool start();
	void stop();

	SendPack* newSendPack(uint64_t send_pack_id, uint32_t send_cmd_type, uint32_t send_seq); // if send_seq == -1, network will generate send_seq
	bool sendPack(const SendPack& send_pack);
	void cancelSendPack(uint64_t pack_id);
	bool updateReconnectIntervals(const std::vector<uint32_t> intervalMss) { return true; }







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

	enum __ELoginState
	{
		__ELoginState_login,
		__ELoginState_logout,
	};

	class __SendPackInfo // Spi
	{
	public:
		__SendPackInfo() { m_is_cancel = false; m_create_time = 0; m_cgi_info = NULL; }
		~__SendPackInfo() { }
		//void copy(const SendPack& send_pack) { *(SendPack*)this = send_pack; }

		bool m_is_cancel;
		uint64_t m_create_time;
		SendPack m_pack;
		ClientCgiInfo* m_cgi_info;
	};

	typedef __SendPackInfo __WaitRespPackInfo; // Wpi

	class __ClientCtx
	{
	public:
		__ClientCtx() { m_sid = 0; m_connect_state = __EConnectState_disconnected; m_sending_pack_index = -1; m_svr_port = 0; }

		void resetConnectState()
		{
			m_connect_state = __EConnectState_disconnected;
			m_recv_data.clear();
			m_sending_pack_index = -1;
		}

		socket_id_t m_sid;
		__EConnectState m_connect_state;
		int m_sending_pack_index;
		std::string m_svr_ip;
		uint32_t m_svr_port;
		Binary m_recv_data;
	};


	// IMessageHandler
	virtual void onMessage(Message * msg, bool* isHandled);

	// IMessageTimerHandler
	virtual void onMessageTimerTick(uint64_t timer_id, void* user_data);

	void __onMsgSendPack(Message* msg);
	void __onMsgNetworkStatusChanged(Message* msg);
	void __onMsgForgroundChanged(Message* msg);
	void __onMsgTcpSocketClientConnected(ITcpSocketCallbackApi::ClientSocketConnectedMsg* msg);
	void __onMsgTcpSocketClientDisconnected(ITcpSocketCallbackApi::ClientSocketDisconnectedMsg* msg);
	void __onMsgTcpSocketClientSendDataEnd(ITcpSocketCallbackApi::ClientSocketSendDataEndMsg* msg);
	void __onMsgTcpSocketClientRecvData(ITcpSocketCallbackApi::ClientSocketRecvDataMsg* msg);

	void __onMsgNetSpeedTestStart(Message* msg);
	void __onMsgNetSpeedTestEnd(Message* msg);
	void __onMsgNetSpeedTestResultUpdate(Message* msg);



	bool __doSendPack();
	bool __doSendTcpPack();
	bool __doConnectTcpSvr();
	bool __doTestSvrSpeed();
	void __checkTimeOutPacks();

	bool __isTimeToConnect();
	void __postSendPackMsgToSelf();
	//void __postMsgToTarget(Message* msg);
	void __postMsgToSelf(Message* msg);
	void __postSendPackTimeOutMsg(__SendPackInfo* p);
	int __getMaxPriorySpiIndex();
	int __getMaxPrioryNoSessionSpiIndex();
	int __getSpiIndexBySendPackId(uint64_t send_pack_id);
	int __getWpiIndexByRecvPackCmdTypeAndSeq(uint32_t cmd_type, uint64_t seq);
	int __getWpiIndexBySendPackId(uint64_t send_pack_id);
	int __getSvrInfoIndexBySvrIpAndPort(const std::string& svr_ip, uint32_t prot);
	size_t __getSendPackCountBySendPackCmdType(uint32_t cmd_type);
	void __deleteAndEraseSpiByIndex(size_t index);
	uint64_t __getConnectIntervalMs(size_t connect_count);
	ClientCgiInfo* __getClientCgiInfoBySendCmdType(uint32_t send_cmd_type);
	ClientCgiInfo* __getClientCgiInfoByRecvCmdType(uint32_t recv_cmd_type);



	InitParam m_init_param;
	bool m_is_running;
	uint64_t m_timer_id;
	std::vector<int32_t> m_connect_interval_mss;
	bool m_is_repeat_last_connect_interval_ms;
	uint64_t m_last_reconnect_time_ms;
	size_t m_connect_count;

	ClientNetSpeedTester* m_speed_tester;
	std::map<std::string, ClientNetSpeedTester::TestResult> m_speed_test_results;
	bool m_is_testing_speed;

	__ClientCtx m_client_ctx;
	//uint64_t m_send_pack_id_seed;
	//uint32_t m_send_seq_seed;
	std::vector<__SendPackInfo*> m_send_pack_infos;
	std::vector<__WaitRespPackInfo*> m_wait_resp_pack_infos;
};











SCLIENT_NAMESPACE_END
#endif

