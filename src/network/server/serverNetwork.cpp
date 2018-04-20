#include "serverNetwork.h"
SSVR_NAMESPACE_BEGIN



// session_id_t ------------------------------------------------------------------------------------

std::string session_id_t::toString() const
{
	return StringUtil::toString(m_data, 16);
}

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






// ServerCgi ------------------------------------------------------------------------------------

session_id_t ServerNetwork::ServerCgi::getSessionId()
{
	if (m_send_pack != nullptr)
		return m_send_pack->m_ssid;
	else if (m_recv_pack != nullptr)
		return m_recv_pack->m_ssid;
	else
		return session_id_t();
}

void ServerNetwork::ServerCgi::__setSendPack(SendPack * send_pack)
{
	m_send_pack = send_pack;
	if (send_pack == nullptr)
		return;
	
	//send_pack->m_send_cmd_type = getServerCgiInfo().m_send_cmd_type;

	//if (m_recv_pack == nullptr)
	//	return;

	//if (getServerCgiInfo().m_cgi_type == EServerCgiType_c2sReq_s2cResp)
	//{
	//	send_pack->m_send_seq = m_recv_pack->m_recv_seq;
	//	send_pack->m_ssid = m_recv_pack->m_ssid;
	//	send_pack->m_sid = m_recv_pack->m_sid;
	//}
}









// ServerNetwork ------------------------------------------------------------------------------------

uint64_t ServerNetwork::s_send_pack_id_seed = 0;



// interface --
ServerNetwork::ServerNetwork()
{
	slog_d("new ServerNetwork=%0", (uint64_t)this);
	m_is_running = false;
	m_listen_sid = 0;
	m_timer_id = 0;
}

ServerNetwork::~ServerNetwork()
{
	slog_d("delete ServerNetwork=%0", (uint64_t)this);
	__stop();
	if (m_init_param.m_sapi != NULL)
	{
		if (m_listen_sid != 0)
			m_init_param.m_sapi->releaseSvrListenSocket(m_listen_sid);
		m_init_param.m_work_looper->releasseTimer(m_timer_id);
	}
	slog_d("delete ServerNetwork=%0 end", (uint64_t)this);
}

bool ServerNetwork::init(const InitParam& init_param)
{
	slog_d("init server network");
	if (m_init_param.m_callback != nullptr)
	{
		slog_d("ServerNetwork::init m_init_param.m_callback != nullptr, maybe already init? ignore.");
		return true;
	}

	if (init_param.m_callback == NULL || init_param.m_unpacker == NULL || init_param.m_sapi == NULL 
		|| init_param.m_work_looper == NULL || init_param.m_svr_port == 0 || init_param.m_svr_ip_or_name.size() == 0
		|| init_param.m_send_cmd_type_to_cgi_info_map.size() == 0)
	{
		slog_e("ServerNetwork::init fail, param is invalid.");
		return false;
	}

	m_init_param = init_param;
	m_timer_id = m_init_param.m_work_looper->createTimer(this);
	return true;
}

bool ServerNetwork::start()
{
	slog_d("start server network");
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
		slog_e("ServerNetwork::start fail to createSvrListenSocket");
		return false;
	}
	if (!m_init_param.m_sapi->startSvrListenSocket(m_listen_sid))
	{
		slog_e("ServerNetwork::start fail to startSvrListenSocket");
		return false;
	}
	if (!m_init_param.m_work_looper->startTimer(m_timer_id, 1, 1))
	{
		slog_e("ServerNetwork::start fail to startTimer");
		return false;
	}

	m_is_running = true;
	return true;
}

void ServerNetwork::stop()
{
	slog_d("stop server network");
	__stop();
	slog_d("stop server network end");
}

