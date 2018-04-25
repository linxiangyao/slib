#ifndef TEST_SOCKET_CALLBACK_SVR_H
#define TEST_SOCKET_CALLBACK_SVR_H
#include "testS.h"
#include "testSocket.h"
//#include "g.pb.h"
using namespace std;
USING_NAMESPACE_S




class __SvrLogic : public IConsoleAppLogic
{
public:

private:
	virtual void onAppStartMsg(IConsoleAppApi * api)
	{
		initSocketLib();

		m_console_api = api;

		m_sapi = new TcpSocketCallbackApi();
		if (!m_sapi->init(&(m_console_api->getMessageLooper())))
		{
			printf("svr:: fail to init\n");
			m_console_api->exit();
			return;
		}

		m_listen_sid = 0;
		ITcpSocketCallbackApi::CreateSvrSocketParam param;
		param.m_callback_looper = &m_console_api->getMessageLooper();
		param.m_callback_target = this;
		param.m_svr_ip = "0.0.0.0";
		param.m_svr_port = 12306;
		if (!m_sapi->createSvrListenSocket(&m_listen_sid, param))
		{
			printf("svr::  fail to createSvrListenSocket\n");
			m_console_api->exit();
			return;
		}

		if (!m_sapi->startSvrListenSocket(m_listen_sid))
		{
			printf("svr:: fail to startSvrListenSocket\n");
			m_console_api->exit();
			return;
		}
	}

	virtual void onAppStopMsg()
	{
		printf("svr:: exiting...\n");
		delete m_sapi;
		releaseSocketLib();
		printf("svr:: exit\n");
	}

	virtual void onTextMsg(const std::string& textMsg)
	{
		if (!m_sapi->sendDataFromSvrTranSocketToClient(m_tran_sid, (const byte_t*)textMsg.c_str(), textMsg.size() + 1))
		{
			printf("svr:: fail to send\n");
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
			case ITcpSocketCallbackApi::EMsgType_svrListenSocketStarted:
				printf("svr:: started, wait client...\n");
				break;

			case ITcpSocketCallbackApi::EMsgType_svrListenSocketStopped:
				printf("svr:: server stopped\n");
				break;

			case ITcpSocketCallbackApi::EMsgType_svrListenSocketAccepted:
				{
					printf("svr:: accept\n");
					ITcpSocketCallbackApi::SvrListenSocketAcceptedMsg* m = (ITcpSocketCallbackApi::SvrListenSocketAcceptedMsg*)msg;
					m_tran_sid = m->m_svr_trans_sid;
					std::string str = "hello, i am svr";
					if (!m_sapi->sendDataFromSvrTranSocketToClient(m_tran_sid, (const byte_t*)str.c_str(), str.size() + 1))
					{
						printf("fail to sendDataFromSvrTranSocketToClient\n");
					}
				}
				break;

			case ITcpSocketCallbackApi::EMsgType_svrTranSocketStopped:
				printf("svr:: client disconnected\n");
				m_tran_sid = 0;
				break;

			case ITcpSocketCallbackApi::EMsgType_svrTranSocketRecvData:
				{
					ITcpSocketCallbackApi::SvrTranSocketRecvDataMsg* m = (ITcpSocketCallbackApi::SvrTranSocketRecvDataMsg*)msg;
					printf("svr:: recv=%s\n", m->m_data.getData());
				}
				break;

			case ITcpSocketCallbackApi::EMsgType_svrTranSocketSendDataEnd:
				printf("svr:: send end\n");
				break;

			default:
				break;
			}
		}
	}


	IConsoleAppApi* m_console_api;
	ITcpSocketCallbackApi* m_sapi;
	socket_id_t m_listen_sid;
	socket_id_t m_tran_sid;
};


void __testSocketCallbackSvr()
{
	printf("\n__testSocketSvr ---------------------------------------------------------\n");
	__initLog(ELogLevel_debug);
	ConsoleApp* app = new ConsoleApp();
	__SvrLogic* logic = new __SvrLogic();
	app->run(logic);
	delete logic;
	delete app;
}




#endif // !TESTSOCKETSVR_H