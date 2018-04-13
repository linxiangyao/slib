#include "clientNetwork.h"
#include "../../util/timeUtil.h"
SCLIENT_NAMESPACE_BEGIN


// interface --------------------------------------------------------------
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
}

ClientNetwork::~ClientNetwork()
{
	slog_d("delete ClientNetwork=%0", (uint64_t)this);
	stop();
	m_init_param.m_sapi->releaseClientSocket(m_client_ctx.m_sid);
	m_init_param.m_work_looper->releasseTimer(m_timer_id);
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

	{
		if (m_speed_tester != NULL)
		{
			slog_e("ClientNetwork::start fail, m_speed_tester != NULL");
			return false;
		}
		m_speed_tester = new ClientNetSpeedTester();
		std::vector<ClientNetSpeedTester::SvrInfo> svr_infos;
		for (size_t i = 0; i < m_init_param.m_svr_infos.size(); ++i)
		{
			ClientNetSpeedTester::SvrInfo svr_info;
			svr_info.m_svr_ip_or_name = m_init_param.m_svr_infos[i].m_svr_ip_or_name;
			svr_info.m_svr_port = m_init_param.m_svr_infos[i].m_svr_port;
			svr_info.m_send_count = 0;
			svr_infos.push_back(svr_info);
		}
		ClientNetSpeedTester::InitParam p;
		p.m_notify_looper = m_init_param.m_work_looper;
		p.m_notify_target = this;
		p.m_work_looper = m_init_param.m_work_looper;
		p.m_sapi = m_init_param.m_sapi;
		p.m_svr_infos = svr_infos;
		if (!m_speed_tester->init(p))
		{
			slog_e("ClientNetwork::start fail to m_speed_tester->init");
			return false;
		}

		if (!__doTestSvrSpeed())
		{
			slog_e("ClientNetwork::start fail to __doTestSvrSpeed");
			return false;
		}
	}
	
	if (!m_init_param.m_work_looper->startTimer(m_timer_id, 1, 1))
	{
		slog_e("ClientNetwork::start fail to startTimer");
		return false;
	}
	
	m_init_param.m_callback->onClientNetworkStatred(this);
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
	
	m_speed_test_results.clear();

	m_client_ctx.resetConnectState();
	delete_and_erase_collection_elements(&m_send_pack_infos);
	delete_and_erase_collection_elements(&m_wait_resp_pack_infos);

	m_init_param.m_callback->onClientNetworkStopped(this);
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

bool ClientNetwork::sendPack(const SendPack& send_pack)
{
	if (!m_is_running)
	{
		slog_e("ClientNetwork::sendPack fail, !m_is_running! %0", send_pack.toOverviewString());
		return false;
	}
	if (send_pack.m_send_whole_pack_bin.getLen() == 0)
	{
		slog_e("ClientNetwork::sendPack fail, len == 0! %0", send_pack.toOverviewString());
		return false;
	}

	ClientCgiInfo* cgi_info = __getClientCgiInfoBySendCmdType(send_pack.m_send_cmd_type);
	if (cgi_info == nullptr)
	{
		slog_e("ClientNetwork::sendPack fail, cgi_info == nullptr! %0", send_pack.toOverviewString());
		return false;
	}
	if ((cgi_info->m_cgi_type == EClientCgiType_c2sReq_s2cResp && send_pack.m_send_seq == 0)
		|| (cgi_info->m_cgi_type == EClientCgiType_c2sNotify && send_pack.m_send_seq != 0)
		|| (cgi_info->m_cgi_type == EClientCgiType_c2sReq_s2cResp && send_pack.m_send_seq == 0))
	{
		slog_e("ClientNetwork::sendPack fail, invalid send_seq! %0", send_pack.toOverviewString());
		return false;
	}

	size_t pack_count = __getSendPackCountBySendPackCmdType(send_pack.m_send_cmd_type);
	if (pack_count >= cgi_info->m_max_pack_count_in_queue)
	{
		slog_e("ClientNetwork::sendPack fail, too many send pack in queue in network! %0", send_pack.toOverviewString());
		return false;
	}


	slog_d("sendPack, %0", send_pack.toOverviewString());
	__SendPackInfo* spi = new __SendPackInfo();
	spi->m_pack = send_pack;
	spi->m_create_time = TimeUtil::getMsTime();
	spi->m_cgi_info = cgi_info;
	m_send_pack_infos.push_back(spi);

	__postSendPackMsgToSelf();
	return true;
}

void ClientNetwork::cancelSendPack(uint64_t pack_id)
{
	if (!m_is_running)
		return;

	int index = __getSpiIndexBySendPackId(pack_id);
	if (index >= 0) // in send queue
	{
		slog_d("cancelSendPack");
		if (m_client_ctx.m_sending_pack_index == index) // sending
		{
			m_send_pack_infos[index]->m_is_cancel = true;
		}
		else // not sending
		{
			delete_and_erase_vector_element_by_index(&m_send_pack_infos, index);
		}

		return;
	}

	index = __getWpiIndexBySendPackId(pack_id);
	if (index >= 0) // in wait resp queue
	{
		slog_d("cancelSendPack");
		delete_and_erase_vector_element_by_index(&m_wait_resp_pack_infos, index);
	}
}










// message handler --------------------------------------------------------------
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
		case ClientNetSpeedTester::EMsgType_onOneSvrResultUpdate: __onMsgNetSpeedTestResultUpdate(msg); break;
		case ClientNetSpeedTester::EMsgType_onTestEnd: __onMsgNetSpeedTestEnd(msg); break;
		}
		return;
	}
	else if (msg->m_sender == this)
	{
		switch (msg->m_msg_type)
		{
		case __EMsgType_sendPack: __onMsgSendPack(msg); break;
		case __EmsgType_onNetworkStateChanged: __onMsgNetworkStatusChanged(msg); break;
		case __EmsgType_onForgroundChanged: __onMsgForgroundChanged(msg);break;
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
	__checkTimeOutPacks();
}

void ClientNetwork::__onMsgSendPack(Message * msg)
{
	__doSendPack();
}

void ClientNetwork::__onMsgNetworkStatusChanged(Message * msg)
{
	// m_device_network_type = ... 
}

void ClientNetwork::__onMsgForgroundChanged(Message * msg)
{
}

void ClientNetwork::__onMsgTcpSocketClientConnected(ITcpSocketCallbackApi::ClientSocketConnectedMsg * msg)
{
	slog_d("connected, svr_ip=%0, svr_port=%1", m_client_ctx.m_svr_ip.c_str(), m_client_ctx.m_svr_port);
	m_client_ctx.m_connect_state = __EConnectState_connected;
	m_connect_count = 0;

	{
		m_init_param.m_callback->onClientNetworkConnectStateChanged(this, EConnectState_connected);
	}
	__doSendPack();
}

void ClientNetwork::__onMsgTcpSocketClientDisconnected(ITcpSocketCallbackApi::ClientSocketDisconnectedMsg * msg)
{
	slog_d("disconnected, svr_ip=%0, svr_port=%1", m_client_ctx.m_svr_ip.c_str(), m_client_ctx.m_svr_port);
	m_client_ctx.resetConnectState();
	{
		m_init_param.m_callback->onClientNetworkConnectStateChanged(this, EConnectState_disconnected);
	}

	m_send_pack_infos.insert(m_send_pack_infos.begin(), m_wait_resp_pack_infos.begin(), m_wait_resp_pack_infos.end());
	m_wait_resp_pack_infos.clear();

	if (!__doConnectTcpSvr())
	{
		slog_e("fail to __doConnectTcpSvr");
	}
}

void ClientNetwork::__onMsgTcpSocketClientSendDataEnd(ITcpSocketCallbackApi::ClientSocketSendDataEndMsg * msg)
{
	int sending_pack_index = m_client_ctx.m_sending_pack_index;
	m_client_ctx.m_sending_pack_index = -1;

	if (sending_pack_index >= 0)
	{
		__SendPackInfo* sending_pack = m_send_pack_infos[sending_pack_index];
		if (sending_pack->m_cgi_info->m_cgi_type == EClientCgiType_c2sReq_s2cResp) // req pack : need to wait resp pack
		{
			m_wait_resp_pack_infos.push_back(sending_pack);
			m_send_pack_infos.erase(m_send_pack_infos.begin() + sending_pack_index);
		}
		else // notify or resp pack : already done, notify message
		{
			m_init_param.m_callback->onClientNetworkSendPackEnd(this, EErrType_ok, 0, sending_pack->m_pack.m_send_pack_id, NULL);

			delete_and_erase_vector_element_by_index(&m_send_pack_infos, sending_pack_index);
		}
	}

	__doSendPack();
}

void ClientNetwork::__onMsgTcpSocketClientRecvData(ITcpSocketCallbackApi::ClientSocketRecvDataMsg * msg)
{
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
				slog_e("fail to get cgi_info, recv_pack.recv_cmd_type=%0, ignore", r.m_recv_cmd_type);
				continue;
			}

			RecvPack* recv_pack = new RecvPack();
			recv_pack->m_recv_cmd_type = r.m_recv_cmd_type;
			recv_pack->m_recv_seq = r.m_recv_seq;
			recv_pack->m_recv_ext = r.m_recv_ext;
			std::unique_ptr<RecvPack> recv_pack_ap(recv_pack);

			if (cgi_info->m_cgi_type == EClientCgiType_s2cPush || cgi_info->m_cgi_type == EClientCgiType_s2cReq_cs2Resp)
			{
				m_init_param.m_callback->onClientNetworkRecvPack(this, &recv_pack_ap);
			}
			else if (cgi_info->m_cgi_type == EClientCgiType_c2sNotify)
			{
				m_init_param.m_callback->onClientNetworkSendPackEnd(this, EErrType_ok, 0, 0, &recv_pack_ap);
			}
			else // c2sReq_s2cResp
			{
				int wait_resp_pack_index = __getWpiIndexByRecvPackCmdTypeAndSeq(recv_pack->m_recv_cmd_type, recv_pack->m_recv_seq);
				if (wait_resp_pack_index >= 0)
				{
					__WaitRespPackInfo* wait_resp_pack = m_wait_resp_pack_infos[wait_resp_pack_index];
					if (!wait_resp_pack->m_is_cancel)
					{
						wait_resp_pack->m_pack.m_send_whole_pack_bin.clear();

						m_init_param.m_callback->onClientNetworkSendPackEnd(this, EErrType_ok, 0, wait_resp_pack->m_pack.m_send_pack_id, &recv_pack_ap);
					}

					m_wait_resp_pack_infos.erase(m_wait_resp_pack_infos.begin() + wait_resp_pack_index);
					delete wait_resp_pack;
				}
				else
				{
					slog_w("unkonw err, recv a s2cResp pack, but can't find c2sReq pack. ignore.");
				}
			}

			if (m_client_ctx.m_recv_data.getLen() == 0)
				break;
		}
		else if (r.m_result_type == EUnpackResultType_needMoreData)
		{
			break;
		}
		else
		{
			slog_e("fail to unpack. try disconnect");
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
	ClientNetSpeedTester::TestResult r;
	ClientNetSpeedTester::parseTestResultFromMsg(&r, msg);
	m_speed_test_results[r.m_svr_ip_or_name + StringUtil::toString(r.m_svr_port)] = r;

	int index = __getSvrInfoIndexBySvrIpAndPort(r.m_svr_ip_or_name, r.m_svr_port);
	if (index < 0)
		return;
	SvrInfo* svr_info = &(m_init_param.m_svr_infos[index]);
	if (m_client_ctx.m_sid != 0)
		return;

	if (!r.m_is_connected)
		return;

	m_speed_tester->stop();

	// init client and connect
	m_connect_count = 0;
	ITcpSocketCallbackApi::CreateClientSocketParam param;
	param.m_callback_looper = m_init_param.m_work_looper;
	param.m_callback_target = this;
	param.m_svr_ip_or_name = svr_info->m_svr_ip_or_name;
	param.m_svr_port = svr_info->m_svr_port;
	if (!m_init_param.m_sapi->createClientSocket(&m_client_ctx.m_sid, param))
		return;
	m_client_ctx.m_svr_ip = svr_info->m_svr_ip_or_name;
	m_client_ctx.m_svr_port = svr_info->m_svr_port;
	__doConnectTcpSvr();
}