ServerNetwork::SendPack* ServerNetwork::newSendPack(socket_id_t sid, session_id_t ssid, uint32_t send_cmd_type, uint32_t send_seq)
{
	ServerCgiInfo* cgi_info = __getServerCgiInofoBySendCmdType(send_cmd_type);
	if (cgi_info == NULL)
	{
		slog_d("ServerNetwork::newSendPack fail to find cgi info, send_cmd_type=%0", send_cmd_type);
		return nullptr;
	}

	if (cgi_info->m_cgi_type == EServerCgiType_s2cPush && send_seq != 0)
	{
		slog_e("ServerNetwork::newSendPack fail, cgi_type=push, and send_seq != 0, send_seq=%0", send_seq);
		return nullptr;
	}

	SendPack* send_pack = new SendPack();
	send_pack->m_send_pack_id = ++s_send_pack_id_seed;
	send_pack->m_send_cmd_type = send_cmd_type;
	send_pack->m_send_seq = send_seq;
	send_pack->m_sid = sid;
	send_pack->m_ssid = ssid;
	
	return send_pack;
}

bool ServerNetwork::startCgi(ServerCgi * cgi)
{
	// check cgi param
	if (cgi == nullptr)
	{
		slog_e("ServerNetwork::startCgi fail, cgi == null");
		return false;
	}
	if (cgi->getSendPack() == nullptr)
	{
		slog_e("ServerNetwork::startCgi fail, send_pack == null. cgi=%0", (uint64_t)cgi);
		return false;
	}
	SendPack& send_pack = *cgi->getSendPack();
	if (send_pack.m_send_whole_pack_bin.getLen() == 0)
	{
		slog_e("ServerNetwork::startCgi fail, send_pack.len == 0. cgi=%0, %1", (uint64_t)cgi, send_pack.toOverviewString());
		return false;
	}
	
	// check is running
	if (!m_is_running)
	{
		slog_e("ServerNetwork::startCgi fail, !m_is_running. cgi=%0, %1", (uint64_t)cgi, send_pack.toOverviewString());
		return false;
	}

	// check cgi info
	ServerCgiInfo* cgi_info = __getServerCgiInofoBySendCmdType(send_pack.m_send_cmd_type);
	if (cgi_info == nullptr)
	{
		slog_e("ServerNetwork::startCgi fail, cgi_info == null. cgi=%0, %1", (uint64_t)cgi, send_pack.toOverviewString());
		return false;
	}
	if ((cgi_info->m_cgi_type == EServerCgiType_c2sReq_s2cResp && send_pack.m_send_seq == 0)
		|| (cgi_info->m_cgi_type == EServerCgiType_s2cPush && send_pack.m_send_seq != 0)
		|| (cgi_info->m_cgi_type == EServerCgiType_s2cReq_c2sResp && send_pack.m_send_seq == 0))
	{
		slog_e("ServerNetwork::startCgi fail, invalid send_seq! cgi=%0, %1", (uint64_t)cgi, send_pack.toOverviewString());
		return false;
	}

	// check client
	__Client* client = __getClientBySid(send_pack.m_sid);
	if (client == NULL)
	{
		slog_e("ServerNetwork::startCgi fail, __Client == null! cgi=%0, %1", (uint64_t)cgi, send_pack.toOverviewString());
		return false;
	}
	if (client->getCgiIndexBySendPackId(cgi->getSendPack()->m_send_pack_id) >= 0)
	{
		slog_w("ServerNetwork::startCgi cgi already start, ignore. cgi=%0, %1", (uint64_t)cgi, send_pack.toOverviewString());
		return true;
	}

	// start cgi
	slog_d("start cgi, cgi=%0, %1", (uint64_t)cgi, send_pack.toOverviewString());
	client->addCgi(cgi);


	__postSendPackMsgToSelf();

	return true;
}

void ServerNetwork::stopCgi(ServerCgi * cgi)
{
	if (cgi == nullptr || cgi->getSendPack() == nullptr)
		return;

	SendPack& send_pack = *cgi->getSendPack();
	slog_d("stop cgi, cgi=%0, %1", (uint64_t)cgi, send_pack.toOverviewString());
	if (!m_is_running)
		return;

	__Client* client = __getClientBySid(send_pack.m_sid);
	if (client == nullptr)
		return;
	client->stopCgi(cgi);
}

void ServerNetwork::disconnectClient(socket_id_t client_sid)
{
	slog_d("disconnectClient, sid=%0", client_sid);
	if (!m_is_running)
		return;

	__stopClient(client_sid);
}



// message handler --
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

