#include "../../util/timeUtil.h"
#include "clientNetwork.h"
SCLIENT_NAMESPACE_BEGIN



// Msg ---------------------------------------------------------------------------------------------
class ClientNetwork::__Msg_notifyConectStateChanged : public Message
{
public:
	__Msg_notifyConectStateChanged(EConnectState connect_state)
	{
		m_msg_type = __EMsgType_notifyConectStateChanged; 
		m_connect_state = connect_state;
	}

	EConnectState m_connect_state;
};

class ClientNetwork::__Msg_notifyRecvS2cPushPack : public Message
{
public:
	__Msg_notifyRecvS2cPushPack(RecvPack* recv_pack)
	{
		m_msg_type = __EMsgType_notifyRecvS2cPushPack;
		m_recv_pack = recv_pack;
	}

	~__Msg_notifyRecvS2cPushPack()
	{
		delete m_recv_pack;
	}

	RecvPack* m_recv_pack;
};

class ClientNetwork::__Msg_notifyRecvS2cReqPack : public Message
{
public:
	__Msg_notifyRecvS2cReqPack(RecvPack* recv_pack)
	{
		m_msg_type = __EMsgType_notifyRecvS2cReqPack;
		m_recv_pack = recv_pack;
	}

	~__Msg_notifyRecvS2cReqPack()
	{
		delete m_recv_pack;
	}

	RecvPack* m_recv_pack;
};

class ClientNetwork::__Msg_notifyCgiDone : public Message
{
public:
	__Msg_notifyCgiDone(ClientCgi* cgi)
	{
		m_msg_type = __EMsgType_notifyCgiDone;
		m_cgi = cgi;
	}

	~__Msg_notifyCgiDone()
	{
	}

	ClientCgi* m_cgi;
};




// ClientCgi ------------------------------------------------------------------------------------------
void ClientNetwork::ClientCgi::setSendPack(SendPack * send_pack) 
{ 
	m_send_pack = send_pack; 
	if (send_pack != nullptr)
	{
		if (send_pack->m_send_cmd_type != getCgiInfo().m_send_cmd_type)
		{
			slog_e("ClientNetwork::ClientCgi::setSendPack send_pack->m_send_cmd_type(%) != getCgiInfo().m_send_cmd_type(%0)", send_pack->m_send_cmd_type, getCgiInfo().m_send_cmd_type);
		}
	}
}

void ClientNetwork::ClientCgi::setRecvPack(RecvPack * recv_pack) 
{
	m_recv_pack = recv_pack;
	if (recv_pack != nullptr)
	{
		if (recv_pack->m_recv_cmd_type != getCgiInfo().m_recv_cmd_type)
		{
			slog_e("ClientNetwork::ClientCgi::setSendPack recv_pack->m_recv_cmd_type(%) != getCgiInfo().m_recv_cmd_type(%0)", recv_pack->m_recv_cmd_type, getCgiInfo().m_recv_cmd_type);
		}
	}
	onSetRecvPackEnd();
}




// __CgiCtx ------------------------------------------------------------------------------------------
class ClientNetwork::__CgiCtx
{
public:
	__CgiCtx() { m_is_sent = false; m_create_time = 0; m_cgi = nullptr; m_try_count = 0; }
	~__CgiCtx() { }

	bool m_is_sent;
	uint64_t m_create_time;
	ClientCgi* m_cgi;
	size_t m_try_count;
};




// __ClientCtx ------------------------------------------------------------------------------------------
class ClientNetwork::__ClientCtx
{
public:
	__ClientCtx(InitParam* param, ClientNetwork* network)
	{
		m_init_param = param; 
		m_network = network;
		m_connect_interval_mss = m_init_param->m_connect_interval_mss;

		m_sid = 0;
		m_connect_state = __EConnectState_disconnected;
		m_sending_cgi_index = -1;
		m_svr_port = 0;

		m_is_repeat_last_connect_interval_ms = true;
		m_last_reconnect_time_ms = 0;
		m_connect_count = 0;
	}

	~__ClientCtx()
	{
		stop();
	}

	bool createSocket(const std::string& svr_ip_or_name, const std::string& svr_ip, uint32_t svr_port)
	{
		if (m_sid != 0)
		{
			slog_e("ClientNetwork::__ClientCtx::createSocket already create socket.");
			return false;
		}

		m_connect_count = 0;
		ITcpSocketCallbackApi::CreateClientSocketParam param;
		param.m_callback_looper = m_init_param->m_work_looper;
		param.m_callback_target = m_network;
		param.m_svr_ip = svr_ip;
		param.m_svr_port = svr_port;
		if (!m_init_param->m_sapi->createClientSocket(&m_sid, param))
			return false;

		m_svr_ip_or_name = svr_ip_or_name;
		m_svr_ip = svr_ip;
		m_svr_port = svr_port;

		return true;
	}

