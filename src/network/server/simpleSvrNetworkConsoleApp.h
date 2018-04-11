#ifndef S_SIMPLE_SVR_NETWORK_CONSOLE_APP_H_
#define S_SIMPLE_SVR_NETWORK_CONSOLE_APP_H_
#include "serverNetwork.h"
#include "serverCgiMgr.h"
#include "../packer/simpleTcpPacker.h"
#include "../../console/consoleApp.h"
SSVR_NAMESPACE_BEGIN






class SimpleSvrNetworkConsoleLogic : public IConsoleAppLogic, public ServerCgiMgr::ICallback, public ServerCgi::ICallback
{
public:
	SimpleSvrNetworkConsoleLogic() {}
	~SimpleSvrNetworkConsoleLogic() {}

	bool init(int argc, char** argv, const std::string& svr_ip, uint32_t svr_port, const std::vector<ServerCgiInfo>& cgi_infos, ServerNetwork::IUnpacker* unpacker)
	{
		m_args = ConsoleUtil::parseArgs(argc, argv);
		m_svr_ip = svr_ip;
		m_svr_port = svr_port;
		m_cgi_infos = cgi_infos;
		m_unpacker = unpacker;
		return true;
	}





protected:
	virtual void onAppStartMsg(IConsoleAppApi * api) override
	{
		slog_d("app start...");
		if (!initSocketLib())
		{
			slog_e("fail to initSocketLib");
			return;
		}
		m_console_api = api;

		m_sapi = new TcpSocketCallbackApi();
		m_network = new ServerNetwork();
		m_cgi_mgr = new ServerCgiMgr();


		// m_sapi
		if (!m_sapi->init(getLooper()))
		{
			slog_e("fail to init sapi");
			return;
		}

		// m_network
		{
			ServerNetwork::InitParam param;
			param.m_unpacker = m_unpacker;
			param.m_sapi = m_sapi;
			param.m_work_looper = getLooper();
			param.m_callback = m_cgi_mgr;
			param.m_svr_ip_or_name = m_svr_ip;
			param.m_svr_port = m_svr_port;
			for (size_t i = 0; i < m_cgi_infos.size(); ++i)
			{
				ServerCgiInfo& info = m_cgi_infos[i];
				param.m_send_cmd_type_to_cgi_info_map[info.m_send_cmd_type] = info;
			}
			if (!m_network->init(param))
			{
				slog_e("fail to init network");
				return;
			}
		}

		// m_cgi_mgr
		{
			ServerCgiMgr::InitParam param;
			param.m_callback = this;
			param.m_network = m_network;
			param.m_work_looper = getLooper();
			param.m_cgi_infos = m_cgi_infos;
			param.m_session_time_out_second = 5 * 60;
			if (!m_cgi_mgr->init(param))
			{
				slog_e("fail to init m_cgi_mgr");
				return;
			}
		}

		if (!m_network->start())
		{
			slog_e("fail to start network");
			return;
		}
		slog_d("app start ok");
	}

	virtual void onAppStopMsg() override
	{
		slog_d("app exit...");
		delete m_cgi_mgr;
		delete m_network;
		delete m_sapi;
		slog_d("app exit ok");
	}

	virtual void onTextMsg(const std::string & textMsg) override {}
	virtual void onMessage(Message * msg, bool* is_handled) override {}
	virtual void onMessageTimerTick(uint64_t timer_id, void * user_data) override {}
	virtual void onServerCgiMgr_sessionCreated(socket_id_t sid, session_id_t ssid) override {}
	virtual void onServerCgiMgr_sessionClosed(socket_id_t sid, session_id_t ssid) override {}
	virtual void onServerCgiMgr_recvC2sNotifyPack(std::unique_ptr<ServerCgi::RecvPack>* recv_pack) override {}
	virtual void onServerCgiMgr_recvC2sReqPack(std::unique_ptr<ServerCgi::RecvPack>* recv_pack) override {}
	virtual void onServerCgi_cgiDone(ServerCgi* cgi) override {}



	const std::string& getSvrIp() { return m_svr_ip; }
	uint32_t getSvrPort() { return m_svr_port; }
	std::map<std::string, std::string> getArgs() { return m_args; }
	ITcpSocketCallbackApi* getSapi() { return m_sapi; }
	ServerNetwork* getNetwork() { return m_network; }
	ServerCgiMgr* getCgiMgr() { return m_cgi_mgr; }
	MessageLooper* getLooper() { return &(m_console_api->getMessageLooper()); }




private:
	std::string m_svr_ip;
	uint32_t m_svr_port;
	std::vector<ServerCgiInfo> m_cgi_infos;
	IConsoleAppApi* m_console_api;
	std::map<std::string, std::string> m_args;
	ServerNetwork::IUnpacker* m_unpacker;
	ITcpSocketCallbackApi* m_sapi;
	ServerNetwork* m_network;
	ServerCgiMgr* m_cgi_mgr;
};





class ServerNetworkMsgLooperHandler : public IMessageLoopHandler, public ServerCgiMgr::ICallback, public ServerCgi::ICallback
{
#define MSG_TYPE_ServerNetworkMsgLooperHandler_startCmd 8176371
#define MSG_TYPE_ServerNetworkMsgLooperHandler_stopCmd 8176372

public:
	ServerNetworkMsgLooperHandler() {}
	~ServerNetworkMsgLooperHandler() {}

