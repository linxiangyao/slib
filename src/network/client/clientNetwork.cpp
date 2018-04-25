#include "../../util/timeUtil.h"
#include "clientNetwork.h"
SCLIENT_NAMESPACE_BEGIN

// ClientNetwork ------------------------------------------------------------------------------------------
// interface --
ClientNetwork::ClientNetwork()
{
	slog_d("new ClientNetwork=%0", (uint64_t)this);
	m_is_running = false;
	m_timer_id = 0;

	m_is_repeat_last_connect_interval_ms = true;
	m_last_reconnect_time_ms = 0;
	m_connect_count = 0;

	m_speed_tester = NULL;
	m_is_testing_speed = false;

	m_dns_resolver = nullptr;
}

ClientNetwork::~ClientNetwork()
{
	slog_d("delete ClientNetwork=%0", (uint64_t)this);
	stop();
	m_init_param.m_dns_resolver->removeNotifyLooper(m_init_param.m_work_looper);
	m_init_param.m_sapi->releaseClientSocket(m_client_ctx.m_sid);
	m_init_param.m_work_looper->releasseTimer(m_timer_id);
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

	m_connect_interval_mss = m_init_param.m_connect_interval_mss;
	m_timer_id = m_init_param.m_work_looper->createTimer(NULL);
	m_client_ctx.m_init_param = &m_init_param;
	
	// dns_resolver
	{
		if (m_init_param.m_dns_resolver == nullptr)
		{
			m_dns_resolver = new DnsResolver();
			if (!m_dns_resolver->init(m_init_param.m_work_looper))
				return false;
			m_init_param.m_dns_resolver = m_dns_resolver;
		}
		m_dns_resolver->addNotifyLooper(m_init_param.m_work_looper);
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
	
	m_init_param.m_callback->onClientNetwork_statred(this);
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
	
	m_init_param.m_sapi->stopClientSocket(m_client_ctx.m_sid);
	m_speed_tester->stop();

	m_init_param.m_work_looper->removeMessagesByTarget(this);

	m_init_param.m_work_looper->stopTimer(m_timer_id);


	//m_client_ctx.resetConnectState();
	//delete_and_erase_collection_elements(&m_send_pack_infos);
	//delete_and_erase_collection_elements(&m_wait_resp_pack_infos);

	m_init_param.m_callback->onClientNetwork_stopped(this);
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

	if (m_client_ctx.m_cgi_ctxs.size() >= m_init_param.m_max_pack_count)
	{
		slog_e("ClientNetwork::startCgi fail, too many packs in network");
		return false;
	}

	SendPack& send_pack = *cgi->getSendPack();
	if (send_pack.m_send_whole_pack_bin.getLen() == 0)
	{
		slog_e("ClientNetwork::sendPack fail, len == 0. %0", send_pack.toOverviewString());
		return false;
	}

	if (!m_is_running)
	{
		slog_e("ClientNetwork::startCgi fail, !m_is_running. %0", send_pack.toOverviewString());
		return false;
	}

	int cgi_index = m_client_ctx.getCgiIndexBySendPackId(cgi->getSendPack()->m_send_pack_id);
	if (cgi_index >= 0)
	{
		slog_d("ClientCgiMgr::startCgi cgi already start? ignore");
		return true;
	}

	const ClientCgiInfo* cgi_info = &cgi->getCgiInfo();
	if ((cgi_info->m_cgi_type == EClientCgiType_c2sReq_s2cResp && send_pack.m_send_seq == 0)
		|| (cgi_info->m_cgi_type == EClientCgiType_c2sNotify && send_pack.m_send_seq != 0)
		|| (cgi_info->m_cgi_type == EClientCgiType_c2sReq_s2cResp && send_pack.m_send_seq == 0))
	{
		slog_e("ClientNetwork::startCgi fail, invalid send_seq! %0", send_pack.toOverviewString());
		return false;
	}

	size_t pack_count = m_client_ctx.getCgiCountBySendPackCmdType(send_pack.m_send_cmd_type);
	if (pack_count >= cgi_info->m_max_pack_count_in_queue)
	{
		slog_e("ClientNetwork::startCgi fail, too many send pack in queue in network! %0", send_pack.toOverviewString());
		return false;
	}

	slog_d("sendPack, %0", send_pack.toOverviewString());
	m_client_ctx.addCgi(cgi);

	__postSendPackMsgToSelf();
	return true;
}

void ClientNetwork::stopCgi(ClientCgi * cgi)
{
	slog_v("stop cgi, cgi=%0", (uint64_t)cgi);
	m_client_ctx.cancelCgi(cgi);
}







// message handler --
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
		case __EMsgType_sendPack: __onMsgSendPack(msg); break;
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
	m_client_ctx.checkTimeOutPacks();
}