	void stop()
	{
		if (m_sid == 0)
			return;

		m_init_param->m_sapi->stopClientSocket(m_sid);
		__resetConnectState();
		delete_and_erase_collection_elements(&m_cgi_ctxs);

		m_init_param->m_work_looper->removeMessagesBySender(this);
	}

	bool connect()
	{
		if (m_connect_state == __EConnectState_connected || m_connect_state == __EConnectState_connecting)
			return true;

		if (m_sid == 0)
		{
			slog_e("ClientNetwork::__ClientCtx::connect, fail, sid=0");
			return false;
		}

		if (!__isTimeToConnect())
			return true;

		slog_v("doConnectSvr, connect_cout=%0", m_connect_count);
		m_last_reconnect_time_ms = TimeUtil::getMsTime();
		++m_connect_count;

		// connect fasterst svr
		slog_d("doConnectSvr, svr_name=%0, svr_ip=%1, svr_port=%2", m_svr_ip_or_name, m_svr_ip.c_str(), m_svr_port);
		if (!m_init_param->m_sapi->startClientSocket(m_sid))
		{
			slog_e("ClientNetwork::__ClientCtx::connect fail to startClientSocket");
			return false;
		}

		m_connect_state = __EConnectState_connecting;
		m_init_param->m_callback->onClientNetwork_connectStateChanged(m_network, EConnectState_connecting);
		return true;
	}
	
	bool startCgi(ClientCgi* cgi)
	{
		if (cgi == nullptr || cgi->getSendPack() == nullptr)
		{
			slog_e("ClientNetwork::__ClientCtx::startCgi fail, cgi param err");
			return false;
		}

		SendPack& send_pack = *cgi->getSendPack();
		if (send_pack.m_send_whole_pack_bin.getLen() == 0)
		{
			slog_e("ClientNetwork::__ClientCtx::startCgi fail, len == 0. %0", send_pack.toOverviewString());
			return false;
		}

		const ClientCgiInfo* cgi_info = &cgi->getCgiInfo();
		if ((cgi_info->m_cgi_type == EClientCgiType_c2sReq_s2cResp && send_pack.m_send_seq == 0)
			|| (cgi_info->m_cgi_type == EClientCgiType_c2sNotify && send_pack.m_send_seq != 0)
			|| (cgi_info->m_cgi_type == EClientCgiType_c2sReq_s2cResp && send_pack.m_send_seq == 0))
		{
			slog_e("ClientNetwork::__ClientCtx::startCgi, invalid send_seq! %0", send_pack.toOverviewString());
			return false;
		}

		if (m_sid == 0)
		{
			slog_e("ClientNetwork::__ClientCtx::startCgi, fail, sid=0");
			return false;
		}

		if (m_cgi_ctxs.size() >= m_init_param->m_max_pack_count)
		{
			slog_e("ClientNetwork::__ClientCtx::startCgi, too many packs in network");
			return false;
		}

		int cgi_index = __getCgiIndexBySendPackId(cgi->getSendPack()->m_send_pack_id);
		if (cgi_index >= 0)
		{
			slog_d("ClientNetwork::__ClientCtx::startCgi cgi already start? ignore");
			return true;
		}

		size_t pack_count = __getCgiCountBySendPackCmdType(send_pack.m_send_cmd_type);
		if (pack_count >= cgi_info->m_max_pack_count_in_queue)
		{
			slog_e("ClientNetwork::__ClientCtx::startCgi fail, too many send pack in queue in network! %0", send_pack.toOverviewString());
			return false;
		}

		slog_d("sendPack, %0", send_pack.toOverviewString());
		__addCgi(cgi);

		return true;
	}
	
	void cancelCgi(ClientCgi* cgi)
	{
		if (m_sid == 0)
			return;

		int cgi_index = __getCgiIndexBySendPackId(cgi->getSendPack()->m_send_pack_id);
		if (cgi_index < 0)
			return;

		if (m_sending_cgi_index == cgi_index)
		{
			m_sending_cgi_index = -1;
		}

		delete_and_erase_vector_element_by_index(&m_cgi_ctxs, cgi_index);
	}
	
