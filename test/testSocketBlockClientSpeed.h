#ifndef TEST_SOCKET_BLOCK_CLIENT_SPEED_H
#define TEST_SOCKET_BLOCK_CLIENT_SPEED_H
#include "testS.h"
#include "../src/socket/socketLib.h"
#include "../src/socket/win/socketApi_win.h"
#include "../src/util/utilLib.h"
#include "../src/thread/threadLib.h"
#include "../src/console/consoleApp.h"
#include "testLog.h"
using namespace std;
USING_NAMESPACE_S



// block client --------------------------------------------------------------------------------------------------
class __TestBlockClientSpeedRun : public IThreadRun
{
private:
#define __TestBlockClientSpeedRun_uploadPackSize (100 * 1024)
#define __TestBlockClientSpeedRun_uploadPackCount 400000
	virtual void run()
	{
		m_is_exit = false;
		m_upload_pack_count = 0;
		byte_t* d = new byte_t[__TestBlockClientSpeedRun_uploadPackSize];
		m_upload_pack_data.attach(d, __TestBlockClientSpeedRun_uploadPackSize);

		initSocketLib(); 
		m_sapi = ITcpSocketBlockApi::newBlockApi();

		__run();

		releaseSocketLib();
		delete m_sapi;
		m_sapi = NULL;
		printf("client:: exit\n");
	}

	virtual void stop()
	{
		m_is_exit = true;
		if (m_sapi == NULL)
			return;
		m_sapi->closeSocket(m_sid);
	}

	void __run()
	{
		if (!m_sapi->openSocket(&m_sid))
		{
			printf("client:: fail to m_sapi->openSocket\n");
			return;
		}
		printf("client:: open socket ok\n");

		if (!m_sapi->connect(m_sid, "127.0.0.1", 12306))
		{
			printf("client:: fail to m_sapi->connect\n");
			return;
		}
		printf("client:: connected\n");

		if (m_is_exit)
			return;

		printf("client:: test upload\n");
		std::string str = std::string() + "test_case=upload_start"
			+ ",pack_size=" + StringUtil::toString(__TestBlockClientSpeedRun_uploadPackSize)
			+ ",pack_count=" + StringUtil::toString(__TestBlockClientSpeedRun_uploadPackCount)
			+ ",";
		m_upload_start_time_ms = TimeUtil::getMsTime();
		if (!__sendPackToSvr(str))
			return;

		while (m_upload_pack_count < __TestBlockClientSpeedRun_uploadPackCount)
		{
			if (m_is_exit)
				return;

			if (!__sendPackToSvr(m_upload_pack_data.getData(), m_upload_pack_data.getLen()))
			{
				return;
			}
			
			++m_upload_pack_count;
			//printf("send pack=%u\n", (uint32_t)m_upload_pack_count);
		}

		uint64_t total_size = (uint64_t)__TestBlockClientSpeedRun_uploadPackSize * __TestBlockClientSpeedRun_uploadPackCount;
		uint64_t total_time_span_ms = TimeUtil::getMsTime() - m_upload_start_time_ms;
		uint64_t byte_per_second = total_size * 1000 / total_time_span_ms;
		printf("client:: upload end, total_size=%s, sent_ms=%" PRIu64 ", byte_per_second=%s\n"
			, StringUtil::byteCountToDisplayString(total_size).c_str(), total_time_span_ms, StringUtil::byteCountToDisplayString(byte_per_second).c_str());

		str = std::string() + "test_case=upload_end";
		if (!__sendPackToSvr(str))
			return;
	}


	bool __sendPackToSvr(const std::string& str)
	{
		if (!m_sapi->send(m_sid, (const byte_t*)str.c_str(), str.size() + 1))
		{
			printf("client:: fail to send\n");
			return false;
		}
		return true;
	}

	bool __sendPackToSvr(const byte_t* data, size_t data_len)
	{
		return m_sapi->send(m_sid, data, data_len);
	}

	bool m_is_exit;
	size_t m_upload_pack_count;
	Binary m_upload_pack_data;
	uint64_t m_upload_start_time_ms;
	ITcpSocketBlockApi* m_sapi;
	socket_id_t m_sid;
};


