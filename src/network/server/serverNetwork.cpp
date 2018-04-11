#include "serverNetwork.h"
SSVR_NAMESPACE_BEGIN



bool operator < (const session_id_t & l, const session_id_t & r)
{
	for (size_t i = 0; i < 16; ++i)
	{
		if (l.m_data[i] < r.m_data[i])
			return true;
		else if (l.m_data[i] > r.m_data[i])
			return false;
	}
	return false;
}

bool operator == (const session_id_t & l, const session_id_t & r)
{
	for (size_t i = 0; i < 16; ++i)
	{
		if (l.m_data[i] != r.m_data[i])
			return false;
	}
	return true;
}

bool operator != (const session_id_t& l, const session_id_t& r)
{
	return !(l == r);
}


uint64_t ServerNetwork::s_send_pack_id_seed = 0;



// interface --------------------------------------------------------------
ServerNetwork::ServerNetwork()
{
	slog_d("new ServerNetwork=%0", (uint64_t)this);
	m_is_running = false;
	m_listen_sid = 0;
	//m_timer = NULL;
	m_timer_id = 0;
}

ServerNetwork::~ServerNetwork()
{
	slog_d("delete ServerNetwork=%0", (uint64_t)this);
	//ScopeMutex __l(m_mutex);
	__stop();
	if (m_init_param.m_sapi != NULL)
	{
		if (m_listen_sid != 0)
			m_init_param.m_sapi->releaseSvrListenSocket(m_listen_sid);
		m_init_param.m_work_looper->releasseTimer(m_timer_id);
	}
}

bool ServerNetwork::init(const InitParam& init_param)
{
	slog_d("init server network");
	if (init_param.m_callback == NULL || init_param.m_unpacker == NULL || init_param.m_sapi == NULL 
		|| init_param.m_work_looper == NULL || init_param.m_svr_port == 0 || init_param.m_svr_ip_or_name.size() == 0
		|| init_param.m_send_cmd_type_to_cgi_info_map.size() == 0)
	{
		slog_e("init network fail, param is invalid.");
		return false;
	}

	m_init_param = init_param;
	m_timer_id = m_init_param.m_work_looper->createTimer(this);
	return true;
}

bool ServerNetwork::start()
{
	slog_d("start server network");
	//ScopeMutex __l(m_mutex);
	if (m_is_running)
		return true;
	m_init_param.m_work_looper->addMsgHandler(this);
	m_init_param.m_work_looper->addMsgTimerHandler(this);

	ITcpSocketCallbackApi::CreateSvrSocketParam param;
	param.m_callback_looper = m_init_param.m_work_looper;
	param.m_callback_target = this;
	param.m_svr_ip_or_name = m_init_param.m_svr_ip_or_name;
	param.m_svr_port = m_init_param.m_svr_port;
	if (!m_init_param.m_sapi->createSvrListenSocket(&m_listen_sid, param))
	{
		slog_e("fail to createSvrListenSocket");
		return false;
	}
	if (!m_init_param.m_sapi->startSvrListenSocket(m_listen_sid))
	{
		slog_e("fail to startSvrListenSocket");
		return false;
	}
	if (!m_init_param.m_work_looper->startTimer(m_timer_id, 1, 1))
	{
		slog_e("fail to startTimer");
		return false;
	}

	m_is_running = true;
	return true;
}

void ServerNetwork::stop()
{
	slog_d("stop server network");
	//ScopeMutex __l(m_mutex);
	__stop();
}

ServerNetwork::SendPack * ServerNetwork::newSendPack(socket_id_t sid, session_id_t ssid, uint32_t send_cmd_type, uint32_t send_seq)
{
	ServerCgiInfo* cgi_info = __getServerCgiInofoBySendCmdType(send_cmd_type);
	if (cgi_info == NULL)
		return nullptr;

	SendPack* send_pack = new SendPack();
	send_pack->m_send_pack_id = ++s_send_pack_id_seed;
	send_pack->m_send_cmd_type = send_cmd_type;
	send_pack->m_send_seq = send_seq;
	send_pack->m_sid = sid;
	send_pack->m_ssid = ssid;

	if (cgi_info->m_cgi_type == EServerCgiType_c2sReq_s2cResp)
	{
		// user sholud set send_seq
	}
	else if (cgi_info->m_cgi_type == EServerCgiType_s2cReq_c2sResp)
	{
		//ScopeMutex l(m_mutex);
		__Client* client = __getClientBySid(sid);
		if (client == nullptr)
			return nullptr;
		send_pack->m_send_seq = ++client->m_send_seq_seed;
	}
	else if (cgi_info->m_cgi_type == EServerCgiType_s2cPush)
	{
		send_pack->m_send_seq = 0;
	}
	else
	{
		delete send_pack;
		return nullptr;
	}

	return send_pack;
}