	bool sendPack()
	{
		if (m_sid == 0)
			return false;

		checkTimeOutPacks();

		// need connect
		if (m_connect_state != __EConnectState_connected)
			return false;

		// already sending
		if (m_sending_cgi_index >= 0)
			return true;

		// get max priory pack and send
		int to_send_cgi_index = __getMaxPrioryCgiIndex();
		if (to_send_cgi_index < 0)
			return true;

		SendPack* send_pack = m_cgi_ctxs[to_send_cgi_index]->m_cgi->getSendPack();
		if (!m_init_param->m_sapi->sendDataFromClientSocketToSvr(m_sid, send_pack->m_send_whole_pack_bin.getData(), send_pack->m_send_whole_pack_bin.getLen()))
		{
			slog_w("fail to sendDataFromClientSocketToSvr, maybe is disconnected, will retry");
			return false;
		}

		slog_d("send pack ok, cmd_type=%0, pack_data_len=%1, pack_len=%2", send_pack->m_send_cmd_type, send_pack->m_send_whole_pack_bin.getLen(), send_pack->m_send_whole_pack_bin.getLen());
		m_cgi_ctxs[to_send_cgi_index]->m_try_count++;
		m_sending_cgi_index = to_send_cgi_index;

		return true;
	}
	
	void checkTimeOutPacks()
	{
		if (m_sid == 0)
			return;

		uint64_t cur_time_ms = TimeUtil::getMsTime();
		for (size_t i = 0; i < m_cgi_ctxs.size(); ++i)
		{
			__CgiCtx* cgi_ctx = m_cgi_ctxs[i];
			bool is_time_out = cur_time_ms - cgi_ctx->m_create_time >= cgi_ctx->m_cgi->getCgiInfo().m_cgi_time_out_ms;
			if (is_time_out)
			{
				__markCgiDoneByIndex((int)i, EErrType_local, ELocalErrCode_sendPackTimeOutErr);
				--i;
			}
		}
	}
	
	socket_id_t getSid() const
	{
		return m_sid;
	}

	void onSendDataEnd()
	{
		if (m_sid == 0)
			return;

		int sending_pack_index = m_sending_cgi_index;
		m_sending_cgi_index = -1;

		if (sending_pack_index >= 0)
		{
			__CgiCtx* cgi_ctx = m_cgi_ctxs[sending_pack_index];
			ClientCgi* cgi = cgi_ctx->m_cgi;
			if (cgi->getCgiInfo().m_cgi_type == EClientCgiType_c2sReq_s2cResp) // need to wait s2cResp pack
			{
				cgi_ctx->m_is_sent = true;
			}
			else if (cgi->getCgiInfo().m_cgi_type == EClientCgiType_c2sNotify || cgi->getCgiInfo().m_cgi_type == EClientCgiType_s2cReq_cs2Resp)
			{
				__markCgiDoneByIndex(sending_pack_index, EErrType_ok, 0);
			}
			else
			{
				slog_e("__ClientCtx::onSendDataEnd invalid path, cgi_type=%0", cgi_ctx->m_cgi->getCgiInfo().m_cgi_type);
			}
		}

		sendPack();
	}
	
	void onRecvData(const Binary& recv_data)
	{
		if (m_sid == 0)
			return;

		slog_d("recv data len=%0", recv_data.getLen());
		m_recv_data.append(recv_data);

		while (true)
		{
			//if (!m_is_running)
			//	break;

			UnpackResult r;
			m_init_param->m_unpacker->unpackClientRecvPack(m_recv_data.getData(), m_recv_data.getLen(), &r);
			if (r.m_result_type == EUnpackResultType_ok)
			{
				slog_d("unpack ok");
				m_recv_data.shrinkBegin(r.m_unpack_raw_data_len);

				ClientCgiInfo* cgi_info = __getClientCgiInfoByRecvCmdType(r.m_recv_cmd_type);
				if (cgi_info == nullptr)
				{
					slog_w("ClientNetwork::__onMsgTcpSocketClientRecvData fail to get cgi_info, recv_pack.recv_cmd_type=%0, ignore", r.m_recv_cmd_type);
					continue;
				}

				RecvPack* recv_pack = new RecvPack();
				recv_pack->m_recv_cmd_type = r.m_recv_cmd_type;
				recv_pack->m_recv_seq = r.m_recv_seq;
				recv_pack->m_recv_ext = r.m_recv_ext;
				__onRecvPack(recv_pack); // ignore err.

				if (m_recv_data.getLen() == 0)
					break;
			}
			else if (r.m_result_type == EUnpackResultType_needMoreData)
			{
				break;
			}
			else // fatal error
			{
				slog_e("ClientNetwork::__onMsgTcpSocketClientRecvData fail to unpack. try disconnect");
				stop();// TODO: not stop, but disconnect, mark all sent cgi fail once.
				break;
			}
		}
	}
	
	void onConnected()
	{
		if (m_sid == 0)
			return;

		slog_d("connected, svr_name=%0, svr_ip=%1, svr_port=%2", m_svr_ip_or_name, m_svr_ip.c_str(), m_svr_port);
		if (m_connect_state == __EConnectState_connected)
			return;

		m_connect_state = __EConnectState_connected;
		m_connect_count = 0;

		sendPack();

		__notifyConnectStateChanged(EConnectState_connected);
	}
	
