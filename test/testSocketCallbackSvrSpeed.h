#ifndef TEST_SOCKET_CALLBACK_SVR_SPEED_H
#define TEST_SOCKET_CALLBACK_SVR_SPEED_H
#include "testS.h"
#include "testSocket.h"
#include "testLog.h"
//#include "g.pb.h"
using namespace std;
USING_NAMESPACE_S




class __TestSocketCallbackSvrSpeedLogic : public IConsoleAppLogic
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

		printf("svr start, port=12306\n");
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
		//if (!m_sapi->sendDataFromSvrTranSocketToClient(m_tran_sid, (const byte_t*)textMsg.c_str(), textMsg.size() + 1))
		//{
		//	printf("svr:: fail to send\n");
		//}
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
				__onMsg_accept(msg);
				break;

			case ITcpSocketCallbackApi::EMsgType_svrTranSocketStopped:
				__onMsg_tranSocketStopped(msg);
				break;

			case ITcpSocketCallbackApi::EMsgType_svrTranSocketRecvData:
				__onMsg_recvData(msg);
				break;

			case ITcpSocketCallbackApi::EMsgType_svrTranSocketSendDataEnd:
				__onMsg_sendDataEnd(msg);
				break;

			default:
				break;
			}
		}
	}

	void __onMsg_accept(Message* msg)
	{
		printf("svr:: accept one client\n");
		ITcpSocketCallbackApi::SvrListenSocketAcceptedMsg* m = (ITcpSocketCallbackApi::SvrListenSocketAcceptedMsg*)msg;
		m_tran_ctx.m_tran_sid = m->m_svr_trans_sid;
		m_tran_ctx.m_start_time_ms = TimeUtil::getMsTime();
		std::string str = "hello, i am svr";
		if (!m_sapi->sendDataFromSvrTranSocketToClient(m_tran_ctx.m_tran_sid, (const byte_t*)str.c_str(), str.size() + 1))
		{
			printf("fail to sendDataFromSvrTranSocketToClient\n");
		}
	}

	void __onMsg_tranSocketStopped(Message* msg)
	{
		printf("svr:: client disconnected\n");
		uint64_t total_time_span_ms = TimeUtil::getMsTime() - m_tran_ctx.m_start_time_ms;
		uint64_t byte_per_second = m_tran_ctx.m_recv_total_len * 1000 / total_time_span_ms;
		printf("svr:: recv_total_len=%s, recv_total_ms=%" PRIu64 ", byte_per_second=%s\n"
			, StringUtil::byteCountToDisplayString(m_tran_ctx.m_recv_total_len).c_str(), total_time_span_ms, StringUtil::byteCountToDisplayString(byte_per_second).c_str());

		m_tran_ctx = __TranCtx();
	}

	void __onMsg_recvData(Message* msg)
	{
		ITcpSocketCallbackApi::SvrTranSocketRecvDataMsg* m = (ITcpSocketCallbackApi::SvrTranSocketRecvDataMsg*)msg;
		m_tran_ctx.m_recv_total_len += (uint32_t)m->m_data.getLen();

		size_t recv_100m_count = (size_t)m_tran_ctx.m_recv_total_len / (100 * 1024 * 1024);
		while (recv_100m_count > m_tran_ctx.m_already_print_recv_100m_count)
		{
			++m_tran_ctx.m_already_print_recv_100m_count;
			printf(".");
			fflush(stdout);
		}


		const char* sz = (const char*)m->m_data.getData();
		if (sz[0] == 't' && sz[1] == 'e' && strncmp(sz, "test_case=", strlen("test_case=")) == 0)
		{
			if (m_tran_ctx.m_already_print_recv_100m_count > 0)
				printf("\n");
			printf("svr:: recv=%s\n", m->m_data.getData());
		}
	}

	void __onMsg_sendDataEnd(Message* msg)
	{
		//printf("svr:: send end\n");
	}


	class __TranCtx
	{
	public:
		__TranCtx() { m_tran_sid = 0; m_recv_total_len = 0; m_start_time_ms = 0; m_already_print_recv_100m_count = 0; }
		socket_id_t m_tran_sid;
		uint32_t m_recv_total_len;
		uint64_t m_start_time_ms;
		size_t m_already_print_recv_100m_count;
	};

	IConsoleAppApi* m_console_api;
	ITcpSocketCallbackApi* m_sapi;
	socket_id_t m_listen_sid;
	__TranCtx m_tran_ctx;
};


void __testSocketCallbackSvrSpeed()
{
	printf("\n__testSocketCallbackSvrSpeed ---------------------------------------------------------\n");
	__initLog(ELogLevel_info);
	ConsoleApp* app = new ConsoleApp();
	__TestSocketCallbackSvrSpeedLogic* logic = new __TestSocketCallbackSvrSpeedLogic();
	app->run(logic);
	delete logic;
	delete app;
}




#endif // !TESTSOCKETSVR_H