//#ifndef TEST_CHAT_SERVER_H_
//#define TEST_CHAT_SERVER_H_
//#include "testServerNetwork.h"
//using namespace std;
//USING_NAMESPACE_S
//USING_NAMESPACE_SSVR
//
//
//
//
//
//// s2c resp sendText ---------------------------------------------------------------------------------------------
//class __ServerCgi_s2cResp_sendText : public ServerCgi
//{
//public:
//	virtual void initSendPack(ICallback* callback, const std::string& resp_text)
//	{
//		SendPack* send_pack = new SendPack();
//		send_pack->m_data.append((uint8_t*)resp_text.c_str(), resp_text.size() + 1);
//		setSendPack(send_pack);
//		setCallback(callback);
//	}
//
//	static const ServerCgiInfo & s_getServerCgiInfo()
//	{
//		s_cgi_info.m_cgi_type = EServerCgiType_c2sReq_s2cResp;
//		s_cgi_info.m_recv_cmd_type = __EServerCgiCmdType_c2sReq_sendText;
//		s_cgi_info.m_send_cmd_type = __EServerCgiCmdType_s2cResp_sendText;
//		return s_cgi_info;
//	}
//
//	virtual const ServerCgiInfo& getServerCgiInfo() const
//	{
//		return s_getServerCgiInfo();
//	}
//
//	virtual void setRecvPack(RecvPack* recv_pack)
//	{
//		ServerCgi::setRecvPack(recv_pack);
//		m_c2sReq_text = (const char*)recv_pack->m_data.getData();
//	}
//
//
//private:
//	static ServerCgiInfo s_cgi_info;
//public:
//	std::string m_c2sReq_text;
//};
//ServerCgiInfo __ServerCgi_s2cResp_sendText::s_cgi_info;
//
//
//// s2c push syncChanged --------------------------------------------------------------------------------
//class __ServerCgi_s2cPush_syncChanged : public ServerCgi
//{
//public:
//	virtual void initSendPack(ICallback* callback, const std::string& push_text)
//	{
//		SendPack* send_pack = new SendPack();
//		send_pack->m_data.append((uint8_t*)push_text.c_str(), push_text.size() + 1);
//		setSendPack(send_pack);
//		setCallback(callback);
//	}
//
//	static const ServerCgiInfo & s_getServerCgiInfo()
//	{
//		s_cgi_info.m_cgi_type = EServerCgiType_s2cPush;
//		s_cgi_info.m_send_cmd_type = __EServerCgiCmdType_s2cPush_syncChanged;
//		return s_cgi_info;
//	}
//
//	virtual const ServerCgiInfo& getServerCgiInfo() const
//	{
//		return s_getServerCgiInfo();
//	}
//
//private:
//	static ServerCgiInfo s_cgi_info;
//};
//ServerCgiInfo __ServerCgi_s2cPush_syncChanged::s_cgi_info;
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//// __ServerNetworkLogic ---------------------------------------------------------------------------------------
//class __ServerNetworkLogic : public SimpleSvrNetworkConsoleLogic
//{
//public:
//	__ServerNetworkLogic()
//	{
//		std::vector<ServerCgiInfo> cgi_infos;
//		cgi_infos.push_back(__ServerCgi_s2cResp_sendText::s_getServerCgiInfo());
//		cgi_infos.push_back(__ServerCgi_c2sNotify_notifyStatistic::s_getServerCgiInfo());
//		cgi_infos.push_back(__ServerCgi_s2cPush_syncChanged::s_getServerCgiInfo());
//		SimpleSvrNetworkConsoleLogic::init(0, NULL, "127.0.0.1", 12306, cgi_infos);
//	}
//
//	virtual void onTextMsg(const std::string & textMsg)
//	{
//		SimpleSvrNetworkConsoleLogic::onTextMsg(textMsg);
//	}
//
//	virtual void onServerCgiMgr_recvC2sNotifyPack(const ServerCgi::RecvPack& recv_pack)
//	{
//		SimpleSvrNetworkConsoleLogic::onServerCgiMgr_recvC2sNotifyPack(recv_pack);
//		if (recv_pack.m_cmd_type == __EServerCgiCmdType_c2sNotify_notifyStatistic)
//		{
//			printf("recv c2sNotify_notifyStatistic, data=%s\n", (const char*)recv_pack.m_data.getData());
//		}		
//		else
//		{
//			printf("recv unkonw c2s_notify pack\n");
//		}
//	}
//
//	virtual void onServerCgiMgr_recvC2sReqPack(const ServerCgi::RecvPack& recv_pack)
//	{
//		SimpleSvrNetworkConsoleLogic::onServerCgiMgr_recvC2sReqPack(recv_pack);
//		if (recv_pack.m_cmd_type == __EServerCgiCmdType_c2sReq_sendText)
//		{
//			__ServerCgi_s2cResp_sendText* cgi = new __ServerCgi_s2cResp_sendText();
//			cgi->setRecvPack(recv_pack.clone());
//			printf("recv c2sReq_sendText, text=%s\n", cgi->m_c2sReq_text.c_str());
//
//			cgi->initSendPack(this, std::string() + "ok gogogo, your req text is: " + cgi->m_c2sReq_text);
//			if (!getCgiMgr()->startCgi(cgi))
//			{
//				printf("fail to start cgi\n");
//			}
//		}
//		else
//		{
//			printf("recv unkonw c2s_req pack\n");
//		}
//	}
//
//	virtual void onServerCgi_cgiDone(ServerCgi* cgi)
//	{
//		SimpleSvrNetworkConsoleLogic::onServerCgi_cgiDone(cgi);
//		if (cgi->getSendPack() != NULL && cgi->getSendPack()->m_cmd_type == __EServerCgiCmdType_s2cResp_sendText)
//		{
//			if(cgi->getIsCgiSuccess())
//				printf("s2cResp_sendText cgi ok\n");
//			else
//				printf("s2cResp_sendText cgi fail\n");
//		}
//		else if (cgi->getSendPack() != NULL && cgi->getSendPack()->m_cmd_type == __EServercgiCmdType_s2cPush_dataChanged)
//		{
//			if (cgi->getIsCgiSuccess())
//				printf("s2cPush_dataChanged cgi ok\n");
//			else
//				printf("s2cPush_dataChanged cgi fail\n");
//		}
//		else
//		{
//			printf("unkonw cgi\n");
//		}
//		delete cgi;
//	}
//};
//
//
//void __testServerNetwork()
//{
//	__initLog();
//	ConsoleApp* app = new ConsoleApp();
//	__ServerNetworkLogic* logic = new __ServerNetworkLogic();
//	app->run(logic);
//	delete logic;
//	delete app;
//}
//
//
//#endif