	void onDisconnected()
	{
		if (m_sid == 0)
			return;

		slog_d("disconnected, svr_name=%0, svr_ip=%1, svr_port=%2", m_svr_ip_or_name, m_svr_ip.c_str(), m_svr_port);
		if (m_connect_state == __EConnectState_disconnected)
			return;

		__resetConnectState();


		for (size_t i = 0; i < m_cgi_ctxs.size(); ++i)
		{
			__CgiCtx* cgi_ctx = m_cgi_ctxs[i];
			if (cgi_ctx->m_is_sent)  // need to resend
			{
				cgi_ctx->m_is_sent = false;

				if (cgi_ctx->m_try_count >= cgi_ctx->m_cgi->getMaxTryCount())
				{
					__markCgiDoneByIndex((int)i, EErrType_local, ELocalErrCode_sendPackSysErr);
					--i;
				}
			}
		}

		__notifyConnectStateChanged(EConnectState_disconnected);
	}





private:
	void __onRecvPack(RecvPack* recv_pack)
	{
		std::unique_ptr<RecvPack> recv_pack_ap(recv_pack);

		ClientCgiInfo* cgi_info = __getClientCgiInfoByRecvCmdType(recv_pack->m_recv_cmd_type);
		if (cgi_info->m_cgi_type == EClientCgiType_s2cPush)
		{
			__notifyRecvS2cPushPack(recv_pack_ap.release());
		}
		else if (cgi_info->m_cgi_type == EClientCgiType_s2cReq_cs2Resp)
		{
			__notifyRecvS2cReqPack(recv_pack_ap.release());
		}
		else if (cgi_info->m_cgi_type == EClientCgiType_c2sReq_s2cResp)
		{
			int cgi_indx = __getCgiIndexBySendPackSeq(recv_pack->m_recv_seq);
			if (cgi_indx < 0)
			{
				slog_d("ClientNetwork::__ClientCtx::onRecvPack recv s2cResp pack, but can't find c2sReq pack, maybe cgi is canceled. ignore.");
				return;
			}

			m_cgi_ctxs[cgi_indx]->m_cgi->setRecvPack(recv_pack_ap.release());

			__markCgiDoneByIndex(cgi_indx, EErrType_ok, 0);
		}
		else
		{
			slog_w("ClientNetwork::__ClientCtx::onRecvPack recv c2sPush_pack! ignore.");
		}
	}
	
	void __resetConnectState()
	{
		m_connect_state = __EConnectState_disconnected;
		m_recv_data.clear();
		m_sending_cgi_index = -1;
	}
	
	void __addCgi(ClientCgi* cgi)
	{
		cgi->setStartMs(TimeUtil::getMsTime());

		__CgiCtx* cgi_ctx = new __CgiCtx();
		cgi_ctx->m_cgi = cgi;
		cgi_ctx->m_create_time = TimeUtil::getMsTime();
		m_cgi_ctxs.push_back(cgi_ctx);
	}
	
	void __markCgiDoneByIndex(int index, EErrType err_type, int err_code)
	{
		if (index < 0 || index >= m_cgi_ctxs.size())
			return;

		__CgiCtx* cgi_ctx = m_cgi_ctxs[index];
		m_cgi_ctxs.erase(m_cgi_ctxs.begin() + index);
		ClientCgi* cgi = cgi_ctx->m_cgi;
		delete cgi_ctx;

		{
			cgi->setErrType(err_type);
			cgi->setErrCode(err_code);
			cgi->setEndMs(TimeUtil::getMsTime());
			if (cgi->getCgiInfo().m_cgi_type == EClientCgiType_s2cPush)
			{
				slog_e("ClientNetwork::__ClientCtx::__markCgiDoneByCgiCtx invalid path");
			}

			slog_v("call back cgi=%0", (uint64_t)cgi);
			__notifyCgiDone(cgi);
		}
	}
	
	bool __isTimeToConnect()
	{
		uint64_t connect_interval_ms = __getConnectIntervalMs(m_connect_count);
		uint64_t cur_time_ms = TimeUtil::getMsTime();
		bool is_time_to_connect = cur_time_ms > m_last_reconnect_time_ms + connect_interval_ms;
		return is_time_to_connect;
	}
	
	uint64_t __getConnectIntervalMs(size_t connect_count)
	{
		if (m_connect_interval_mss.size() == 0)
			return -1;

		if (connect_count <= m_connect_interval_mss.size() - 1)
		{
			return m_connect_interval_mss[connect_count];
		}
		else
		{
			if (m_is_repeat_last_connect_interval_ms)
				return *m_connect_interval_mss.rbegin();
			else
				return -1;
		}
	}
	
