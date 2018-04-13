#ifndef TEST_CLIENT_NETWORK_H
#define TEST_CLIENT_NETWORK_H
#include "testS.h"
#include "testLog.h"
#include "../src/network/client/clientNetworkLib.h"
using namespace std;
USING_NAMESPACE_S
USING_NAMESPACE_SCLIENT


class __ClientSendPackBuilder
{
public:
	__ClientSendPackBuilder()
	{
		m_callback = nullptr;
		m_uin = 0;

		memset(m_session_id_bin, 0, 16);
		uint64_t tick = TimeUtil::getTick();
		memcpy(m_session_id_bin, &tick, 8);
		m_session_id_bin[14] = 0x11;
		m_session_id_bin[15] = 0x12;

		m_packer = nullptr;
		m_network = nullptr;

		m_send_pack_id = 0;
		m_send_pack_seq = 0;
		m_send_cmd_type = 0;
	}

	ClientCgi::SendPack* newSendPackAndPack(const byte_t* send_body, size_t send_body_len) const
	{
		ClientCgi::SendPack* send_pack = m_network->newSendPack(m_send_pack_id, m_send_cmd_type, m_send_pack_seq);

		Binary* whole_pack_bin = &(send_pack->m_send_whole_pack_bin);
		StPacker::Pack p;
		p.m_head.m_cmd_type = send_pack->m_send_cmd_type;
		p.m_head.m_err = 0;
		p.m_head.m_seq = send_pack->m_send_seq;
		p.m_head.m_uin = m_uin;
		memcpy(p.m_head.m_session_id_bin, m_session_id_bin, 16);
		p.m_body.attach((byte_t*)send_body, send_body_len);
		m_packer->packToBin(p, whole_pack_bin);
		p.m_body.detach();

		return send_pack;
	}

	uint32_t m_uin;
	byte_t m_session_id_bin[16];
	StPacker* m_packer;
	ClientNetwork* m_network;

	ClientCgi::ICallback* m_callback;
	uint64_t m_send_pack_id;
	uint32_t m_send_pack_seq;
	uint32_t m_send_cmd_type;
};




class __ClientCgi_base : public ClientCgi
{
protected:
	bool __initSendPack(__ClientSendPackBuilder& ctx, const std::string& body)
	{
		ctx.m_send_cmd_type = getCgiInfo().m_send_cmd_type;
		ClientCgi::SendPack* send_pack = ctx.newSendPackAndPack((const byte_t*)body.c_str(), body.size() + 1);
		setSendPack(send_pack);

		setCallback(ctx.m_callback);
		return true;
	}
};


class __ClientCgi_NotifyStatistic : public __ClientCgi_base
{
public:
	static const ClientCgiInfo & s_getCgiInfo()
	{
		s_cgi_info.m_cgi_type = EClientCgiType_c2sNotify;
		s_cgi_info.m_send_cmd_type = __ECgiCmdType_c2sNotify_notifyStatistic;
		return s_cgi_info;
	}

	virtual const ClientCgiInfo & getCgiInfo() const { return s_getCgiInfo(); }

	bool initSendPack(__ClientSendPackBuilder& ctx, const std::string& statistic_info)
	{
		m_c2s_notify_body = "c2sNotify_notifyStatistic: " + statistic_info;
		return __initSendPack(ctx, m_c2s_notify_body);	
	}

	std::string m_c2s_notify_body;

private:
	static ClientCgiInfo s_cgi_info;
};
ClientCgiInfo __ClientCgi_NotifyStatistic::s_cgi_info;



class __ClientCgi_CheckVersion : public __ClientCgi_base
{
public:
	static const ClientCgiInfo & s_getCgiInfo()
	{
		s_cgi_info.m_cgi_type = EClientCgiType_c2sReq_s2cResp;
		s_cgi_info.m_send_cmd_type = __ECgiCmdType_c2sReq_checkVersion;
		s_cgi_info.m_recv_cmd_type = __ECgiCmdType_s2cResp_checkVersion;
		return s_cgi_info;
	}

	virtual const ClientCgiInfo & getCgiInfo() const { return s_getCgiInfo(); }

	bool initSendPack(__ClientSendPackBuilder& ctx)
	{
		m_c2s_req_body = "c2sReq_checkVersion: client_ver=1.0.123,";
		return __initSendPack(ctx, m_c2s_req_body);
	}