// helper function --------------------------------------------------------------
bool ClientNetwork::__doSendPack()
{
	__checkTimeOutPacks();
	if (!__doSendTcpPack())
		return false;
	return true;
}

bool ClientNetwork::__doSendTcpPack()
{
	// need connect
	if (m_client_ctx.m_connect_state == __EConnectState_disconnected)
		return __doConnectTcpSvr();

	if (m_client_ctx.m_connect_state != __EConnectState_connected)
		return true;

	if (m_client_ctx.m_sending_pack_index >= 0)
		return true;

	// get max priory pack and send
	int send_pack_index = __getMaxPriorySpiIndex();
	if (send_pack_index < 0)
		return true;
	__SendPackInfo* p = m_send_pack_infos[send_pack_index];

	if (!m_init_param.m_sapi->sendDataFromClientSocketToSvr(m_client_ctx.m_sid, p->m_pack.m_send_whole_pack_bin.getData(), p->m_pack.m_send_whole_pack_bin.getLen()))
	{
		slog_w("fail to sendDataFromClientSocketToSvr, maybe is disconnected, will retry");
		return false;
	}

	slog_d("send pack ok, cmd_type=%0, pack_data_len=%1, pack_len=%2", p->m_pack.m_send_cmd_type, p->m_pack.m_send_whole_pack_bin.getLen(), p->m_pack.m_send_whole_pack_bin.getLen());
	m_client_ctx.m_sending_pack_index = send_pack_index;
	return true;
}

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
	m_init_param.m_callback->onClientNetworkConnectStateChanged(this, EConnectState_connecting);
	return true;
}