	int __getMaxPrioryCgiIndex()
	{
		if (m_cgi_ctxs.size() == 0)
			return -1;

		int max_priory_pack_index = -1;
		int max_priory = 0;
		for (size_t i = 0; i < m_cgi_ctxs.size(); ++i)
		{
			__CgiCtx* ctx = m_cgi_ctxs[i];
			if (ctx->m_is_sent)
				continue;
			if (ctx->m_cgi->getCgiInfo().m_priority > max_priory)
			{
				max_priory = ctx->m_cgi->getCgiInfo().m_priority;
				max_priory_pack_index = (int)i;
			}
		}

		return max_priory_pack_index;
	}
	
	int __getCgiIndexBySendPackId(uint64_t send_pack_id)
	{
		for (size_t i = 0; i < m_cgi_ctxs.size(); ++i)
		{
			if (m_cgi_ctxs[i]->m_cgi->getSendPack()->m_send_pack_id == send_pack_id)
				return (int)i;
		}
		return -1;
	}
	
	int __getCgiIndexBySendPackSeq(uint64_t send_pack_seq)
	{
		for (size_t i = 0; i < m_cgi_ctxs.size(); ++i)
		{
			__CgiCtx* ctx = m_cgi_ctxs[i];
			if (ctx->m_cgi->getSendPack()->m_send_seq == send_pack_seq)
				return (int)i;
		}
		return -1;
	}
	
	size_t __getCgiCountBySendPackCmdType(uint32_t send_pack_cmd_type)
	{
		return 0;
	}
	
	ClientCgiInfo* __getClientCgiInfoByRecvCmdType(uint32_t recv_cmd_type)
	{
		for (auto it = m_init_param->m_send_cmd_type_to_cgi_info_map.begin(); it != m_init_param->m_send_cmd_type_to_cgi_info_map.end(); ++it)
		{
			if (it->second.m_recv_cmd_type == recv_cmd_type)
				return &it->second;
		}
		return nullptr;
	}
	
	void __postMsgToNetwork(Message* msg)
	{
		msg->m_sender = this;
		msg->m_target = m_network;
		m_init_param->m_work_looper->postMessage(msg);
	}
	
	void __notifyConnectStateChanged(EConnectState state)
	{
		__Msg_notifyConectStateChanged* m = new __Msg_notifyConectStateChanged(state);
		__postMsgToNetwork(m);
	}
	
	void __notifyRecvS2cPushPack(RecvPack* recv_pack)
	{
		__Msg_notifyRecvS2cPushPack* m = new __Msg_notifyRecvS2cPushPack(recv_pack);
		__postMsgToNetwork(m);
	}
	
	void __notifyRecvS2cReqPack(RecvPack* recv_pack)
	{
		__Msg_notifyRecvS2cReqPack* m = new __Msg_notifyRecvS2cReqPack(recv_pack);
		__postMsgToNetwork(m);
	}
	
	void __notifyCgiDone(ClientCgi* cgi)
	{
		__Msg_notifyCgiDone* m = new __Msg_notifyCgiDone(cgi);
		__postMsgToNetwork(m);
	}





	InitParam* m_init_param;
	ClientNetwork* m_network;

	std::string m_svr_ip_or_name;
	std::string m_svr_ip;
	uint32_t m_svr_port;
	socket_id_t m_sid;
	__EConnectState m_connect_state;
	std::vector<int32_t> m_connect_interval_mss;
	bool m_is_repeat_last_connect_interval_ms;
	uint64_t m_last_reconnect_time_ms;
	size_t m_connect_count;

	Binary m_recv_data;

	int m_sending_cgi_index;
	std::vector<__CgiCtx*> m_cgi_ctxs;
};



















// ClientNetwork ------------------------------------------------------------------------------------------
ClientNetwork::ClientNetwork()
{
	slog_d("new ClientNetwork=%0", (uint64_t)this);
	m_is_running = false;
	m_timer_id = 0;

	m_client_ctx = nullptr;

	m_speed_tester = nullptr;
	m_is_testing_speed = false;

	m_dns_resolver = nullptr;
}

ClientNetwork::~ClientNetwork()
{
	slog_d("delete ClientNetwork=%0", (uint64_t)this);
	stop();

	m_init_param.m_work_looper->releasseTimer(m_timer_id);
	delete m_client_ctx;
	delete m_speed_tester;
	delete m_dns_resolver;
}

