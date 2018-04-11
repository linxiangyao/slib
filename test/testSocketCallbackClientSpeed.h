#ifndef TEST_SOCKET_CALLBACK_CLIENT_SPEED_H
#define TEST_SOCKET_CALLBACK_CLIENT_SPEED_H
#include "testS.h"
#include "../src/socket/socketLib.h"
#include "../src/thread/threadLib.h"
#include "../src/console/consoleApp.h"
#include "testLog.h"
using namespace std;
USING_NAMESPACE_S




// callback client --------------------------------------------------------------------------------------------------
class __TestCallbackClientSpeedLogic : public IConsoleAppLogic
{
private:
	enum __ETestCase
	{
		__ETestCase_none,
		__ETestCase_upload,
		__ETestCase_download,
		__ETestCase_c2sReq_s2cResp,
	};

#define __TestCallbackClientSpeedLogic_uploadPackSize (100 * 1024)
#define __TestCallbackClientSpeedLogic_uploadPackCount 400000
#define __TestCallbackClientSpeedLogic_downloadPackSize (10 * 1024)
#define __TestCallbackClientSpeedLogic_downloadPackCount 1000
#define __TestCallbackClientSpeedLogic_c2sReq_s2cResp_packSize (10 * 1024)
#define __TestCallbackClientSpeedLogic_c2sReq_s2cResp_packCount 1000
#define __TestCallbackClientSpeedLogic_paraPackCount 10