bool ServerNetwork::sendPackToClient(const SendPack& send_pack)
{
	//ScopeMutex __l(m_mutex);
	if (!m_is_running)
	{
		slog_e("fail to send pack, !m_is_running! %0", send_pack.toOverviewString());
		return false;
	}
	if (send_pack.m_send_whole_pack_bin.getLen() == 0)
	{
		slog_e("fail to send pack, len == 0! %0", send_pack.toOverviewString());
		return false;
	}

	ServerCgiInfo* cgi_info = __getServerCgiInofoBySendCmdType(send_pack.m_send_cmd_type);
	if (cgi_info == nullptr)
	{
		slog_e("fail to send pack, cgi_info == null! %0", send_pack.toOverviewString());
		return false;
	}
	if ((cgi_info->m_cgi_type == EServerCgiType_c2sReq_s2cResp && send_pack.m_send_seq == 0)
		|| (cgi_info->m_cgi_type == EServerCgiType_s2cPush && send_pack.m_send_seq != 0)
		|| (cgi_info->m_cgi_type == EServerCgiType_s2cReq_c2sResp && send_pack.m_send_seq == 0))
	{
		slog_e("fail to send pack, invalid send_seq! %0", send_pack.toOverviewString());
		return false;
	}

	__Client* c = __getClientBySid(send_pack.m_sid);
	if (c == NULL)
	{
		slog_e("fail to send pack, __Client == null! %0", send_pack.toOverviewString());
		return false;
	}

	slog_d("send pack to client, %0", send_pack.toOverviewString());
	__SendPackInfo* spi = new __SendPackInfo();
	spi->m_pack = send_pack;
	spi->m_send_pack_start_time_ms = TimeUtil::getMsTime();
	spi->m_cgi_info = cgi_info;
	c->m_send_pack_infos.push_back(spi);

	__postSendPackMsgToSelf();
	return true;
}

void ServerNetwork::cancelSendPackToClient(socket_id_t client_sid, uint64_t send_pack_id)
{
	//ScopeMutex __l(m_mutex);
	slog_d("cancel send pack to client, sid=%0, send_pack_id=%1", client_sid, send_pack_id);
	if (!m_is_running)
		return;
	__Client* c = __getClientBySid(client_sid);
	if (c == NULL)
		return;

	int index = c->getSpiIndexBySendPackId(send_pack_id);
	if (index < 0)
		return;

	__SendPackInfo* spi = c->m_send_pack_infos[index];
	if (c->m_sending_index == index)
	{
		spi->m_is_cancel = true;
	}
	else
	{
		delete_and_erase_vector_element_by_index(&c->m_send_pack_infos, index);
	}
}

void ServerNetwork::disconnectClient(socket_id_t client_sid)
{
	slog_d("disconnectClient, sid=%0", client_sid);
	//ScopeMutex __l(m_mutex);
	if (!m_is_running)
		return;

	__stopClient(client_sid);
}