void ServerNetwork::__onSvrStartedMsg(ITcpSocketCallbackApi::SvrListenSocketStartedMsg* msg)
{
	if (msg->m_svr_listen_sid != m_listen_sid)
		return;
	slog_v("__onSvrStartedMsg");
}

void ServerNetwork::__onSvrStoppedMsg(ITcpSocketCallbackApi::SvrListenSocketStoppedMsg * msg)
{
	if (msg->m_svr_listen_sid != m_listen_sid)
		return;
	slog_v("__onSvrStoppedMsg");
	__stopAllClients();
}

void ServerNetwork::__onSvrAcceptMsg(ITcpSocketCallbackApi::SvrListenSocketAcceptedMsg * msg)
{
	socket_id_t listen_sid = msg->m_svr_listen_sid;
	socket_id_t tran_sid = msg->m_svr_trans_sid;
	if (listen_sid != m_listen_sid)
		return;

	slog_v("__onSvrAcceptMsg");
	__Client* client = new __Client(tran_sid, &m_init_param, this);
	m_sid_to_client_map[client->m_sid] = client;
	m_init_param.m_callback->onServerNetwork_clientConnected(this, tran_sid);
}

void ServerNetwork::__onSvrTranStoppedMsg(ITcpSocketCallbackApi::SvrTranSocketStoppedMsg* msg)
{
	socket_id_t listen_sid = msg->m_svr_listen_sid;
	socket_id_t tran_sid = msg->m_svr_trans_sid;
	if (listen_sid != m_listen_sid)
		return;

	slog_v("__onSvrTranStoppedMsg");
	__Client* client = __getClientBySid(tran_sid);
	if (client == nullptr)
		return;

	delete_and_erase_map_element_by_key(&m_sid_to_client_map, tran_sid);

	m_init_param.m_callback->onServerNetwork_clientDisconnected(this, tran_sid);
}

void ServerNetwork::__onSvrTranRecvDataMsg(ITcpSocketCallbackApi::SvrTranSocketRecvDataMsg * msg)
{
	socket_id_t listen_sid = msg->m_svr_listen_sid;
	socket_id_t tran_sid = msg->m_svr_trans_sid;
	if (listen_sid != m_listen_sid)
		return;

	slog_v("__onSvrTranRecvDataMsg, len=%0", msg->m_data.getLen());
	__Client* client = __getClientBySid(tran_sid);
	if (client == nullptr)
		return;

	client->m_recv_data.append(msg->m_data);
	while (true)
	{
		UnpackResult r;
		m_init_param.m_unpacker->unpackServerRecvPack(client->m_recv_data.getData(), client->m_recv_data.getLen(), &r);
		if (r.m_result_type == EUnpackResultType_fail)
		{
			slog_e("ServerNetwork::__onSvrTranRecvDataMsg EUnpackResultType_fail, client_sid=%0", tran_sid);
			__stopClient(client->m_sid);
			m_init_param.m_callback->onServerNetwork_clientDisconnected(this, tran_sid);
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
			slog_d("recv pack = %0", recv_pack->toOverviewString());

			client->onRecvPack(recv_pack);

			if (client->m_recv_data.getLen() == 0)
				break;
		}
		else if(r.m_result_type == EUnpackResultType_needMoreData)
		{
			break;
		}
		else
		{
			slog_e("ServerNetwork::__onSvrTranRecvDataMsg unpack err, client_sid=%0", tran_sid);
		}
	}
}

void ServerNetwork::__onSvrTranSendDataEndMsg(ITcpSocketCallbackApi::SvrTranSocketSendDataEndMsg * msg)
{
	socket_id_t listen_sid = msg->m_svr_listen_sid;
	socket_id_t tran_sid = msg->m_svr_trans_sid;
	if (listen_sid != m_listen_sid)
		return;

	slog_v("__onSvrTranSendDataEndMsg");
	__Client* client = __getClientBySid(tran_sid);
	if (client == nullptr)
		return;
	client->onSendDataEnd();

	__doSendPacks();
}

void ServerNetwork::__onSendPackMsg()
{
	__doSendPacks();
}




// private function --
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

void ServerNetwork::__checkTimeOutCgis()
{
	for (SidToClientMap::iterator it = m_sid_to_client_map.begin(); it != m_sid_to_client_map.end(); ++it)
	{
		__Client* client = it->second;
		client->checkTimeOutCgis();
	}
}

