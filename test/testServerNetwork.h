#ifndef TEST_SERVER_NETWORK_H_
#define TEST_SERVER_NETWORK_H_
#include "testS.h"
#include "testLog.h"
#include "../src/network/server/serverNetworkLib.h"
using namespace std;
USING_NAMESPACE_S
USING_NAMESPACE_SSVR




class __ServerCgiCtx
{
public:
	__ServerCgiCtx()
	{
		m_uin = 0;
		m_packer = nullptr;
		m_network = nullptr;
	}

	ServerCgi::SendPack* newSendPackAndPack(uint32_t send_cmd_type, uint32_t send_seq, const byte_t* send_body, size_t send_body_len) const
	{
		ServerCgi::SendPack* send_pack = m_network->newSendPack(m_sid, m_ssid, send_cmd_type, send_seq);

		Binary* whole_pack_bin = &(send_pack->m_send_whole_pack_bin);
		StPacker::Pack p;
		p.m_head.m_cmd_type = send_pack->m_send_cmd_type;
		p.m_head.m_err = 0;
		p.m_head.m_seq = send_pack->m_send_seq;
		p.m_head.m_uin = m_uin;
		memcpy(p.m_head.m_session_id_bin, m_ssid.m_data, 16);
		p.m_body.attach((byte_t*)send_body, send_body_len);
		m_packer->packToBin(p, whole_pack_bin);
		p.m_body.detach();

		return send_pack;
	}

	socket_id_t m_sid;
	session_id_t m_ssid;
	uint32_t m_uin;
	StPacker* m_packer;
	ServerNetwork* m_network;
	ServerCgi::ICallback* m_callback;
};




class __ServerCgi_c2sNotify_notifyStatistic : public ServerCgi
{
public:
	static const ServerCgiInfo & s_getServerCgiInfo()
	{
		s_cgi_info.m_cgi_type = EServerCgiType_c2sNotify;
		s_cgi_info.m_recv_cmd_type = __ECgiCmdType_c2sNotify_notifyStatistic;
		return s_cgi_info;
	}

	virtual const ServerCgiInfo& getServerCgiInfo() const override { return s_getServerCgiInfo(); }

	std::string m_c2s_notify_body;


private:
	virtual void onSetRecvPackEnd() override
	{
		StPacker::Pack* st_pack = (StPacker::Pack*)getRecvPack()->m_recv_ext;
		m_c2s_notify_body = (const char*)st_pack->m_body.getData();
	}

	static ServerCgiInfo s_cgi_info;
};
ServerCgiInfo __ServerCgi_c2sNotify_notifyStatistic::s_cgi_info;





class __ServerCgi_s2cResp_checkVersion : public ServerCgi
{
public:
	__ServerCgi_s2cResp_checkVersion(const __ServerCgiCtx& cgi_ctx)
	{
		m_cgi_ctx = cgi_ctx;
		setCallback(m_cgi_ctx.m_callback);
	}

	static const ServerCgiInfo & s_getServerCgiInfo()
	{
		s_cgi_info.m_cgi_type = EServerCgiType_c2sReq_s2cResp;
		s_cgi_info.m_recv_cmd_type = __ECgiCmdType_c2sReq_checkVersion;
		s_cgi_info.m_send_cmd_type = __ECgiCmdType_s2cResp_checkVersion;
		return s_cgi_info;
	}

	virtual const ServerCgiInfo& getServerCgiInfo() const override { return s_getServerCgiInfo(); }

	bool initSendPack(const std::string& new_client_ver, bool is_need_update, const std::string& download_url, uint32_t download_total_size)
	{
		m_s2c_resp_body = std::string() + "s2cResp_checkVersion: new_client_ver=" + new_client_ver
			+ ", is_need_update=" + StringUtil::toString(is_need_update)
			+ ", download_url=" + download_url
			+ ", download_total_size=" + StringUtil::toString(download_total_size) 
			+ ",";
		SendPack* send_pack = m_cgi_ctx.newSendPackAndPack(getServerCgiInfo().m_send_cmd_type, getRecvPack()->m_recv_seq, (const byte_t*)m_s2c_resp_body.c_str(), m_s2c_resp_body.size() + 1);
		setSendPack(send_pack);
		return send_pack != nullptr;
	}

	std::string m_c2s_req_body;
	std::string m_s2c_resp_body;


private:
	virtual void onSetRecvPackEnd() override
	{
		StPacker::Pack* st_pack = (StPacker::Pack*)(getRecvPack()->m_recv_ext);
		m_c2s_req_body = (const char*)st_pack->m_body.getData();
	}

