//#ifndef TEST_CHAT_CLIENT_H
//#define TEST_CHAT_CLIENT_H
//#include "testS.h"
//#include "testClientNetwork.h"
//using namespace std;
//USING_NAMESPACE_S
//USING_NAMESPACE_SCLIENT
//
//
//
//
//
//
//
//// authed: c2s_notify notifyWriting ------------------------------------------------------------------------------------------
//class __ClientCgi_c2sNotify_NotifyWriting : public ClientCgi
//{
//public:
//	bool init(ICallback* callback, const std::string& str)
//	{
//		ClientCgi::SendPack* send_pack = new ClientCgi::SendPack();
//		send_pack->m_cmd_type = __EClientCgiCmdType_c2sNotify_notifyWriting;
//		send_pack->m_data.append((const byte_t*)str.c_str(), str.size() + 1);
//		setSendPack(send_pack);
//		setCallback(callback);
//		return true;
//	}
//
//	static const ClientCgiInfo & s_getCgiInfo()
//	{
//		s_cgi_info.m_cgi_type = EClientCgiType_c2sNotify;
//		s_cgi_info.m_send_cmd_type = __EClientCgiCmdType_c2sNotify_notifyWriting;
//		return s_cgi_info;
//	}
//
//	virtual const ClientCgiInfo & getCgiInfo() const
//	{
//		return s_getCgiInfo();
//	}
//
//private:
//	static ClientCgiInfo s_cgi_info;
//};
//ClientCgiInfo __ClientCgi_c2sNotify_NotifyWriting::s_cgi_info;
//
//
//
//// authed: c2s_req init ------------------------------------------------------------------------------------------
//class __ClientCgi_c2sReq_Init : public ClientCgi
//{
//public:
//	bool init(ICallback* callback)
//	{
//		ClientCgi::SendPack* send_pack = new ClientCgi::SendPack();
//		send_pack->m_cmd_type = __EClientCgiCmdType_c2sReq_init;
//		std::string str = "init";
//		send_pack->m_data.append((const byte_t*)str.c_str(), str.size() + 1);
//		setSendPack(send_pack);
//		setCallback(callback);
//		return true;
//	}
//
//	static const ClientCgiInfo & s_getCgiInfo()
//	{
//		s_cgi_info.m_cgi_type = EClientCgiType_c2sReq_s2cResp;
//		s_cgi_info.m_send_cmd_type = __EClientCgiCmdType_c2sReq_init;
//		s_cgi_info.m_recv_cmd_type = __EClientCgiCmdType_s2cResp_init;
//		return s_cgi_info;
//	}
//
//	virtual const ClientCgiInfo & getCgiInfo() const
//	{
//		return s_getCgiInfo();
//	}
//
//	virtual void setRecvPack(RecvPack* recv_pack)
//	{
//		ClientCgi::setRecvPack(recv_pack);
//		if (recv_pack == NULL)
//			return;
//
//		m_s2c_resp_str = (const char*)recv_pack->m_data.getData();
//	}
//
//private:
//	static ClientCgiInfo s_cgi_info;
//public:
//	std::string m_s2c_resp_str;
//};
//ClientCgiInfo __ClientCgi_c2sReq_Init::s_cgi_info;
//
//
//
//// authed: c2s_req sendText ------------------------------------------------------------------------------------------
//class __ClientCgi_c2sReq_SendText : public ClientCgi
//{
//public:
//	bool init(ICallback* callback, const std::string& str)
//	{
//		ClientCgi::SendPack* send_pack = new ClientCgi::SendPack();
//		send_pack->m_cmd_type = __EClientCgiCmdType_c2sReq_sendText;
//		send_pack->m_data.append((const byte_t*)str.c_str(), str.size() + 1);
//		setSendPack(send_pack);
//		setCallback(callback);
//		return true;
//	}
//
//	static const ClientCgiInfo & s_getCgiInfo()
//	{
//		s_cgi_info.m_cgi_type = EClientCgiType_c2sReq_s2cResp;
//		s_cgi_info.m_send_cmd_type = __EClientCgiCmdType_c2sReq_sendText;
//		s_cgi_info.m_recv_cmd_type = __EClientCgiCmdType_s2cResp_sendText;
//		s_cgi_info.m_network_types = EClientCgiNetworkType_tcp;
//		return s_cgi_info;
//	}
//
//	virtual const ClientCgiInfo & getCgiInfo() const
//	{
//		return s_getCgiInfo();
//	}
//
//	virtual void setRecvPack(RecvPack* recv_pack)
//	{
//		ClientCgi::setRecvPack(recv_pack);
//		if (recv_pack == NULL)
//			return;
//
//		m_s2c_resp_str = (const char*)recv_pack->m_data.getData();
//	}
//
//private:
//	static ClientCgiInfo s_cgi_info;
//public:
//	std::string m_s2c_resp_str;
//};
//ClientCgiInfo __ClientCgi_c2sReq_SendText::s_cgi_info;
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
//// logic ------------------------------------------------------------------------------------------
//class __ChatClientLogic : public SimpleClientNetworkConsoleLogic
//{
//public:
//	__ChatClientLogic()
//	{
//		std::map<uint32_t, ClientCgiInfo> cgi_infos;
//		cgi_infos[__ClientCgi_c2sNotify_NotifyStatistic::s_getCgiInfo().m_send_cmd_type] = __ClientCgi_c2sNotify_NotifyStatistic::s_getCgiInfo();
//		cgi_infos[__ClientCgi_c2sReq_SendText::s_getCgiInfo().m_send_cmd_type] = __ClientCgi_c2sReq_SendText::s_getCgiInfo();
//		SimpleClientNetworkConsoleLogic::init("127.0.0.1", 12306, cgi_infos);
//	}
//
//private:
//	virtual void onTextMsg(const std::string & textMsg)
//	{
//		SimpleClientNetworkConsoleLogic::onTextMsg(textMsg);
//		
//		{
//			__ClientCgi_c2sNotify_NotifyStatistic* cgi = new __ClientCgi_c2sNotify_NotifyStatistic();
//			if (!cgi->init(this, std::string("notify:") + textMsg))
//			{
//				printf("fail to !cgi->init\n");
//				return;
//			}
//
//			if (!m_cgi_mgr->startCgi(cgi))
//			{
//				printf("fail to startCgi\n");
//				return;
//			}
//		}
//
//
//		{
//			__ClientCgi_c2sReq_SendText* cgi = new __ClientCgi_c2sReq_SendText();
//			if (!cgi->init(this, std::string("req:") + textMsg))
//			{
//				printf("fail to !cgi->init\n");
//				return;
//			}
//
//			if (!m_cgi_mgr->startCgi(cgi))
//			{
//				printf("fail to startCgi\n");
//				return;
//			}
//		}
//	}
//
//	virtual void onClientCgi_cgiDone(ClientCgi* cgi)
//	{
//		SimpleClientNetworkConsoleLogic::onClientCgi_cgiDone(cgi);
//		if (cgi->getCgiInfo().m_cgi_type == EClientCgiType_c2sNotify && cgi->getCgiInfo().m_send_cmd_type == __EClientCgiCmdType_c2sNotify_sendStatistic)
//		{
//			__ClientCgi_c2sNotify_NotifyStatistic* text_cgi = (__ClientCgi_c2sNotify_NotifyStatistic*)cgi;
//			if (cgi->getIsSuccess())
//				printf("notify_statistic cgi ok\n");
//			else
//				printf("notify_statistic cgi fail\n");
//		}
//		else if (cgi->getCgiInfo().m_cgi_type == EClientCgiType_c2sReq_s2cResp && cgi->getCgiInfo().m_send_cmd_type == __EClientCgiCmdType_c2sReq_sendText)
//		{
//			__ClientCgi_c2sReq_SendText* text_cgi = (__ClientCgi_c2sReq_SendText*)cgi;
//			if(cgi->getIsSuccess())
//				printf("send_text cgi ok, resp=%s\n", text_cgi->m_s2c_resp_str.c_str());
//			else
//				printf("send_text cgi fail\n");
//		}
//		else
//		{
//			printf("unknow cgi!\n");
//		}
//		delete cgi;
//	}
//};
//
//
//
//void __testChatClient()
//{
//	__initLog();
//	ConsoleApp* app = new ConsoleApp();
//	__ChatClientLogic* logic = new __ChatClientLogic();
//	app->run(logic);
//	delete logic;
//	delete app;
//}
//
//
//
//#endif