bool ClientNetwork::init(const InitParam& param)
{
	slog_d("init ClientNetwork");
	if (param.m_sapi == NULL || param.m_svr_infos.size() == 0 || param.m_send_cmd_type_to_cgi_info_map.size() == 0 
		|| param.m_unpacker == NULL || param.m_callback == nullptr)
	{
		slog_e("ClientNetwork::init fail, param is error");
		return false;
	}
	m_init_param = param;

	m_timer_id = m_init_param.m_work_looper->createTimer(NULL);
	m_client_ctx = new __ClientCtx(&m_init_param, this);
	
	// dns_resolver
	{
		if (m_init_param.m_dns_resolver == nullptr)
		{
			m_dns_resolver = new DnsResolver();
			if (!m_dns_resolver->init(m_init_param.m_work_looper))
				return false;
			m_init_param.m_dns_resolver = m_dns_resolver;
		}
	}

	// speed_tester
	{
		std::vector<ClientNetSpeedTester::SvrInfo> svr_infos;
		for (size_t i = 0; i < m_init_param.m_svr_infos.size(); ++i)
		{
			ClientNetSpeedTester::SvrInfo svr_info;
			svr_info.m_svr_ip_or_name = m_init_param.m_svr_infos[i].m_svr_ip_or_name;
			svr_info.m_svr_port = m_init_param.m_svr_infos[i].m_svr_port;
			svr_infos.push_back(svr_info);
		}

		ClientNetSpeedTester::InitParam p;
		p.m_notify_looper = m_init_param.m_work_looper;
		p.m_notify_target = this;
		p.m_work_looper = m_init_param.m_work_looper;
		p.m_sapi = m_init_param.m_sapi;
		p.m_dns_resolver = m_init_param.m_dns_resolver;
		p.m_svr_infos = svr_infos;

		m_speed_tester = new ClientNetSpeedTester();
		if (!m_speed_tester->init(p))
		{
			slog_e("ClientNetwork::init fail to m_speed_tester->init");
			return false;
		}
	}

	return true;
}

bool ClientNetwork::start()
{
	if (m_is_running)
	{
		slog_d("ClientNetwork::start already start, ignore.");
		return true;
	}
	m_is_running = true;
	slog_d("network start");

	m_init_param.m_work_looper->addMsgHandler(this);
	m_init_param.m_work_looper->addMsgTimerHandler(this);
	m_init_param.m_dns_resolver->addNotifyLooper(m_init_param.m_work_looper, this);

	if (!__doTestSvrSpeed())
	{
		slog_e("ClientNetwork::start fail to __doTestSvrSpeed");
		return false;
	}
	
	if (!m_init_param.m_work_looper->startTimer(m_timer_id, 1, 1))
	{
		slog_e("ClientNetwork::start fail to startTimer");
		return false;
	}
	
	__notifyStarted();
	return true;
}

void ClientNetwork::stop()
{
	if (!m_is_running)
		return;
	m_is_running = false;
	slog_d("network stop");

	m_init_param.m_work_looper->removeMsgHandler(this);
	m_init_param.m_work_looper->removeMsgTimerHandler(this);
	m_init_param.m_dns_resolver->removeNotifyLooper(m_init_param.m_work_looper, this);

	m_client_ctx->stop();
	m_speed_tester->stop();

	m_init_param.m_work_looper->stopTimer(m_timer_id);

	__notifyStopped();
	slog_d("network stop end");
}

ClientNetwork::SendPack* ClientNetwork::newSendPack(uint64_t send_pack_id, uint32_t send_cmd_type, uint32_t send_seq)
{
	ClientCgiInfo* cgi_info = __getClientCgiInfoBySendCmdType(send_cmd_type);
	if (cgi_info == nullptr)
	{
		slog_e("ClientNetwork::newSendPack fail to __getClientCgiInfoBySendCmdType, send_cmd_type=%0", send_cmd_type);
		return nullptr;
	}
	if (cgi_info->m_cgi_type == EClientCgiType_c2sNotify && send_seq != 0)
	{
		slog_e("ClientNetwork::newSendPack fail, m_cgi_type == EClientCgiType_c2sNotify && send_seq != 0. send_cmd_type=%0", send_cmd_type);
		return nullptr;
	}

	SendPack* send_pack = new ClientNetwork::SendPack;
	send_pack->m_send_pack_id = send_pack_id;
	send_pack->m_send_cmd_type = send_cmd_type;
	send_pack->m_send_seq = send_seq;
	return send_pack;
}

bool ClientNetwork::startCgi(ClientCgi * cgi)
{
	slog_v("start cgi, cgi=%0", (uint64_t)cgi);
	if (cgi == nullptr || cgi->getSendPack() == nullptr)
	{
		slog_e("ClientNetwork::startCgi fail, cgi param err");
		return false;
	}

	SendPack& send_pack = *cgi->getSendPack();
	if (!m_is_running)
	{
		slog_e("ClientNetwork::startCgi fail, !m_is_running. %0", send_pack.toOverviewString());
		return false;
	}

	if (!m_client_ctx->startCgi(cgi))
		return false;

	__postSendPackMsgToSelf();
	return true;
}

