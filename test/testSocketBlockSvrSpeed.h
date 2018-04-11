#ifndef TEST_SOCKET_BLOCK_SVR_SPEED_H
#define TEST_SOCKET_BLOCK_SVR_SPEED_H
#include "../src/socket/socketLib.h"
#include "../src/thread/threadLib.h"
#include "../src/util/timeUtil.h"
#include "testS.h"
#include "testSocket.h"
#include "testLog.h"
//#include "g.pb.h"
using namespace std;
USING_NAMESPACE_S




class __TestSocketBlockSvrSpeedRun : public IThreadRun
{
private:
#define __TestSocketBlockSvrSpeedRun_recvBufLen (64 * 1024)
	virtual void run()
	{
		m_is_exit = false;
		m_buf = new byte_t[__TestSocketBlockSvrSpeedRun_recvBufLen];
		initSocketLib();
		m_sapi = ITcpSocketBlockApi::newBlockApi();

		__run();

		delete m_sapi;
		releaseSocketLib();
		delete[] m_buf;
	}

	virtual void stop()
	{
		m_is_exit = true;
		m_sapi->closeSocket(m_listen_sid);
		m_sapi->closeSocket(m_tran_sid);
	}


	void __run()
	{
		if (!m_sapi->openSocket(&m_listen_sid))
		{
			printf("svr:: fail to openSocket\n");
			return;
		}
		printf("svr:: openSocket ok\n");

		if (!m_sapi->bindAndListen(m_listen_sid, "0.0.0.0", 12306))
		{
			printf("svr:: fail to bindAndListen\n");
			return;
		}
		printf("svr:: bindAndListen ok, port=12306\n");

		while (true)
		{
			if (m_is_exit)
				return;

			printf("svr:: start accept\n");
			if (!m_sapi->accept(m_listen_sid, &m_tran_sid))
			{
				printf("svr:: fail to accept\n");
				return;
			}
			printf("svr:: accept one client\n");

			__runTran();
		}
	}

	void __runTran()
	{
		uint64_t recv_total_len = 0;
		uint64_t start_time_ms = TimeUtil::getMsTime();
		size_t already_print_recv_100m_count = 0;
		while (true)
		{
			size_t recv_len = 0;
			if (!m_sapi->recv(m_tran_sid, m_buf, __TestSocketBlockSvrSpeedRun_recvBufLen, &recv_len))
			{
				printf("svr:: fail to recv\n");
				break;
			}
			recv_total_len += recv_len;

			size_t recv_100m_count = (size_t)recv_total_len / (100 * 1024 * 1024);
			while (recv_100m_count > already_print_recv_100m_count)
			{
				++already_print_recv_100m_count;
				printf(".");
				fflush(stdout);
			}

			if (recv_len > 10)
			{
				const char* sz = (const char*)m_buf;
				if (sz[0] == 't' && sz[1] == 'e' && strncmp(sz, "test_case=", strlen("test_case=")) == 0)
				{
					if (already_print_recv_100m_count > 0)
						printf("\n");
					printf("svr:: recv=%s\n", sz);
				}
			}
		}

		uint64_t total_time_span_ms = TimeUtil::getMsTime() - start_time_ms;
		uint64_t byte_per_second = recv_total_len * 1000 / total_time_span_ms;
		printf("\nsvr:: recv_total_len=%s, recv_total_ms=%" PRIu64 ", byte_per_second=%s\n"
			, StringUtil::byteCountToDisplayString(recv_total_len).c_str(), total_time_span_ms, StringUtil::byteCountToDisplayString(byte_per_second).c_str());
	}
	

	bool m_is_exit;
	ITcpSocketBlockApi* m_sapi;
	socket_id_t m_listen_sid;
	socket_id_t m_tran_sid;
	byte_t* m_buf;
};




void __TestSocketBlockSvrSpeed()
{
	printf("\n__TestSocketBlockSvrSpeed ---------------------------------------------------------\n");
	__initLog(ELogLevel_info);
	__TestSocketBlockSvrSpeedRun* r = new __TestSocketBlockSvrSpeedRun();
	Thread* t = new Thread(r);
	t->start();
	__pauseConsole();
	t->stopAndJoin();
	delete t;
}




#endif // !TESTSOCKETSVR_H