void __testSocketBlockClientSpeed()
{
	printf("\n__testSocketBlockClientSpeed ---------------------------------------------------------\n");
	__initLog(ELogLevel_info);
	__TestBlockClientSpeedRun* r = new __TestBlockClientSpeedRun();
	Thread* t = new Thread(r);
	t->start();
	__pauseConsole();
	t->stopAndJoin();
	delete t;
}






// raw client --------------------------------------------------------------------------------------------------
class __TestRawSocketBlockClientSpeedRun : public IThreadRun
{
private:
#define __TestRawSocketBlockClientSpeedRun_uploadPackSize ( 100 * 1024)
#define __TestRawSocketBlockClientSpeedRun_uploadPackCount 400000
	virtual void run()
	{
		m_is_exit = false;
		m_upload_pack_count = 0;
		byte_t* d = new byte_t[__TestRawSocketBlockClientSpeedRun_uploadPackSize];
		m_upload_pack_data.attach(d, __TestRawSocketBlockClientSpeedRun_uploadPackSize);

		initSocketLib();

		__run();

#ifdef WIN32
		closesocket(m_s);
#else
		close(m_s);
#endif // WIN32

		releaseSocketLib();
		printf("client:: exit\n");
	}

	virtual void stop()
	{
		m_is_exit = true;
	}

	void __run()
	{
		m_s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (m_s == -1)
		{
			printf("client:: fail to create socket\n");
			return;
		}
		printf("client:: open socket ok\n");

		if (!SocketUtil::connect(m_s, "127.0.0.1", 12306))
		{
			printf("client:: fail to m_sapi->connect\n");
			return;
		}
		printf("client:: connected\n");

		if (m_is_exit)
			return;

		printf("client:: test upload\n");
		std::string str = std::string() + "test_case=upload_start"
			+ ",pack_size=" + StringUtil::toString(__TestRawSocketBlockClientSpeedRun_uploadPackSize)
			+ ",pack_count=" + StringUtil::toString(__TestRawSocketBlockClientSpeedRun_uploadPackCount)
			+ ",";
		m_upload_start_time_ms = TimeUtil::getMsTime();
		if (!__sendPackToSvr(str))
			return;

		while (m_upload_pack_count < __TestRawSocketBlockClientSpeedRun_uploadPackCount)
		{
			if (m_is_exit)
				return;
			if (!__sendPackToSvr(m_upload_pack_data.getData(), m_upload_pack_data.getLen()))
			{
				return;
			}
			++m_upload_pack_count;
			//printf("send pack=%u\n", (uint32_t)m_upload_pack_count);
		}

		uint64_t total_size = (uint64_t)__TestRawSocketBlockClientSpeedRun_uploadPackSize * __TestRawSocketBlockClientSpeedRun_uploadPackCount;
		uint64_t total_time_span_ms = TimeUtil::getMsTime() - m_upload_start_time_ms;
		uint64_t byte_per_second = total_size * 1000 / total_time_span_ms;
		printf("client:: upload end, total_size=%s, sent_ms=%" PRIu64 ", byte_per_second=%s\n"
			, StringUtil::byteCountToDisplayString(total_size).c_str(), total_time_span_ms, StringUtil::byteCountToDisplayString(byte_per_second).c_str());

		str = std::string() + "test_case=upload_end";
		if (!__sendPackToSvr(str))
			return;
	}

	bool __sendPackToSvr(const std::string& str)
	{
		if (!SocketUtil::send(m_s, (const byte_t*)str.c_str(), str.size() + 1))
		{
			printf("client:: fail to send\n");
			return false;
		}
		return true;
	}

	bool __sendPackToSvr(const byte_t* data, size_t data_len)
	{
		return SocketUtil::send(m_s, data, data_len);
	}




	bool m_is_exit;
	size_t m_upload_pack_count;
	Binary m_upload_pack_data;
	uint64_t m_upload_start_time_ms;
	SOCKET m_s;
};


void __testRawSocketBlockClientSpeed()
{
	printf("\n__testRawSocketBlockClientSpeed ---------------------------------------------------------\n");
	__initLog(ELogLevel_info);
	__TestRawSocketBlockClientSpeedRun* r = new __TestRawSocketBlockClientSpeedRun();
	Thread* t = new Thread(r);
	t->start();
	__pauseConsole();
	t->stopAndJoin();
	delete t;
}






#endif // !TESTSOCKETCLIENT_H