void ClientNetwork::__onMsgSendPack(Message * msg)
{
	m_client_ctx.doSendPack();
}

void ClientNetwork::__onMsgTcpSocketClientConnected(ITcpSocketCallbackApi::ClientSocketConnectedMsg * msg)
{
	if (msg->m_client_sid != m_client_ctx.m_sid)
		return;
	slog_d("connected, svr_ip=%0, svr_port=%1", m_client_ctx.m_svr_ip.c_str(), m_client_ctx.m_svr_port);

	m_connect_count = 0;
	m_client_ctx.onConnected();

	m_init_param.m_callback->onClientNetwork_connectStateChanged(this, EConnectState_connected);
}

void ClientNetwork::__onMsgTcpSocketClientDisconnected(ITcpSocketCallbackApi::ClientSocketDisconnectedMsg * msg)
{
	if (msg->m_client_sid != m_client_ctx.m_sid)
		return;
	slog_d("disconnected, svr_ip=%0, svr_port=%1", m_client_ctx.m_svr_ip.c_str(), m_client_ctx.m_svr_port);

	m_client_ctx.onDisconnected();

	if (!__doConnectTcpSvr())
	{
		slog_e("fail to __doConnectTcpSvr");
	}

	{
		m_init_param.m_callback->onClientNetwork_connectStateChanged(this, EConnectState_disconnected);
	}
}

void ClientNetwork::__onMsgTcpSocketClientSendDataEnd(ITcpSocketCallbackApi::ClientSocketSendDataEndMsg * msg)
{
	if (msg->m_client_sid != m_client_ctx.m_sid)
		return;
	m_client_ctx.onSendPackEnd();
}

void ClientNetwork::__onMsgTcpSocketClientRecvData(ITcpSocketCallbackApi::ClientSocketRecvDataMsg * msg)
{
	if (msg->m_client_sid != m_client_ctx.m_sid)
		return;

	slog_d("recv data len=%0", msg->m_recv_data.getLen());
	m_client_ctx.m_recv_data.append(msg->m_recv_data);

	while (true)
	{
		if (!m_is_running)
			break;

		UnpackResult r;
		m_init_param.m_unpacker->unpackClientRecvPack(m_client_ctx.m_recv_data.getData(), m_client_ctx.m_recv_data.getLen(), &r);
		if (r.m_result_type == EUnpackResultType_ok)
		{
			slog_d("unpack ok");
			m_client_ctx.m_recv_data.shrinkBegin(r.m_unpack_raw_data_len);

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
			m_client_ctx.onRecvPack(recv_pack); // ignore err.

			if (m_client_ctx.m_recv_data.getLen() == 0)
				break;
		}
		else if (r.m_result_type == EUnpackResultType_needMoreData)
		{
			break;
		}
		else // fatal error
		{
			slog_e("ClientNetwork::__onMsgTcpSocketClientRecvData fail to unpack. try disconnect");
			m_init_param.m_sapi->stopClientSocket(m_client_ctx.m_sid);
			break;
		}
	}
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
	if (m_client_ctx.m_sid != 0)
		return;

	if (!m->m_is_connected)
		return;

	m_speed_tester->stop();

	// init client and connect
	m_connect_count = 0;
	ITcpSocketCallbackApi::CreateClientSocketParam param;
	param.m_callback_looper = m_init_param.m_work_looper;
	param.m_callback_target = this;
	param.m_svr_ip = svr_info->m_svr_ip_or_name;
	param.m_svr_port = svr_info->m_svr_port;
	if (!m_init_param.m_sapi->createClientSocket(&m_client_ctx.m_sid, param))
		return;
	m_client_ctx.m_svr_ip = svr_info->m_svr_ip_or_name;
	m_client_ctx.m_svr_port = svr_info->m_svr_port;
	__doConnectTcpSvr();
}