void ClientNetwork::stopCgi(ClientCgi * cgi)
{
	slog_v("stop cgi, cgi=%0", (uint64_t)cgi);
	m_client_ctx->cancelCgi(cgi);
}



void ClientNetwork::onMessage(Message * msg, bool* isHandled)
{
	if (msg->m_target != this)
		return;
	*isHandled = true;

	if (!m_is_running)
		return;

	if (msg->m_sender == m_speed_tester)
	{
		switch (msg->m_msg_type)
		{
		case ClientNetSpeedTester::EMsgType_onTestStart: __onMsgNetSpeedTestStart(msg); break;
		case ClientNetSpeedTester::EMsgType_onOneTestResult: __onMsgNetSpeedTestResultUpdate(msg); break;
		case ClientNetSpeedTester::EMsgType_onTestEnd: __onMsgNetSpeedTestEnd(msg); break;
		}
		return;
	}
	else if (msg->m_sender == this)
	{
		switch (msg->m_msg_type)
		{
		case __EMsgType_sendPack:
			__onMsgSendPack(msg); 
			break;
		}
	}
	else if (msg->m_sender == m_client_ctx)
	{
		switch (msg->m_msg_type)
		{
		case __EMsgType_notifyStarted:
			{
				m_init_param.m_callback->onClientNetwork_started(this);
			}
			break;

		case __EMsgType_notifyStopped:
			{
				m_init_param.m_callback->onClientNetwork_stopped(this);
			}
				break;

		case __EMsgType_notifyConectStateChanged:
			{
				__Msg_notifyConectStateChanged* m = (__Msg_notifyConectStateChanged*)msg;
				m_init_param.m_callback->onClientNetwork_connectStateChanged(this, m->m_connect_state);
			}
			break;

		case __EMsgType_notifyRecvS2cPushPack:
			{
				__Msg_notifyRecvS2cPushPack* m = (__Msg_notifyRecvS2cPushPack*)msg;
				std::unique_ptr<RecvPack> recv_pack(m->m_recv_pack);
				m->m_recv_pack = nullptr;
				m_init_param.m_callback->onClientNetwork_recvS2cPushPack(this, &recv_pack);
			}
			break;

		case __EMsgType_notifyRecvS2cReqPack:
			{
				__Msg_notifyRecvS2cReqPack* m = (__Msg_notifyRecvS2cReqPack*)msg;
				std::unique_ptr<RecvPack> recv_pack(m->m_recv_pack);
				m->m_recv_pack = nullptr;
				m_init_param.m_callback->onClientNetwork_recvS2cReqPack(this, &recv_pack);
			}
			break;

		case __EMsgType_notifyCgiDone:
			{
				__Msg_notifyCgiDone* m = (__Msg_notifyCgiDone*)msg;
				m_init_param.m_callback->onClientNetwork_cgiDone(this, m->m_cgi);
			}
			break;
		}
	}
	else if (msg->m_sender == m_init_param.m_sapi)
	{
		switch (msg->m_msg_type)
		{
		case ITcpSocketCallbackApi::EMsgType_clientSocketConnected: __onMsgTcpSocketClientConnected((ITcpSocketCallbackApi::ClientSocketConnectedMsg*)msg); break;
		case ITcpSocketCallbackApi::EMsgType_clientSocketDisconnected: __onMsgTcpSocketClientDisconnected((ITcpSocketCallbackApi::ClientSocketDisconnectedMsg*)msg); break;
		case ITcpSocketCallbackApi::EMsgType_clientSocketRecvData: __onMsgTcpSocketClientRecvData((ITcpSocketCallbackApi::ClientSocketRecvDataMsg*)msg); break;
		case ITcpSocketCallbackApi::EMsgType_clientSocketSendDataEnd: __onMsgTcpSocketClientSendDataEnd((ITcpSocketCallbackApi::ClientSocketSendDataEndMsg*)msg); break;
		}
		return;
	}
}

void ClientNetwork::onMessageTimerTick(uint64_t timer_id, void* user_data)
{
	if (!m_is_running)
		return;
	if (timer_id != m_timer_id)
		return;

	__doConnectTcpSvr();
	m_client_ctx->checkTimeOutPacks();
}

void ClientNetwork::__onMsgSendPack(Message * msg)
{
	m_client_ctx->sendPack();
}

void ClientNetwork::__onMsgTcpSocketClientConnected(ITcpSocketCallbackApi::ClientSocketConnectedMsg * msg)
{
	if (msg->m_client_sid != m_client_ctx->getSid())
		return;

	m_client_ctx->onConnected();
}