	std::string m_c2s_req_body;
	std::string m_s2c_resp_body;
	std::string m_s2c_resp_new_client_ver;
	bool m_s2c_resp_is_need_update;
	std::string m_s2c_resp_download_url;
	uint32_t m_s2c_resp_download_total_size;


private:
	virtual void onSetRecvPackEnd()
	{
		RecvPack* recv_pack = getRecvPack();
		StPacker::Pack* st_pack = (StPacker::Pack*)recv_pack->m_recv_ext;
		m_s2c_resp_body = (const char*)st_pack->m_body.getData();
		m_s2c_resp_new_client_ver = StringUtil::fetchMiddle(m_s2c_resp_body, "new_client_ver=", ",");
		m_s2c_resp_is_need_update = StringUtil::parseInt(StringUtil::fetchMiddle(m_s2c_resp_body, "is_need_update=", ","));
		m_s2c_resp_download_url = StringUtil::fetchMiddle(m_s2c_resp_body, "download_url=", ",");
		m_s2c_resp_download_total_size = StringUtil::parseUint(StringUtil::fetchMiddle(m_s2c_resp_body, "download_total_size=", ","));
	}

	static ClientCgiInfo s_cgi_info;
};
ClientCgiInfo __ClientCgi_CheckVersion::s_cgi_info;































// logic ------------------------------------------------------------------------------------------
class __ClientNetworkLogic : public SimpleClientNetworkConsoleLogic
{
public:
#define __ClientNetworkLogic_10K (64 * 1024)
	__ClientNetworkLogic()
	{
		m_send_pack_id_seed = 0;
		m_send_pack_seq_seed = 0;
		m_timer_id = 0;
		m_packer = new StPacker();

		std::map<uint32_t, ClientCgiInfo> cgi_infos;
		cgi_infos[__ClientCgi_NotifyStatistic::s_getCgiInfo().m_send_cmd_type] = __ClientCgi_NotifyStatistic::s_getCgiInfo();
		cgi_infos[__ClientCgi_CheckVersion::s_getCgiInfo().m_send_cmd_type] = __ClientCgi_CheckVersion::s_getCgiInfo();


		//SimpleClientNetworkConsoleLogic::init("120.78.58.61", 12306, cgi_infos);
		SimpleClientNetworkConsoleLogic::init("127.0.0.1", 12306, cgi_infos, m_packer);
	}




private:
	virtual void onAppStartMsg(IConsoleAppApi * api) override
	{
		SimpleClientNetworkConsoleLogic::onAppStartMsg(api);
		//m_timer_id = getLooper()->createTimer(NULL);
		//getLooper()->startTimer(m_timer_id, 1, 20 * 1000);

		m_send_pack_builder.m_packer = m_packer;
		m_send_pack_builder.m_network = getNetwork();
		m_send_pack_builder.m_callback = this;

		__startNotifyStatistic();
		__startCheckVersion();
		//__startLogin();
	}

	virtual void onAppStopMsg() override
	{
		SimpleClientNetworkConsoleLogic::onAppStopMsg();
		//getLooper()->releasseTimer(m_timer_id);
	}

	virtual void onMessageTimerTick(uint64_t timer_id, void* user_data) override
	{
		if (timer_id != m_timer_id)
			return;
		printf("on timer\n");
	}

	virtual void onClientCgi_cgiDone(ClientCgi* cgi) override
	{
		SimpleClientNetworkConsoleLogic::onClientCgi_cgiDone(cgi);
		if (cgi->getCgiInfo().m_cgi_type == EClientCgiType_c2sNotify && cgi->getCgiInfo().m_send_cmd_type == __ECgiCmdType_c2sNotify_notifyStatistic)
		{
			__ClientCgi_NotifyStatistic* c = (__ClientCgi_NotifyStatistic*)cgi;
			if (cgi->getIsSuccess())
				printf("client: notify_statistic_cgi ok\n");
			else
				printf("client: notify_statistic_cgi fail\n");
		}
		else if (cgi->getCgiInfo().m_cgi_type == EClientCgiType_c2sReq_s2cResp && cgi->getCgiInfo().m_send_cmd_type == __ECgiCmdType_c2sReq_checkVersion)
		{
			__onClientCgi_cgiDone_checkVersion(cgi);
		}
		else
		{
			printf("client: unknow cgi!\n");
		}
		delete cgi;
	}

