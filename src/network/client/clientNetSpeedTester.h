#ifndef S_CLIENT_NET_SPEED_TESTER_H_
#define S_CLIENT_NET_SPEED_TESTER_H_
#include "clientComm.h"
SCLIENT_NAMESPACE_BEGIN



// test send pack to svr and recv pack speed
class ClientNetSpeedTester : public IMessageHandler
{
public:
	enum EMsgType
	{
		EMsgType_onTestStart = 826146181,
		EMsgType_onOneTestResult,		// (std::string svr_ip_or_name, uint32_t svr_port, bool is_connected, uint64_t send_bytes_per_second, uint64_t recv_bytes_per_second)
		EMsgType_onTestEnd,
	};

	class SvrInfo
	{
	public:
		std::string m_svr_ip_or_name;
		uint32_t m_svr_port;
	};

	class InitParam
	{
	public:
		InitParam() 
		{
			m_sapi = nullptr;
			m_work_looper = nullptr;
			m_notify_looper = nullptr;
			m_notify_target = nullptr;
			m_dns_resolver = nullptr;
		}

		ITcpSocketCallbackApi* m_sapi;
		MessageLooper* m_work_looper;
		MessageLooper* m_notify_looper;
		void* m_notify_target;
		DnsResolver* m_dns_resolver;
		std::vector<SvrInfo> m_svr_infos;
	};
	
	class Msg_oneTestResult : public Message
	{
	public:
		Msg_oneTestResult()
		{
			m_msg_type = EMsgType_onOneTestResult;
		}

		std::string m_svr_ip_or_name;
		std::string m_svr_ip_str;
		Ip m_svr_ip;
		uint32_t m_svr_port;
		bool m_is_connected;
	};

	ClientNetSpeedTester();
	~ClientNetSpeedTester();
	bool init(const InitParam& param);
	bool start();
	void stop();





private:
	enum __EClientConncectState
	{
		__EClientConncectState_none,
		__EClientConncectState_connecting,
		__EClientConncectState_connected,
		__EClientConncectState_disconnected,
	};

	class __OneIpTestRecord
	{
	public:
		__OneIpTestRecord() { m_sid = 0; m_send_bytes_per_second = 0; m_recv_bytes_per_second = 0; m_connect_state = __EClientConncectState_none; }

		Ip m_ip;
		socket_id_t m_sid;
		uint64_t m_send_bytes_per_second;
		uint64_t m_recv_bytes_per_second;
		__EClientConncectState m_connect_state;
	};

	class __ClientCtx
	{
	public:
		__ClientCtx(const SvrInfo& svr_info, InitParam* param, ClientNetSpeedTester* tester);
		~__ClientCtx();

		void start();
		void stop();
		bool getIsRunning();
		void onDnsResolveEnd(const DnsResolver::DnsRecord& dns_record);
		void onClientConnected(socket_id_t sid);
		void onClientDisconnected(socket_id_t sid);

		SvrInfo m_svr_info;
		std::vector<__OneIpTestRecord> m_ip_records;


	private:
		void __doDns();
		void __doConnect();
		void __addDnsRecord(const DnsResolver::DnsRecord& dns_record);
		void __checkIsRunning();
		void __postMsg(Message* msg);
		void __notifyOneIpTestResult(const __OneIpTestRecord& record);
		int __getIpIndexBySid(socket_id_t sid);

		InitParam* m_init_param;
		ClientNetSpeedTester* m_speed_tester;
		bool m_is_running;
	};

	// IMessageHandler
	virtual void onMessage(Message * msg, bool* isHandled);
	void __onMessage_clientConnected(const ITcpSocketCallbackApi::ClientSocketConnectedMsg& msg);
	void __onMessage_clientDisconnected(const ITcpSocketCallbackApi::ClientSocketDisconnectedMsg& msg);
	void __onMessage_dnsResolved(Message* msg);

	void __checkIsDone();
	void __stop();
	void __postMsgToTarget(Message* msg);
	int __getClientCtxIndexBySvrName(const std::string& svr_ip_or_name);
	__ClientCtx* __getClientCtxBySvrName(const std::string& svr_ip_or_name);

	bool m_is_running;
	Mutex m_mutex;
	InitParam m_init_param;
	std::vector<__ClientCtx*> m_client_ctxs;
};


SCLIENT_NAMESPACE_END
#endif