	static ServerCgiInfo s_cgi_info;
	__ServerCgiCtx m_cgi_ctx;
};
ServerCgiInfo __ServerCgi_s2cResp_checkVersion::s_cgi_info;
























class __ServerNetworkLogic : public SimpleSvrNetworkConsoleLogic
{
public:
	__ServerNetworkLogic()
	{
		std::vector<ServerCgiInfo> cgi_infos;
		cgi_infos.push_back(__ServerCgi_c2sNotify_notifyStatistic::s_getServerCgiInfo());
		cgi_infos.push_back(__ServerCgi_s2cResp_checkVersion::s_getServerCgiInfo());
		m_packer = new StPacker();
		SimpleSvrNetworkConsoleLogic::init(0, NULL, "0.0.0.0", 12306, cgi_infos, m_packer);
	}

private:
	virtual void onServerCgiMgr_sessionCreated(socket_id_t sid, session_id_t ssid) override
	{
		printf("server: session created, sid=%" PRId64 ", ssid=%s\n", sid, ssid.toString().c_str());
	}

	virtual void onServerCgiMgr_sessionClosed(socket_id_t sid, session_id_t ssid) override
	{
		printf("server: session closed, sid=%" PRId64 ", ssid=%s\n", sid, ssid.toString().c_str());
	}

	virtual void onServerCgiMgr_recvC2sNotifyPack(std::unique_ptr<ServerCgi::RecvPack>* recv_pack) override
	{
		SimpleSvrNetworkConsoleLogic::onServerCgiMgr_recvC2sNotifyPack(recv_pack);
		ServerCgi::RecvPack* p = recv_pack->get();
		StPacker::Pack* st_pack = (StPacker::Pack*)p->m_recv_ext;
		if (p->m_recv_cmd_type == __ECgiCmdType_c2sNotify_notifyStatistic)
		{
			printf("server: recv notify=%s\n", (const char*)st_pack->m_body.getData());
		}
		else
		{
			recv_pack->reset();
			printf("server: recv unkonw c2s_notify pack\n");
		}
	}

	virtual void onServerCgiMgr_recvC2sReqPack(std::unique_ptr<ServerCgi::RecvPack>* recv_pack) override
	{
		SimpleSvrNetworkConsoleLogic::onServerCgiMgr_recvC2sReqPack(recv_pack);
		ServerCgi::RecvPack* p = recv_pack->get();
		__ServerCgiCtx cgi_ctx;
		cgi_ctx.m_callback = this;
		cgi_ctx.m_network = getNetwork();
		cgi_ctx.m_packer = m_packer;
		cgi_ctx.m_uin = 0;
		cgi_ctx.m_sid = p->m_sid;
		cgi_ctx.m_ssid = p->m_ssid;

		if (p->m_recv_cmd_type == __ECgiCmdType_c2sReq_checkVersion)
		{
			__ServerCgi_s2cResp_checkVersion* cgi = new __ServerCgi_s2cResp_checkVersion(cgi_ctx);
			cgi->setRecvPack(recv_pack->release());
			printf("server: recv req=%s\n", cgi->m_c2s_req_body.c_str());
			cgi->initSendPack("2.1.1", true, "client_2.1.1", 3 * 1024 * 1024);
			printf("server: send resp=%s\n", cgi->m_s2c_resp_body.c_str());
			if (!getCgiMgr()->startCgi(cgi))
			{
				printf("server: fail to start cgi\n");
			}
		}
		else
		{
			recv_pack->reset();
			printf("server: recv unkonw c2s_req pack\n");
		}
	}

	virtual void onServerCgi_cgiDone(ServerCgi* cgi) override
	{
		SimpleSvrNetworkConsoleLogic::onServerCgi_cgiDone(cgi);
		if (cgi->getSendPack() != NULL && cgi->getSendPack()->m_send_cmd_type == __ECgiCmdType_s2cResp_checkVersion)
		{
			if (cgi->getIsCgiSuccess())
				printf("server: s2cResp_checkVersion cgi ok\n");
			else
				printf("server: s2cResp_checkVersion cgi fail\n");
		}
		else
		{
			printf("server: unkonw cgi\n");
		}
		delete cgi;
	}

	StPacker* m_packer;
};







void __testServerNetwork()
{
	printf("\n__testServerNetwork ---------------------------------------------------------\n");
	__initLog(ELogLevel_debug);
	ConsoleApp* app = new ConsoleApp();
	__ServerNetworkLogic* logic = new __ServerNetworkLogic();
	app->run(logic);
	delete logic;
	delete app;
}


#endif

