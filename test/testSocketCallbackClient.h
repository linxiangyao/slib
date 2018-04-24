#ifndef TEST_SOCKET_CALLBACK_CLIENT_H
#define TEST_SOCKET_CALLBACK_CLIENT_H
#include "testS.h"
#include "../src/socket/socketLib.h"
#include "../src/thread/threadLib.h"
#include "../src/console/consoleApp.h"
using namespace std;
USING_NAMESPACE_S




// callback client --------------------------------------------------------------------------------------------------
class __CallbackClientLogic : public IConsoleAppLogic
{
private:
	virtual void onAppStartMsg(IConsoleAppApi * api)
	{
		printf("client:: onAppStartMsg\n");
		m_console_api = api;
		initSocketLib();
		m_sapi = new TcpSocketCallbackApi();
		if (!m_sapi->init(&(m_console_api->getMessageLooper())))
		{
			printf("client:: fail to m_sapi->init\n");
			m_console_api->exit();
			return;
		}

		m_sid = 0;
		ITcpSocketCallbackApi::CreateClientSocketParam param;
		param.m_callback_looper = &m_console_api->getMessageLooper();
		param.m_callback_target = this;
		param.m_svr_ip = "127.0.0.1";
		param.m_svr_port = 12306;
		if (!m_sapi->createClientSocket(&m_sid, param))
		{
			printf("client:: fail to m_sapi->createClientSocket\n");
			m_console_api->exit();
			return;
		}

		if (!m_sapi->startClientSocket(m_sid))
		{
			printf("client:: fail to m_sapi->startClientSocket\n");
			m_console_api->exit();
			return;
		}
	}

	virtual void onAppStopMsg()
	{
		printf("client:: exiting...\n");
		delete m_sapi;
		releaseSocketLib();
		printf("client:: exited...\n");
	}

	virtual void onTextMsg(const std::string & textMsg)
	{
		if (!m_sapi->sendDataFromClientSocketToSvr(m_sid, (const byte_t*)textMsg.c_str(), textMsg.size() + 1))
		{
			printf("client:: fail to sendDataFromClientSocketToSvr\n");
		}
	}

	virtual void onMessage(Message * msg, bool* is_handled)
	{
		if (msg->m_target != this)
			return;

		if (msg->m_sender == m_sapi)
		{
			switch (msg->m_msg_type)
			{
			case ITcpSocketCallbackApi::EMsgType_clientSocketConnected:
				printf("client:: connected\n");
				break;

			case ITcpSocketCallbackApi::EMsgType_clientSocketDisconnected:
				printf("client:: disconnected\n");
				break;

			case ITcpSocketCallbackApi::EMsgType_clientSocketRecvData:
			{
				ITcpSocketCallbackApi::ClientSocketRecvDataMsg* m = (ITcpSocketCallbackApi::ClientSocketRecvDataMsg*)msg;
				std::string str;
				str.append((const char*)m->m_recv_data.getData());
				printf("client:: recv=%s\n", str.c_str());
			}
			break;

			case ITcpSocketCallbackApi::EMsgType_clientSocketSendDataEnd:
				printf("client:: send end\n");
				break;
			default:
				break;
			}
		}
	}



	IConsoleAppApi* m_console_api;
	ITcpSocketCallbackApi* m_sapi;
	socket_id_t m_sid;
};


void __testSocketCallbackClient()
{
	printf("\n__testSocketClient ---------------------------------------------------------\n");
	__initLog(ELogLevel_debug);
	ConsoleApp* app = new ConsoleApp();
	__CallbackClientLogic* logic = new __CallbackClientLogic();
	app->run(logic);
	delete logic;
	delete app;
}



#endif // !TESTSOCKETCLIENT_H