bool ClientNetwork::__doTestSvrSpeed()
{
	if (!m_speed_tester->start())
	{
		slog_e("fail to m_speed_tester->start");
		return false;
	}
	m_is_testing_speed = true;
	return true;
}

void ClientNetwork::__checkTimeOutPacks()
{
	uint64_t cur_time_ms = TimeUtil::getMsTime();
	{
		std::vector<size_t> remove_indexs;
		for (size_t i = 0; i < m_send_pack_infos.size(); ++i)
		{
			__SendPackInfo* p = m_send_pack_infos[i];
			bool is_time_out = cur_time_ms - p->m_create_time >= p->m_cgi_info->m_cgi_time_out_ms;
			if (is_time_out)   
			{
				__postSendPackTimeOutMsg(p);
				remove_indexs.push_back(i);
			}
		}

		for (int i = (int)remove_indexs.size() - 1; i >= 0; --i)
		{
			__deleteAndEraseSpiByIndex(remove_indexs[i]);
		}
	}

	{
		std::vector<size_t> remove_indexs;
		for (size_t i = 0; i < m_wait_resp_pack_infos.size(); ++i)
		{
			__SendPackInfo* p = m_wait_resp_pack_infos[i];
			bool is_time_out = cur_time_ms - p->m_create_time >= p->m_cgi_info->m_cgi_time_out_ms;
			if (is_time_out)
			{
				__postSendPackTimeOutMsg(p);
				remove_indexs.push_back(i);
			}
		}

		for (int i = (int)remove_indexs.size() - 1; i >= 0; --i)
		{
			delete_and_erase_vector_element_by_index(&m_wait_resp_pack_infos, (int)remove_indexs[i]);
		}
	}
}

