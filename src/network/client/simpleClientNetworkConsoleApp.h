#ifndef S_SIMPLE_CLIENT_NETWORK_CONSOLE_APP_H_
#define S_SIMPLE_CLIENT_NETWORK_CONSOLE_APP_H_
#include "clientNetwork.h"
#include "../../console/consoleApp.h"
SCLIENT_NAMESPACE_BEGIN




class SimpleClientNetworkConsoleLogic : public IConsoleAppLogic, public ClientNetwork::ICallback
{
public:
	void init(const std::string& svr_ip, uint32_t svr_port, const std::map<uint32_t, ClientCgiInfo>& cgi_infos, ClientNetwork::IUnpacker* unpacker)
	{
		m_cgi_infos = cgi_infos;
		m_svr_ip = svr_ip;
		m_svr_port = svr_port;
		m_unpacker = unpacker;
	}
	



protected:
	virtual void onAppStartMsg(IConsoleAppApi * api) override
	{
		m_console_api = api;
		initSocketLib();

		m_sapi = new TcpSocketCallbackApi();
		m_network = new ClientNetwork();

		// sapi
		{
			if (!m_sapi->init(getLooper()))
			{
				printf("fail to m_sapi->init\n");
				m_console_api->exit();
				return;
			}
		}

		// network
		{
			ClientNetwork::SvrInfo svr_info;
			svr_info.m_svr_ip_or_name = m_svr_ip;
			svr_info.m_svr_port = m_svr_port;
			ClientNetwork::InitParam param;
			param.m_unpacker = m_unpacker;
			param.m_sapi = m_sapi;
			param.m_work_looper = getLooper();
			param.m_callback = this;
			param.m_svr_infos.push_back(svr_info);
			param.m_send_cmd_type_to_cgi_info_map = m_cgi_infos;

			if (!m_network->init(param))
			{
				printf("fail to m_network->init\n");
				m_console_api->exit();
				return;
			}
		}

		if (!m_network->start())
		{
			printf("fail to m_network->start\n");
			m_console_api->exit();
			return;
		}
		printf("enter exit to quit\n");
	}

	virtual void onAppStopMsg() override
	{
		printf("exiting...\n");
		if(m_network != nullptr)
			m_network->stop();
		delete m_network;
		delete m_sapi;
		releaseSocketLib();
		printf("exit\n");
	}

	virtual void onClientNetwork_statred(ClientNetwork* network) override {}
	virtual void onClientNetwork_stopped(ClientNetwork* network) override {}
	virtual void onClientNetwork_connectStateChanged(ClientNetwork* network, ClientNetwork::EConnectState state) override {}
	virtual void onClientNetwork_recvS2cPushPack(ClientNetwork* network, std::unique_ptr<ClientNetwork::RecvPack>* recv_pack) override {}
	virtual void onClientNetwork_recvS2cReqPack(ClientNetwork* network, std::unique_ptr<ClientNetwork::RecvPack>* recv_pack) override {}
	virtual void onClientNetwork_cgiDone(ClientNetwork* network, ClientCgi* cgi) override {}


	MessageLooper* getLooper() { return &(m_console_api->getMessageLooper()); }
	ClientNetwork* getNetwork() { return m_network; }






private:
	IConsoleAppApi* m_console_api;
	ITcpSocketCallbackApi* m_sapi;
	ClientNetwork* m_network;
	std::string m_svr_ip;
	uint32_t m_svr_port;
	std::map<uint32_t, ClientCgiInfo> m_cgi_infos;
	ClientNetwork::IUnpacker* m_unpacker;
};













class ClientNetworkMsgLooperHandler : public IMessageLoopHandler, public ClientNetwork::ICallback
{
#define MSG_TYPE_ClientNetworkMsgLooperHandler_startCmd 9827171
#define MSG_TYPE_ClientNetworkMsgLooperHandler_stopCmd 9827172

public:
	ClientNetworkMsgLooperHandler()
	{
		m_network = nullptr;
		m_sapi = nullptr;
		m_packer = nullptr;
	}

	~ClientNetworkMsgLooperHandler()
	{
		if (m_network != nullptr)
			m_network->stop();
		delete m_network;
		delete m_sapi;
		delete m_packer;
	}

	void sendMsg_startCmd()
	{
		Message* msg = new Message();
		msg->m_target = this;
		msg->m_msg_type = MSG_TYPE_ClientNetworkMsgLooperHandler_startCmd;
		getLooper()->postMessage(msg);
	}