	virtual void onAppStartMsg(IConsoleAppApi * api)
	{
		printf("client:: onAppStartMsg\n");

		m_uploaded_pack_count = 0;
		m_upload_pack_count = 0;
		byte_t* d = new byte_t[__TestCallbackClientSpeedLogic_uploadPackSize];
		m_upload_pack_data.attach(d, __TestCallbackClientSpeedLogic_uploadPackSize);


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
		param.m_svr_ip_or_name = "127.0.0.1";
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

		m_testCase = __ETestCase_none;
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
		//if (!m_sapi->sendDataFromClientSocketToSvr(m_sid, (const byte_t*)textMsg.c_str(), textMsg.size() + 1))
		//{
		//	printf("client:: fail to sendDataFromClientSocketToSvr\n");
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
			case ITcpSocketCallbackApi::EMsgType_clientSocketConnected:
				__onMsg_clientConnected(msg);
				break;

			case ITcpSocketCallbackApi::EMsgType_clientSocketDisconnected:
				printf("client:: disconnected\n");
				break;

			case ITcpSocketCallbackApi::EMsgType_clientSocketRecvData:
				__onMsg_recvData(msg);
				break;

			case ITcpSocketCallbackApi::EMsgType_clientSocketSendDataEnd:
				__onMsg_sendDataEnd(msg);
				break;
			default:
				break;
			}
		}
	}

	void __onMsg_clientConnected(Message* msg)
	{
		printf("client:: connected\n");
		if (m_testCase != __ETestCase_none)
			return;

		m_testCase = __ETestCase_upload;
		printf("client:: test upload\n");
		std::string str = std::string() + "test_case=upload_start"
			+ ",pack_size=" + StringUtil::toString(__TestCallbackClientSpeedLogic_uploadPackSize)
			+ ",pack_count=" + StringUtil::toString(__TestCallbackClientSpeedLogic_uploadPackCount)
			+ ",";
		m_upload_start_time_ms = TimeUtil::getMsTime();
		__sendPackToSvr(str);
	}

	void __onMsg_recvData(Message* msg)
	{
		ITcpSocketCallbackApi::ClientSocketRecvDataMsg* m = (ITcpSocketCallbackApi::ClientSocketRecvDataMsg*)msg;
		if (m->m_recv_data.getLen() > 10)
		{
			const char* sz = (const char*)m->m_recv_data.getData();
			if (sz[0] == 't' && sz[1] == 'e' && strncmp(sz, "test_case=", strlen("test_case=")) == 0)
			{
				std::string recv_str;
				recv_str.append((const char*)m->m_recv_data.getData());
				printf("client:: recv=%s\n", recv_str.c_str());

				std::string test_case_str = StringUtil::fetchMiddle(recv_str, "test_case=", ",");
				if (test_case_str == "download_end")
				{

				}
			}
		}
	}

	void __onMsg_sendDataEnd(Message* msg)
	{
		//printf("client:: send end\n");
		if (m_testCase == __ETestCase_upload)
		{
			if (m_upload_pack_count == 0)
			{
				for (int i = 0; i < __TestCallbackClientSpeedLogic_paraPackCount; ++i)
				{
					__sendPackToSvr(m_upload_pack_data.getData(), m_upload_pack_data.getLen());
				}
			}
			else
			{
				++m_uploaded_pack_count;
				//printf("send pack end, count=%llu\n", m_uploaded_pack_count);

				if (m_upload_pack_count < __TestCallbackClientSpeedLogic_uploadPackCount)
				{
					__sendPackToSvr(m_upload_pack_data.getData(), m_upload_pack_data.getLen());
				}

				if (m_uploaded_pack_count == __TestCallbackClientSpeedLogic_uploadPackCount)
				{
					uint64_t total_size = (uint64_t)__TestCallbackClientSpeedLogic_uploadPackCount * __TestCallbackClientSpeedLogic_uploadPackSize;
					uint64_t total_time_span_ms = TimeUtil::getMsTime() - m_upload_start_time_ms;
					uint64_t byte_per_second = total_size * 1000 / total_time_span_ms;
					printf("client:: upload end, total_size=%s, sent_ms=%" PRIu64 ", byte_per_second=%s\n"
						, StringUtil::byteCountToDisplayString(total_size).c_str(), total_time_span_ms, StringUtil::byteCountToDisplayString(byte_per_second).c_str());

					std::string str = std::string() + "test_case=upload_end";
					__sendPackToSvr(str);

					m_testCase = __ETestCase_download;
				}
			}
		}
		else if (m_testCase == __ETestCase_download)
		{
			//str = std::string() + "test_case=download_start"
			//	+ ",pack_size=" + StringUtil::toString(__TestCallbackClientSpeedLogic_downloadPackSize)
			//	+ ",pack_count=" + StringUtil::toString(__TestCallbackClientSpeedLogic_downloadPackCount)
			//	+ ",";
			//__sendPackToSvr(str);
			printf("close socket\n");
			m_sapi->releaseClientSocket(m_sid);
			m_sid = 0;
		}
	}


	bool __sendPackToSvr(const std::string& str)
	{
		if (!m_sapi->sendDataFromClientSocketToSvr(m_sid, (const byte_t*)str.c_str(), str.size() + 1))
		{
			printf("fail to sendDataFromClientSocketToSvr\n");
			return false;
		}
		return true;
	}

	bool __sendPackToSvr(const byte_t* data, size_t data_len)
	{
		bool is_ok = m_sapi->sendDataFromClientSocketToSvr(m_sid, data, data_len);
		if (!is_ok)
		{
			printf("fail to __sendPackToSvr\n");
			return false;
		}
		m_upload_pack_count++;
		if(m_upload_pack_count == __TestCallbackClientSpeedLogic_uploadPackCount)
			printf("upload all data to svr ok, m_upload_pack_count=%u\n", (uint32_t)m_upload_pack_count);
		return is_ok;
	}



	size_t m_uploaded_pack_count;
	size_t m_upload_pack_count;
	Binary m_upload_pack_data;
	uint64_t m_upload_start_time_ms;
	__ETestCase m_testCase;
	IConsoleAppApi* m_console_api;
	ITcpSocketCallbackApi* m_sapi;
	socket_id_t m_sid;
};


void __testSocketCallbackClientSpeed()
{
	printf("\n__testSocketCallbackClientSpeed ---------------------------------------------------------\n");
	__initLog(ELogLevel_info);
	ConsoleApp* app = new ConsoleApp();
	__TestCallbackClientSpeedLogic* logic = new __TestCallbackClientSpeedLogic();
	app->run(logic);
	delete logic;
	delete app;
}



#endif // !TESTSOCKETCLIENT_H