// message handler ------------------------------------------------------
void ServerNetwork::onMessage(Message * msg, bool* isHandled)
{
	if (msg->m_target != this)
		return;
	//ScopeMutex __l(m_mutex);
	if (!m_is_running)
		return;

	if (msg->m_sender == m_init_param.m_sapi)
	{
		switch (msg->m_msg_type)
		{
		case ITcpSocketCallbackApi::EMsgType_svrListenSocketStarted:
			__onSvrStartedMsg((ITcpSocketCallbackApi::SvrListenSocketStartedMsg*)msg);
			break;
		case ITcpSocketCallbackApi::EMsgType_svrListenSocketStopped:
			__onSvrStoppedMsg((ITcpSocketCallbackApi::SvrListenSocketStoppedMsg*)msg);
			break;
		case ITcpSocketCallbackApi::EMsgType_svrListenSocketAccepted:
			__onSvrAcceptMsg((ITcpSocketCallbackApi::SvrListenSocketAcceptedMsg*)msg);
			break;
		case ITcpSocketCallbackApi::EMsgType_svrTranSocketStopped:
			__onSvrTranStoppedMsg((ITcpSocketCallbackApi::SvrTranSocketStoppedMsg*)msg);
			break;
		case ITcpSocketCallbackApi::EMsgType_svrTranSocketRecvData:
			__onSvrTranRecvDataMsg((ITcpSocketCallbackApi::SvrTranSocketRecvDataMsg*)msg);
			break;
		case ITcpSocketCallbackApi::EMsgType_svrTranSocketSendDataEnd:
			__onSvrTranSendDataEndMsg((ITcpSocketCallbackApi::SvrTranSocketSendDataEndMsg*)msg);
			break;
		}
	}
	else if (msg->m_sender == this)
	{
		switch (msg->m_msg_type)
		{
		case __EMsgType_sendPack:
			__onSendPackMsg();
			break;
		}
	}
}

void ServerNetwork::onMessageTimerTick(uint64_t timer_id, void* user_data)
{
	if (timer_id != m_timer_id)
		return;
	//ScopeMutex __l(m_mutex);
}

void ServerNetwork::__onSvrStartedMsg(ITcpSocketCallbackApi::SvrListenSocketStartedMsg * msg)
{
	slog_v("__onSvrStartedMsg");
}

void ServerNetwork::__onSvrStoppedMsg(ITcpSocketCallbackApi::SvrListenSocketStoppedMsg * msg)
{
	slog_v("__onSvrStoppedMsg");
	__stopAllClients();
}

void ServerNetwork::__onSvrAcceptMsg(ITcpSocketCallbackApi::SvrListenSocketAcceptedMsg * msg)
{
	slog_v("__onSvrAcceptMsg");
	socket_id_t listen_sid = msg->m_svr_listen_sid;
	socket_id_t tran_sid = msg->m_svr_trans_sid;
	if (listen_sid != m_listen_sid)
		return;

	__Client* client = new __Client();
	client->m_sid = tran_sid;
	m_sid_to_client_map[client->m_sid] = client;
	__postClientConnectedMsgToTarget(tran_sid);
}

void ServerNetwork::__onSvrTranStoppedMsg(ITcpSocketCallbackApi::SvrTranSocketStoppedMsg* msg)
{
	slog_v("__onSvrTranStoppedMsg");
	socket_id_t listen_sid = msg->m_svr_listen_sid;
	socket_id_t tran_sid = msg->m_svr_trans_sid;
	if (listen_sid != m_listen_sid)
		return;
	__Client* client = __getClientBySid(tran_sid);
	if (client == NULL)
		return;

	__postClientDisconnectedMsgToTarget(client->m_sid);
	delete_and_erase_map_element_by_key(&m_sid_to_client_map, tran_sid);
}