// helper function --
bool ClientNetwork::__doConnectTcpSvr()
{
	if (m_client_ctx.m_connect_state == __EConnectState_connected || m_client_ctx.m_connect_state == __EConnectState_connecting)
		return true;

	if (!__isTimeToConnect())
		return true;

	slog_v("doConnectTcpSvr, connect_cout=%0", m_connect_count);
	m_last_reconnect_time_ms = TimeUtil::getMsTime();
	++m_connect_count;
	
	// find out the fastest svr
	if (m_client_ctx.m_sid == 0)
	{
		if (m_is_testing_speed)
		{
			slog_v("is testing speed, wait, ignore connect cmd");
			return true;
		}

		return __doTestSvrSpeed();
	}

	// connect fasterst svr
	slog_d("doConnectTcpSvr, svr_ip=%0, svr_port=%1", m_client_ctx.m_svr_ip.c_str(), m_client_ctx.m_svr_port);
	if (!m_init_param.m_sapi->startClientSocket(m_client_ctx.m_sid))
	{
		slog_e("fail to startClientSocket");
		return false;
	}
	m_client_ctx.m_connect_state = __EConnectState_connecting;
	m_init_param.m_callback->onClientNetwork_connectStateChanged(this, EConnectState_connecting);
	return true;
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

bool ClientNetwork::__isTimeToConnect()
{
	uint64_t connect_interval_ms = __getConnectIntervalMs(m_connect_count);
	uint64_t cur_time_ms = TimeUtil::getMsTime();
	bool is_time_to_connect = cur_time_ms > m_last_reconnect_time_ms + connect_interval_ms;
	return is_time_to_connect;
}

uint64_t ClientNetwork::__getConnectIntervalMs(size_t connect_count)
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















// __ClientCtx ----------------------------------------------

ClientNetwork::__ClientCtx::__ClientCtx()
{ 
	m_sid = 0;
	m_connect_state = __EConnectState_disconnected;
	m_sending_cgi_index = -1;
	m_svr_port = 0;
}

ClientNetwork::__ClientCtx::~__ClientCtx()
{
	delete_and_erase_collection_elements(&m_cgi_ctxs);
}

void ClientNetwork::__ClientCtx::addCgi(ClientCgi * cgi)
{
	cgi->setStartMs(TimeUtil::getMsTime());

	__CgiCtx* cgi_ctx = new __CgiCtx();
	cgi_ctx->m_cgi = cgi;
	cgi_ctx->m_create_time = TimeUtil::getMsTime();
	m_cgi_ctxs.push_back(cgi_ctx);
}

void ClientNetwork::__ClientCtx::cancelCgi(ClientCgi * cgi)
{
	int cgi_index = getCgiIndexBySendPackId(cgi->getSendPack()->m_send_pack_id);
	if (cgi_index < 0)
		return;

	if (m_sending_cgi_index == cgi_index)
	{
		m_sending_cgi_index = -1;
	}

	delete_and_erase_vector_element_by_index(&m_cgi_ctxs, cgi_index);
}

bool ClientNetwork::__ClientCtx::doSendPack()
{
	checkTimeOutPacks();

	// need connect
	if (m_connect_state != __EConnectState_connected)
		return true;

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
	m_sending_cgi_index = to_send_cgi_index;
	return true;
}

void ClientNetwork::__ClientCtx::checkTimeOutPacks()
{
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

int ClientNetwork::__ClientCtx::getCgiIndexBySendPackId(uint64_t send_pack_id)
{
	for (size_t i = 0; i < m_cgi_ctxs.size(); ++i)
	{
		if (m_cgi_ctxs[i]->m_cgi->getSendPack()->m_send_pack_id == send_pack_id)
			return (int)i;
	}
	return -1;
}

size_t ClientNetwork::__ClientCtx::getCgiCountBySendPackCmdType(uint32_t send_pack_cmd_type)
{
	return 0;
}

void ClientNetwork::__ClientCtx::onSendPackEnd()
{
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
			slog_e("__ClientCtx::onSendPackEnd invalid path, cgi_type=%0", cgi_ctx->m_cgi->getCgiInfo().m_cgi_type);
		}
	}

	doSendPack();
}