void ServerNetwork::__doSendPacks()
{
	std::vector<socket_id_t> err_sids;
	for (SidToClientMap::iterator it = m_sid_to_client_map.begin(); it != m_sid_to_client_map.end(); ++it)
	{
		__Client* client = it->second;
		if (!client->doSendPack())
		{
			err_sids.push_back(it->first);
		}
	}

	for (size_t i = 0; i < err_sids.size(); ++i)
	{
		__stopClient(err_sids[i]);
	}
}

void ServerNetwork::__postSendPackMsgToSelf()
{
	Message* msg = new Message();
	msg->m_sender = this;
	msg->m_target = this;
	msg->m_msg_type = __EMsgType_sendPack;
	m_init_param.m_work_looper->postMessage(msg);
}

ServerNetwork::__Client* ServerNetwork::__getClientBySid(socket_id_t sid)
{
	SidToClientMap::iterator it = m_sid_to_client_map.find(sid);
	if (it == m_sid_to_client_map.end())
		return NULL;
	return it->second;
}

ServerCgiInfo * ServerNetwork::__getServerCgiInofoBySendCmdType(uint32_t send_cmd_type)
{
	auto it = m_init_param.m_send_cmd_type_to_cgi_info_map.find(send_cmd_type);
	if (it == m_init_param.m_send_cmd_type_to_cgi_info_map.end())
		return nullptr;
	return &it->second;
}









// __Client ------------------------------------------------------------------------
ServerNetwork::__Client::__Client(socket_id_t sid, InitParam* init_param, ServerNetwork* network)
{ 
	m_sid = sid; 
	m_init_param = init_param; 
	m_network = network;
	m_sending_index = -1; 
}

ServerNetwork::__Client::~__Client() 
{
	delete_and_erase_collection_elements(&m_cgi_ctxs);
}

void ServerNetwork::__Client::addCgi(ServerCgi * cgi)
{
	cgi->setStartMs(TimeUtil::getMsTime());

	__CgiCtx* cgi_ctx = new __CgiCtx();
	cgi_ctx->m_cgi = cgi;
	m_cgi_ctxs.push_back(cgi_ctx);
}

void ServerNetwork::__Client::stopCgi(ServerCgi * cgi)
{
	int cgi_index = getCgiIndexBySendPackId(cgi->getSendPack()->m_send_pack_id);
	if (cgi_index < 0)
		return;

	__CgiCtx* cgi_ctx = m_cgi_ctxs[cgi_index];
	delete_and_erase_vector_element_by_index(&m_cgi_ctxs, cgi_index);
	if (m_sending_index == cgi_index)
	{
		m_sending_index = -1;
	}
}

bool ServerNetwork::__Client::doSendPack()
{	
	if (m_sending_index >= 0)
		return true;
	if (m_cgi_ctxs.size() == 0)
		return true;

	for (size_t i = 0; i < m_cgi_ctxs.size(); ++i)
	{
		__CgiCtx* cgi_ctx = m_cgi_ctxs[i];
		if (cgi_ctx->m_is_sent)
			continue;

		m_sending_index = (int)i;
		SendPack* send_pack = cgi_ctx->m_cgi->getSendPack();

		if (!m_init_param->m_sapi->sendDataFromSvrTranSocketToClient(send_pack->m_sid, send_pack->m_send_whole_pack_bin.getData(), send_pack->m_send_whole_pack_bin.getLen()))
		{
			slog_e("ServerNetwork::__Client::doSendPack fail to sendDataFromSvrTranSocketToClient. cgi=%0, send_pack=%1", (uint64_t)cgi_ctx->m_cgi, send_pack->toOverviewString());
			return false;
		}

		break;
	}

	return true;
}

void ServerNetwork::__Client::checkTimeOutCgis()
{
	uint64_t cur_time = TimeUtil::getMsTime();
	for (size_t i = 0; i < m_cgi_ctxs.size(); ++i)
	{
		__CgiCtx* cgi_ctx = m_cgi_ctxs[i];
		if (cur_time < cgi_ctx->m_cgi->getStartMs() + m_init_param->m_cgi_time_out_ms)
			continue;

		__markCgiDoneByIndex((int)i, EErrCode_system);
		--i;
	}
}