void ServerNetwork::__onSvrTranRecvDataMsg(ITcpSocketCallbackApi::SvrTranSocketRecvDataMsg * msg)
{
	slog_v("__onSvrTranRecvDataMsg, len=%0", msg->m_data.getLen());
	socket_id_t listen_sid = msg->m_svr_listen_sid;
	socket_id_t tran_sid = msg->m_svr_trans_sid;
	if (listen_sid != m_listen_sid)
		return;
	__Client* client = __getClientBySid(tran_sid);
	if (client == NULL)
		return;

	client->m_recv_data.append(msg->m_data);
	while (true)
	{
		UnpackResult r;
		m_init_param.m_unpacker->unpackServerRecvPack(client->m_recv_data.getData(), client->m_recv_data.getLen(), &r);
		if (r.m_result_type == EUnpackResultType_fail)
		{
			slog_e("EUnpackResultType_fail");
			__postClientDisconnectedMsgToTarget(client->m_sid);
			__stopClient(client->m_sid);
			break;
		}
		else if (r.m_result_type == EUnpackResultType_ok)
		{
			client->m_recv_data.shrinkBegin(r.m_unpack_raw_data_len);

			RecvPack* recv_pack = new RecvPack();
			recv_pack->m_recv_cmd_type = r.m_recv_cmd_type;
			recv_pack->m_recv_seq = r.m_recv_seq;
			recv_pack->m_recv_ext = r.m_recv_ext;
			recv_pack->m_sid = client->m_sid;
			recv_pack->m_ssid = r.m_ssid;

			int sent_pack_index = client->getSpiIndexBySeq(recv_pack->m_recv_seq);
			if (sent_pack_index >= 0)
			{
				__SendPackInfo* spi = client->m_send_pack_infos[sent_pack_index];
				spi->m_pack.m_send_whole_pack_bin.clear();

				//SendPackEndMsg* m = new SendPackEndMsg();
				//m->m_send_pack_id = send_pack->m_pack.m_send_pack_id;
				//m->m_recv_pack = recv_pack;
				//__postMessageToTarget(m);
				m_init_param.m_callback->onServerNetwork_sendPackToClientEnd(this, client->m_sid, EErrCode_ok, spi->m_pack.m_send_pack_id, recv_pack);

				delete_and_erase_vector_element_by_index(&client->m_send_pack_infos, sent_pack_index);
			}
			else
			{
				//RecvPackMsg* m = new RecvPackMsg();
				//m->m_recv_pack = recv_pack;
				//__postMessageToTarget(m);
				m_init_param.m_callback->onServerNetwork_recvPackFromClient(this, client->m_sid, recv_pack);
			}

			slog_v("recv pack, sid=%0, cmd_type=%1, seq=%2\n", client->m_sid, recv_pack->m_recv_cmd_type, recv_pack->m_recv_seq);

			if (client->m_recv_data.getLen() == 0)
				break;
		}
		else if(r.m_result_type == EUnpackResultType_needMoreData)
		{
			break;
		}
		else
		{
			slog_e("unpack err");
		}
	}
}

void ServerNetwork::__onSvrTranSendDataEndMsg(ITcpSocketCallbackApi::SvrTranSocketSendDataEndMsg * msg)
{
	slog_v("__onSvrTranSendDataEndMsg");
	socket_id_t listen_sid = msg->m_svr_listen_sid;
	socket_id_t tran_sid = msg->m_svr_trans_sid;
	if (listen_sid != m_listen_sid)
		return;
	__Client* client = __getClientBySid(tran_sid);
	if (client == NULL)
		return;
	if (client->m_sending_index < 0)
	{
		//slog_e("ServerNetwork::__onSvrTranSendDataEndMsg client->m_sending_index < 0 err\n");
		__doSendPacks();
		return;
	}

	int sending_index = client->m_sending_index;
	client->m_sending_index = -1;
	__SendPackInfo* spi = client->m_send_pack_infos[sending_index];
	if (spi->m_cgi_info->m_cgi_type != EServerCgiType_s2cReq_c2sResp)
	{
		if (!spi->m_is_cancel)
		{
			slog_d("send pack to client end, sid=%0, pack id=%1", spi->m_pack.m_sid, spi->m_pack.m_send_pack_id);
			m_init_param.m_callback->onServerNetwork_sendPackToClientEnd(this, client->m_sid, EErrCode_ok, spi->m_pack.m_send_pack_id, nullptr);
		}

		delete_and_erase_vector_element_by_index(&client->m_send_pack_infos, sending_index);
	}
	__doSendPacks();
}

void ServerNetwork::__onSendPackMsg()
{
	__doSendPacks();
}




// private function ----------------------------------------------------------

void ServerNetwork::__stop()
{
	if (!m_is_running)
		return;
	m_init_param.m_work_looper->removeMsgHandler(this);
	m_init_param.m_work_looper->removeMsgTimerHandler(this);
	m_init_param.m_work_looper->stopTimer(m_timer_id);
	__stopAllClients();
	m_is_running = false;
}

void ServerNetwork::__stopClient(socket_id_t client_sid)
{
	SidToClientMap::iterator it = m_sid_to_client_map.find(client_sid);
	if (it == m_sid_to_client_map.end())
		return;
	m_init_param.m_sapi->stopSvrTranSocket(client_sid);
	delete it->second;
	m_sid_to_client_map.erase(it);
}

void ServerNetwork::__stopAllClients()
{
	for (SidToClientMap::iterator it = m_sid_to_client_map.begin(); it != m_sid_to_client_map.end(); ++it)
	{
		m_init_param.m_sapi->stopSvrTranSocket(it->second->m_sid);
		delete it->second;
	}
	m_sid_to_client_map.clear();
}