	void sendMsg_startCmd()
	{
		Message* msg = new Message();
		msg->m_target = this;
		msg->m_msg_type = MSG_TYPE_ServerNetworkMsgLooperHandler_startCmd;
		getLooper()->postMessage(msg);
	}

	void sendMsg_stopCmd()
	{
		Message* msg = new Message();
		msg->m_target = this;
		msg->m_msg_type = MSG_TYPE_ServerNetworkMsgLooperHandler_stopCmd;
		getLooper()->postMessage(msg);
	}






protected:
	bool init(MessageLooper* looper, const std::string& svr_ip, uint32_t svr_port, const std::vector<ServerCgiInfo>& cgi_infos, ServerNetwork::IUnpacker* unpacker)
	{
		m_looper = looper;
		m_svr_ip = svr_ip;
		m_svr_port = svr_port;
		m_cgi_infos = cgi_infos;
		m_unpacker = unpacker;
		return true;
	}

	virtual void onMessage(Message* msg, bool* is_handled) override
	{
		if (msg->m_target != this)
			return;

		switch (msg->m_msg_type)
		{
		case MSG_TYPE_ServerNetworkMsgLooperHandler_startCmd:
			onMsg_startCmd_before(msg);
			__onMsg_startCmd(msg);
			onMsg_startCmd_end(msg);
			*is_handled = true;
			break;
		case MSG_TYPE_ServerNetworkMsgLooperHandler_stopCmd:
			onMsg_stopCmd_before(msg);
			__onMsg_stopCmd(msg);
			onMsg_stopCmd_end(msg);
			*is_handled = true;
			break;
		default:
			break;
		}
	}

	virtual void onMessageTimerTick(uint64_t timer_id, void * user_data) override {}
	virtual void onServerCgiMgr_sessionCreated(socket_id_t sid, session_id_t ssid) override {}
	virtual void onServerCgiMgr_sessionClosed(socket_id_t sid, session_id_t ssid) override {}
	virtual void onServerCgiMgr_recvC2sNotifyPack(std::unique_ptr<ServerCgi::RecvPack>* recv_pack) override {}
	virtual void onServerCgiMgr_recvC2sReqPack(std::unique_ptr<ServerCgi::RecvPack>* recv_pack) override {}
	virtual void onServerCgi_cgiDone(ServerCgi* cgi) override {}

	virtual void onMsg_startCmd_before(Message* msg) {}
	virtual void onMsg_startCmd_end(Message* msg) {}
	virtual void onMsg_stopCmd_before(Message* msg) {}
	virtual void onMsg_stopCmd_end(Message* msg) {}






	virtual void __onMsg_startCmd(Message* msg)
	{
		slog_d("app start...");
		if (!initSocketLib())
		{
			slog_e("fail to initSocketLib");
			return;
		}

		m_sapi = new TcpSocketCallbackApi();
		m_network = new ServerNetwork();
		m_cgi_mgr = new ServerCgiMgr();


		// m_sapi
		if (!m_sapi->init(getLooper()))
		{
			slog_e("fail to init sapi");
			return;
		}

		// m_network
		{
			ServerNetwork::InitParam param;
			param.m_unpacker = m_unpacker;
			param.m_sapi = m_sapi;
			param.m_work_looper = getLooper();
			param.m_callback = m_cgi_mgr;
			param.m_svr_ip_or_name = m_svr_ip;
			param.m_svr_port = m_svr_port;
			for (size_t i = 0; i < m_cgi_infos.size(); ++i)
			{
				ServerCgiInfo& info = m_cgi_infos[i];
				param.m_send_cmd_type_to_cgi_info_map[info.m_send_cmd_type] = info;
			}
			if (!m_network->init(param))
			{
				slog_e("fail to init network");
				return;
			}
		}

		// m_cgi_mgr
		{
			ServerCgiMgr::InitParam param;
			param.m_callback = this;
			param.m_network = m_network;
			param.m_work_looper = getLooper();
			param.m_cgi_infos = m_cgi_infos;
			param.m_session_time_out_second = 5 * 60;
			if (!m_cgi_mgr->init(param))
			{
				slog_e("fail to init m_cgi_mgr");
				return;
			}
		}

		if (!m_network->start())
		{
			slog_e("fail to start network");
			return;
		}
		slog_d("app start ok");
	}

	virtual void __onMsg_stopCmd(Message* msg)
	{
		slog_d("app exit...");
		delete m_cgi_mgr;
		delete m_network;
		delete m_sapi;
		slog_d("app exit ok");
	}


	const std::string& getSvrIp() { return m_svr_ip; }
	uint32_t getSvrPort() { return m_svr_port; }
	std::map<std::string, std::string> getArgs() { return m_args; }
	ITcpSocketCallbackApi* getSapi() { return m_sapi; }
	ServerNetwork* getNetwork() { return m_network; }
	ServerCgiMgr* getCgiMgr() { return m_cgi_mgr; }
	MessageLooper* getLooper() { return m_looper; }




private:
	MessageLooper* m_looper;
	std::string m_svr_ip;
	uint32_t m_svr_port;
	std::vector<ServerCgiInfo> m_cgi_infos;
	std::map<std::string, std::string> m_args;
	ServerNetwork::IUnpacker* m_unpacker;
	ITcpSocketCallbackApi* m_sapi;
	ServerNetwork* m_network;
	ServerCgiMgr* m_cgi_mgr;
};






SSVR_NAMESPACE_END
#endif