void ClientNetwork::__onMsgTcpSocketClientDisconnected(ITcpSocketCallbackApi::ClientSocketDisconnectedMsg * msg)
{
	if (msg->m_client_sid != m_client_ctx->getSid())
		return;

	m_client_ctx->onDisconnected();
}

void ClientNetwork::__onMsgTcpSocketClientSendDataEnd(ITcpSocketCallbackApi::ClientSocketSendDataEndMsg * msg)
{
	if (msg->m_client_sid != m_client_ctx->getSid())
		return;
	m_client_ctx->onSendDataEnd();
}

void ClientNetwork::__onMsgTcpSocketClientRecvData(ITcpSocketCallbackApi::ClientSocketRecvDataMsg * msg)
{
	if (msg->m_client_sid != m_client_ctx->getSid())
		return;
	m_client_ctx->onRecvData(msg->m_recv_data);
}

void ClientNetwork::__onMsgNetSpeedTestStart(Message * msg)
{
	//slog_v("SpeedTestStart");
	m_is_testing_speed = true;
}

void ClientNetwork::__onMsgNetSpeedTestEnd(Message * msg)
{
	//slog_v("SpeedTestEnd");
	m_is_testing_speed = false;
}

void ClientNetwork::__onMsgNetSpeedTestResultUpdate(Message * msg)
{
	slog_v("ClientNetwork::SpeedTestReultUpdate");
	ClientNetSpeedTester::Msg_oneTestResult* m = (ClientNetSpeedTester::Msg_oneTestResult*)msg;

	int index = __getSvrInfoIndexBySvrIpAndPort(m->m_svr_ip_or_name, m->m_svr_port);
	if (index < 0)
		return;
	SvrInfo* svr_info = &(m_init_param.m_svr_infos[index]);
	if (m_client_ctx->getSid() != 0)
		return;

	if (!m->m_is_connected)
		return;

	m_speed_tester->stop();

	// init client and connect
	m_client_ctx->createSocket(m->m_svr_ip_or_name, m->m_svr_ip_str, m->m_svr_port);
	m_client_ctx->connect();
}



bool ClientNetwork::__doConnectTcpSvr()
{
	// find out the fastest svr
	if (m_client_ctx->getSid() == 0)
	{
		if (m_is_testing_speed)
		{
			slog_v("is testing speed, wait, ignore connect cmd");
			return true;
		}

		return __doTestSvrSpeed();
	}

	bool is_ok = m_client_ctx->connect();
	return is_ok;
}

bool ClientNetwork::__doTestSvrSpeed()
{
	if (!m_speed_tester->start())
	{
		slog_e("ClientNetwork::__doTestSvrSpeed fail to m_speed_tester->start");
		return false;
	}
	m_is_testing_speed = true;
	return true;
}

void ClientNetwork::__postSendPackMsgToSelf()
{
	Message* msg = new Message();
	msg->m_msg_type = __EMsgType_sendPack;
	__postMsgToSelf(msg);
}

void ClientNetwork::__postMsgToSelf(Message * msg)
{
	msg->m_sender = this;
	msg->m_target = this;
	m_init_param.m_work_looper->postMessage(msg);
}

void ClientNetwork::__notifyStarted()
{
	Message* m = new Message();
	m->m_msg_type = __EMsgType_notifyStarted;
	__postMsgToSelf(m);
}

void ClientNetwork::__notifyStopped()
{
	Message* m = new Message();
	m->m_msg_type = __EMsgType_notifyStopped;
	__postMsgToSelf(m);
}

int ClientNetwork::__getSvrInfoIndexBySvrIpAndPort(const std::string& svr_ip, uint32_t prot)
{
	for (size_t i = 0; i < m_init_param.m_svr_infos.size(); ++i)
	{
		if (m_init_param.m_svr_infos[i].m_svr_ip_or_name == svr_ip && m_init_param.m_svr_infos[i].m_svr_port == prot)
		{
			return (int)i;
		}
	}
	return -1;
}

ClientCgiInfo * ClientNetwork::__getClientCgiInfoBySendCmdType(uint32_t send_cmd_type)
{
	auto it = m_init_param.m_send_cmd_type_to_cgi_info_map.find(send_cmd_type);
	if (it == m_init_param.m_send_cmd_type_to_cgi_info_map.end())
		return NULL;
	return &it->second;
}

ClientCgiInfo* ClientNetwork::__getClientCgiInfoByRecvCmdType(uint32_t recv_cmd_type)
{
	for (auto it = m_init_param.m_send_cmd_type_to_cgi_info_map.begin(); it != m_init_param.m_send_cmd_type_to_cgi_info_map.end(); ++it)
	{
		if (it->second.m_recv_cmd_type == recv_cmd_type)
			return &it->second;
	}
	return nullptr;
}



SCLIENT_NAMESPACE_END