	void sendMsg_stopCmd()
	{
		Message* msg = new Message();
		msg->m_target = this;
		msg->m_msg_type = MSG_TYPE_ClientNetworkMsgLooperHandler_stopCmd;
		getLooper()->postMessage(msg);
	}






protected:
	class ClientNetworkMsgLooperHandlerInitParam
	{
	public:
		MessageLooper * m_looper;
		std::string m_svr_ip;
		uint32_t m_svr_port;
		std::map<uint32_t, ClientCgiInfo> m_cgi_infos;
	};



	virtual void onMessage(Message* msg, bool* is_handled) override
	{
		if (msg->m_target != this)
			return;

		switch (msg->m_msg_type)
		{
		case MSG_TYPE_ClientNetworkMsgLooperHandler_startCmd:
			onMsg_startCmd_before(msg);
			__onMsg_startCmd(msg);
			onMsg_startCmd_end(msg);
			*is_handled = true;
			break;
		case MSG_TYPE_ClientNetworkMsgLooperHandler_stopCmd:
			onMsg_stopCmd_before(msg);
			__onMsg_stopCmd(msg);
			onMsg_stopCmd_end(msg);
			*is_handled = true;
			break;
		default:
			break;
		}
	}

	virtual void onMessageTimerTick(uint64_t timer_id, void* user_data) override {}
	virtual void onClientNetwork_statred(ClientNetwork* network) override {}
	virtual void onClientNetwork_stopped(ClientNetwork* network) override {}
	virtual void onClientNetwork_connectStateChanged(ClientNetwork* network, ClientNetwork::EConnectState state) override {}
	virtual void onClientNetwork_recvS2cPushPack(ClientNetwork* network, std::unique_ptr<ClientNetwork::RecvPack>* recv_pack) override {}
	virtual void onClientNetwork_recvS2cReqPack(ClientNetwork* network, std::unique_ptr<ClientNetwork::RecvPack>* recv_pack) override {}
	virtual void onClientNetwork_cgiDone(ClientNetwork* network, ClientCgi* cgi) override {}
	virtual void onMsg_startCmd_before(Message* msg) {}
	virtual void onMsg_startCmd_end(Message* msg) {}
	virtual void onMsg_stopCmd_before(Message* msg) {}
	virtual void onMsg_stopCmd_end(Message* msg) {}



	bool init(const ClientNetworkMsgLooperHandlerInitParam& param)
	{
		m_init_param = param;
		return true;
	}

	MessageLooper * getLooper() { return m_init_param.m_looper; }
	ClientNetwork* getNetwork() { return m_network; }
	StPacker* getPacker() { return m_packer; }







private:
	void __onMsg_startCmd(Message* msg)
	{
		if (m_sapi != nullptr)
			return;

		initSocketLib();

		m_sapi = new TcpSocketCallbackApi();
		m_network = new ClientNetwork();
		m_packer = new StPacker();

		// sapi
		{
			if (!m_sapi->init(getLooper()))
			{
				slog_e("fail to m_sapi->init");
				return;
			}
		}

		// network
		{
			ClientNetwork::SvrInfo svr_info;
			svr_info.m_svr_ip_or_name = m_init_param.m_svr_ip;
			svr_info.m_svr_port = m_init_param.m_svr_port;
			ClientNetwork::InitParam param;
			param.m_unpacker = m_packer;
			param.m_sapi = m_sapi;
			param.m_work_looper = getLooper();
			param.m_callback = this;
			param.m_svr_infos.push_back(svr_info);
			param.m_send_cmd_type_to_cgi_info_map = m_init_param.m_cgi_infos;

			if (!m_network->init(param))
			{
				slog_e("fail to m_network->init");
				return;
			}
		}

		if (!m_network->start())
		{
			slog_e("fail to m_network->start");
			return;
		}
	}

	void __onMsg_stopCmd(Message* msg)
	{
		if (m_network != nullptr)
		{
			m_network->stop();
		}
		getLooper()->stopLoop();
	}


	ClientNetworkMsgLooperHandlerInitParam m_init_param;
	ITcpSocketCallbackApi* m_sapi;
	ClientNetwork* m_network;
	StPacker* m_packer;
};



SCLIENT_NAMESPACE_END
#endif