void ClientNetwork::__ClientCtx::onConnected()
{
	m_connect_state = __EConnectState_connected;
	doSendPack();
}

void ClientNetwork::__ClientCtx::onDisconnected()
{
	__resetConnectState();

	//TODO: resend
	for (size_t i = 0; i < m_cgi_ctxs.size(); ++i)
	{
		m_cgi_ctxs[i]->m_is_sent = false;
	}
}

void ClientNetwork::__ClientCtx::onRecvPack(RecvPack * recv_pack)
{
	std::unique_ptr<RecvPack> recv_pack_ap(recv_pack);

	ClientCgiInfo* cgi_info = __getClientCgiInfoByRecvCmdType(recv_pack->m_recv_cmd_type);
	if (cgi_info->m_cgi_type == EClientCgiType_s2cPush)
	{
		m_init_param->m_callback->onClientNetwork_recvS2cPushPack(m_network, &recv_pack_ap);
	}
	else if (cgi_info->m_cgi_type == EClientCgiType_s2cReq_cs2Resp)
	{
		m_init_param->m_callback->onClientNetwork_recvS2cReqPack(m_network, &recv_pack_ap);
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

void ClientNetwork::__ClientCtx::__markCgiDoneByIndex(int index, EErrType err_type, int err_code)
{
	if (index < 0 || index >= m_cgi_ctxs.size())
		return;

	__CgiCtx* cgi_ctx = m_cgi_ctxs[index];
	ClientCgi* cgi = m_cgi_ctxs[index]->m_cgi;
	cgi->setErrType(err_type);
	cgi->setErrCode(err_code);
	cgi->setEndMs(TimeUtil::getMsTime());
	if (cgi->getCgiInfo().m_cgi_type == EClientCgiType_s2cPush)
	{
		slog_e("onClientNetworkSendPackEnd invalid path");
	}

	slog_v("call back cgi=%0", (uint64_t)cgi);
	m_cgi_ctxs.erase(m_cgi_ctxs.begin() + index);
	m_init_param->m_callback->onClientNetwork_cgiDone(m_network, cgi);
	delete cgi_ctx;
}

int ClientNetwork::__ClientCtx::__getMaxPrioryCgiIndex()
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

void ClientNetwork::__ClientCtx::__resetConnectState()
{
	m_connect_state = __EConnectState_disconnected;
	m_recv_data.clear();
	m_sending_cgi_index = -1;
}

ClientCgiInfo* ClientNetwork::__ClientCtx::__getClientCgiInfoByRecvCmdType(uint32_t recv_cmd_type)
{
	for (auto it = m_init_param->m_send_cmd_type_to_cgi_info_map.begin(); it != m_init_param->m_send_cmd_type_to_cgi_info_map.end(); ++it)
	{
		if (it->second.m_recv_cmd_type == recv_cmd_type)
			return &it->second;
	}
	return nullptr;
}

int ClientNetwork::__ClientCtx::__getCgiIndexBySendPackSeq(uint64_t send_pack_seq)
{
	for (size_t i = 0; i < m_cgi_ctxs.size(); ++i)
	{
		__CgiCtx* ctx = m_cgi_ctxs[i];
		if (ctx->m_cgi->getSendPack()->m_send_seq == send_pack_seq)
			return (int)i;
	}
	return -1;
}




SCLIENT_NAMESPACE_END