void ServerNetwork::__checkTimeOutSendPacks()
{
#define SVR_SEND_PACK_TIME_OUT_MS 30 * 1000
	uint64_t cur_time = TimeUtil::getMsTime();
	for (SidToClientMap::iterator it = m_sid_to_client_map.begin(); it != m_sid_to_client_map.end(); ++it)
	{
		__Client* c = it->second;

		for (size_t i = 0; i < c->m_send_pack_infos.size(); ++i)
		{
			__SendPackInfo* spi = c->m_send_pack_infos[i];
			if (cur_time < spi->m_send_pack_start_time_ms + SVR_SEND_PACK_TIME_OUT_MS)
				continue;

			m_init_param.m_callback->onServerNetwork_sendPackToClientEnd(this, c->m_sid, EErrCode_system, spi->m_pack.m_send_pack_id, NULL);

			delete_and_erase_vector_element_by_index(&c->m_send_pack_infos, (int)i);
			--i;
		}
	}
}

void ServerNetwork::__doSendPacks()
{
	std::vector<socket_id_t> err_sids;
	for (SidToClientMap::iterator it = m_sid_to_client_map.begin(); it != m_sid_to_client_map.end(); ++it)
	{
		if (!__doSendClientPacks(it->second))
		{
			err_sids.push_back(it->first);
		}
	}

	for (size_t i = 0; i < err_sids.size(); ++i)
	{
		__stopClient(err_sids[i]);
	}
}

bool ServerNetwork::__doSendClientPacks(__Client* c)
{
	if (c->m_sending_index >= 0)
		return true;
	if (c->m_send_pack_infos.size() == 0)
		return true;

	for (size_t i = 0; i < c->m_send_pack_infos.size(); ++i)
	{
		if (c->m_send_pack_infos[i]->m_is_sent)
			continue;

		c->m_sending_index = (int)i;
		__SendPackInfo* spi = c->m_send_pack_infos[i];
		spi->m_send_pack_start_time_ms = TimeUtil::getMsTime();

		if (!m_init_param.m_sapi->sendDataFromSvrTranSocketToClient(c->m_sid, spi->m_pack.m_send_whole_pack_bin.getData(), spi->m_pack.m_send_whole_pack_bin.getLen()))
			return false;
		break;
	}

	return true;
}

void ServerNetwork::__postSendPackMsgToSelf()
{
	Message* msg = new Message();
	msg->m_sender = this;
	msg->m_target = this;
	msg->m_msg_type = __EMsgType_sendPack;
	m_init_param.m_work_looper->postMessage(msg);
}

void ServerNetwork::__postClientConnectedMsgToTarget(socket_id_t client_sid)
{
	m_init_param.m_callback->onServerNetwork_clientConnected(this, client_sid);
	//ClientConnectedMsg * m = new ClientConnectedMsg();
	//m->m_client_sid = client_sid;
	//__postMessageToTarget(m);
}

void ServerNetwork::__postClientDisconnectedMsgToTarget(socket_id_t client_sid)
{
	m_init_param.m_callback->onServerNetwork_clientDisconnected(this, client_sid);
	//ClientDisconnectedMsg * m = new ClientDisconnectedMsg();
	//m->m_client_sid = client_sid;
	//__postMessageToTarget(m);
}

ServerNetwork::__Client* ServerNetwork::__getClientBySid(socket_id_t sid)
{
	SidToClientMap::iterator it = m_sid_to_client_map.find(sid);
	if (it == m_sid_to_client_map.end())
		return NULL;
	return it->second;
}

int ServerNetwork::__Client::getSpiIndexBySendPackId(uint64_t send_pack_id)
{
	for (size_t i = 0; i < m_send_pack_infos.size(); ++i)
	{
		if (m_send_pack_infos[i]->m_pack.m_send_pack_id == send_pack_id)
			return (int)i;
	}
	return -1;
}

int ServerNetwork::__Client::getSpiIndexBySeq(uint32_t seq)
{
	for (size_t i = 0; i < m_send_pack_infos.size(); ++i)
	{
		if (m_send_pack_infos[i]->m_pack.m_send_seq == seq)
			return (int)i;
	}
	return -1;
}




SSVR_NAMESPACE_END