int ServerNetwork::__Client::getCgiIndexBySendPackId(uint64_t send_pack_id)
{
	for (size_t i = 0; i < m_cgi_ctxs.size(); ++i)
	{
		if (m_cgi_ctxs[i]->m_cgi->getSendPack()->m_send_pack_id == send_pack_id)
			return (int)i;
	}
	return -1;
}

void ServerNetwork::__Client::onRecvPack(RecvPack * recv_pack)
{
	std::unique_ptr<RecvPack> up_recv_pack(recv_pack);

	ServerCgiInfo* cgi_info = __getServerCgiInofoByRecvCmdType(recv_pack->m_recv_cmd_type);
	if (cgi_info == nullptr)
	{
		slog_e("ServerNetwork::__Client::onRecvPack fail to find cgi info, recv_pack=%0", recv_pack->toOverviewString());
		return;
	}
	
	if (cgi_info->m_cgi_type == EServerCgiType_c2sNotify)
	{
		m_init_param->m_callback->onServerNetwork_recvC2sNotifyPack(m_network, &up_recv_pack);
	}
	else if (cgi_info->m_cgi_type == EServerCgiType_c2sReq_s2cResp)
	{
		m_init_param->m_callback->onServerNetwork_recvC2sReqPack(m_network, &up_recv_pack);
	}
	else if (cgi_info->m_cgi_type == EServerCgiType_s2cReq_c2sResp)
	{
		int cgi_index = __getCgiIndexBySendPackSeq(recv_pack->m_recv_seq);
		if (cgi_index < 0)
		{
			slog_d("ServerNetwork::__Client::onRecvPack recv c2sResp pack, but can't find cgi, maybe is canceled? ignore. recv_pack=%0", recv_pack->toOverviewString());
			return;
		}

		__markCgiDoneByIndex(cgi_index, EErrCode_ok);
	}
	else
	{
		slog_e("ServerNetwork::__Client::onRecvPack err path");
	}
}

void ServerNetwork::__Client::onSendDataEnd()
{
	if (m_sending_index < 0)
	{
		slog_d("ServerNetwork::__onSvrTranSendDataEndMsg client->m_sending_index < 0?");
		doSendPack();
		return;
	}

	int sending_index = m_sending_index;
	m_sending_index = -1;

	__CgiCtx* cgi_ctx = m_cgi_ctxs[sending_index];
	if (cgi_ctx->m_cgi->getServerCgiInfo().m_cgi_type == EServerCgiType_s2cPush || cgi_ctx->m_cgi->getServerCgiInfo().m_cgi_type == EServerCgiType_c2sReq_s2cResp)
	{
		__markCgiDoneByIndex(sending_index, EErrCode_ok);
	}
}

void ServerNetwork::__Client::__markCgiDoneByIndex(int cgi_index, EErrCode err_code)
{
	if (cgi_index < 0 || cgi_index >= m_cgi_ctxs.size())
		return;

	__CgiCtx* cgi_ctx = m_cgi_ctxs[cgi_index];
	m_cgi_ctxs.erase(m_cgi_ctxs.begin() + cgi_index);

	ServerCgi* cgi = cgi_ctx->m_cgi;
	cgi->setErrCode(err_code);
	cgi->setEndMs(TimeUtil::getMsTime());
	m_init_param->m_callback->onServerNetwork_cgiDone(m_network, cgi);

	delete cgi_ctx;
}

int ServerNetwork::__Client::__getCgiIndexBySendPackSeq(uint32_t send_pack_seq)
{
	return 0;
}

ServerCgiInfo * ServerNetwork::__Client::__getServerCgiInofoByRecvCmdType(uint32_t recv_cmd_type)
{
	for (auto it = m_init_param->m_send_cmd_type_to_cgi_info_map.begin(); it != m_init_param->m_send_cmd_type_to_cgi_info_map.end(); ++it)
	{
		if (it->second.m_recv_cmd_type == recv_cmd_type)
			return &it->second;
	}

	return nullptr;
}




SSVR_NAMESPACE_END