	void __onClientCgi_cgiDone_checkVersion(ClientCgi* cgi)
	{
		__ClientCgi_CheckVersion* c = (__ClientCgi_CheckVersion*)cgi;
		if (!cgi->getIsSuccess())
		{
			printf("client: check_version_cgi fail\n");
			return;
		}
		printf("client: recv resp=%s\n", c->m_s2c_resp_body.c_str());
		printf("client: check_version_cgi ok, resp field: new_client_ver=%s, download_url=%s, download_total_size=%s\n"
			, c->m_s2c_resp_new_client_ver.c_str(), c->m_s2c_resp_download_url.c_str(), StringUtil::byteCountToDisplayString(c->m_s2c_resp_download_total_size).c_str());
	}




	  
	void __startNotifyStatistic()
	{
		m_send_pack_builder.m_send_pack_id = ++m_send_pack_id_seed;
		m_send_pack_builder.m_send_pack_seq = 0;

		__ClientCgi_NotifyStatistic* cgi = new __ClientCgi_NotifyStatistic();
		if (!cgi->initSendPack(m_send_pack_builder, std::string("os=win7, cpu=x64,")))
		{
			printf("fail to init notify_statstic_cgi\n");
			return;
		}
		printf("client: send notify=%s\n", cgi->m_c2s_notify_body.c_str());

		if (!getCgiMgr()->startCgi(cgi))
		{
			printf("client: fail to start notify_statstic_cgi\n");
			return;
		}
	}

	void __startCheckVersion()
	{
		m_send_pack_builder.m_send_pack_id = ++m_send_pack_id_seed;
		m_send_pack_builder.m_send_pack_seq = ++m_send_pack_seq_seed;

		__ClientCgi_CheckVersion* cgi = new __ClientCgi_CheckVersion();
		if (!cgi->initSendPack(m_send_pack_builder))
		{
			printf("client: fail to init check_version_cgi\n");
			return;
		}
		printf("client: send req=%s\n", cgi->m_c2s_req_body.c_str());

		if (!getCgiMgr()->startCgi(cgi))
		{
			printf("client: fail to start check_version_cgi\n");
			return;
		}
	}

	uint64_t m_timer_id;
	__ClientSendPackBuilder m_send_pack_builder;
	StPacker* m_packer;
	uint64_t m_send_pack_id_seed;
	uint32_t m_send_pack_seq_seed;
};







void __testClientNetwork()
{
	printf("\n__testClientNetwork ---------------------------------------------------------\n");
	__initLog(ELogLevel_debug);
	ConsoleApp* app = new ConsoleApp();
	__ClientNetworkLogic* logic = new __ClientNetworkLogic();
	app->run(logic);
	delete logic;
	delete app;
}



void __testStPacker()
{
	printf("\n__testStPacker ---------------------------------------------------------\n");
	StPacker packer;
	StPacker::Pack p;
	p.m_head.m_seq = 1;
	p.m_head.m_cmd_type = 1024;
	p.m_head.m_session_id_bin[0] = 1;
	p.m_head.m_session_id_bin[1] = 9;
	p.m_head.m_session_id_bin[2] = 9;
	p.m_head.m_session_id_bin[3] = 8;
	p.m_head.m_session_id_bin[14] = 6;
	p.m_head.m_session_id_bin[15] = 6;
	Binary bin;
	packer.packToBin(p, &bin);
	ClientNetwork::UnpackResult r;
	packer.unpackClientRecvPack(bin.getData(), bin.getLen(), &r);


	StPacker::Pack* up = (StPacker::Pack*)r.m_recv_ext;
	if (r.m_recv_cmd_type == p.m_head.m_cmd_type && r.m_recv_seq == p.m_head.m_seq
		&& up->m_head.m_session_id_bin[0] == 1 && up->m_head.m_session_id_bin[1] == 9 && up->m_head.m_session_id_bin[2] == 9 && up->m_head.m_session_id_bin[3] == 8
		&& up->m_head.m_session_id_bin[14] == 6 && up->m_head.m_session_id_bin[15] == 6)
	{
		printf("ok\n");
	}
	else
	{
		printf("failt\n");
	}
}
/*
*/

#endif