bool ClientNetwork::__isTimeToConnect()
{
	uint64_t connect_interval_ms = __getConnectIntervalMs(m_connect_count);
	uint64_t cur_time_ms = TimeUtil::getMsTime();
	bool is_time_to_connect = cur_time_ms > m_last_reconnect_time_ms + connect_interval_ms;
	return is_time_to_connect;
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

void ClientNetwork::__postSendPackTimeOutMsg(__SendPackInfo* p)
{
	m_init_param.m_callback->onClientNetworkSendPackEnd(this, EErrType_local, ELocalErrCode_sendPackTimeOutErr, p->m_pack.m_send_pack_id, NULL);
}

int ClientNetwork::__getMaxPriorySpiIndex()
{
	if (m_send_pack_infos.size() == 0)
		return -1;
	int max_priory_pack_index = -1;
	int max_priory = 0;
	for (size_t i = 0; i < m_send_pack_infos.size(); ++i)
	{
		__SendPackInfo* pack = m_send_pack_infos[i];
		if (pack->m_cgi_info->m_priority > max_priory)
		{
			max_priory = pack->m_cgi_info->m_priority;
			max_priory_pack_index = (int)i;
		}
	}
	if (max_priory_pack_index < 0)
		return -1;

	return max_priory_pack_index;
}

int ClientNetwork::__getMaxPrioryNoSessionSpiIndex()
{
	if (m_send_pack_infos.size() == 0)
		return -1;

	int max_priory_pack_index = -1;
	int max_priory = 0;
	for (size_t i = 0; i < m_send_pack_infos.size(); ++i)
	{
		__SendPackInfo* pack = m_send_pack_infos[i];
		if (pack->m_cgi_info->m_priority > max_priory)
		{
			max_priory = pack->m_cgi_info->m_priority;
			max_priory_pack_index = (int)i;
		}
	}
	return max_priory_pack_index;
}

int ClientNetwork::__getSpiIndexBySendPackId(uint64_t send_pack_id)
{
	for (size_t i = 0; i < m_send_pack_infos.size(); ++i)
	{
		if (m_send_pack_infos[i]->m_pack.m_send_pack_id == send_pack_id)
			return (int)i;
	}
	return -1;
}

int ClientNetwork::__getWpiIndexByRecvPackCmdTypeAndSeq(uint32_t recv_pack_cmd_type, uint64_t recv_pack_seq)
{
	for (size_t i = 0; i < m_wait_resp_pack_infos.size(); ++i)
	{
		if (m_wait_resp_pack_infos[i]->m_cgi_info->m_recv_cmd_type == recv_pack_cmd_type && m_wait_resp_pack_infos[i]->m_pack.m_send_seq == recv_pack_seq)
			return (int)i;
	}
	return -1;
}

int ClientNetwork::__getWpiIndexBySendPackId(uint64_t send_pack_id)
{
	for (size_t i = 0; i < m_wait_resp_pack_infos.size(); ++i)
	{
		if (m_wait_resp_pack_infos[i]->m_pack.m_send_pack_id == send_pack_id)
			return (int)i;
	}
	return -1;
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

size_t ClientNetwork::__getSendPackCountBySendPackCmdType(uint32_t cmd_type)
{
	size_t count = 0;
	for (size_t i = 0; i < m_send_pack_infos.size(); ++i)
	{
		if (m_send_pack_infos[i]->m_pack.m_send_cmd_type == cmd_type)
		{
			++count;
		}
	}
	for (size_t i = 0; i < m_wait_resp_pack_infos.size(); ++i)
	{
		if (m_wait_resp_pack_infos[i]->m_pack.m_send_cmd_type == cmd_type)
		{
			++count;
		}
	}
	return count;
}

void ClientNetwork::__deleteAndEraseSpiByIndex(size_t index)
{
	if (index == m_client_ctx.m_sending_pack_index)
	{
		m_client_ctx.m_sending_pack_index = -1; // sending pack is timeout
	}
	delete m_send_pack_infos[index];
	m_send_pack_infos.erase(m_send_pack_infos.begin() + index);
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
