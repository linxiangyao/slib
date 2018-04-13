#include "serverCgiMgr.h"
SSVR_NAMESPACE_BEGIN




// interface ------------------------------------------------------------------------------------------
ServerCgiMgr::ServerCgiMgr()
{
	slog_d("new ServerCgiMgr=%0", (uint64_t)this);
	m_timer_id = 0;
}

ServerCgiMgr::~ServerCgiMgr()
{
	slog_d("delete ServerCgiMgr=%0", (uint64_t)this);
	m_init_param.m_work_looper->removeMsgHandler(this);
	m_init_param.m_work_looper->removeMsgTimerHandler(this);
	delete_and_erase_collection_elements(&m_session_ctx_map);
	m_init_param.m_work_looper->releasseTimer(m_timer_id);
}

bool ServerCgiMgr::init(const InitParam& param)
{
	slog_d("init ServerCgiMgr");
	if (param.m_network == NULL || param.m_work_looper == NULL || param.m_callback == nullptr)
	{
		slog_e("ServerCgiMgr::init fail, param error!");
		return false;
	}

	m_init_param = param;
	m_init_param.m_work_looper->addMsgHandler(this);
	m_init_param.m_work_looper->addMsgTimerHandler(this);

	m_timer_id = m_init_param.m_work_looper->createTimer(this);
	if (m_timer_id == 0 || !m_init_param.m_work_looper->startTimer(m_timer_id, 1, 1))
	{
		slog_i("ServerCgiMgr::init fail to start timer");
		return false;
	}

	return true;
}

bool ServerCgiMgr::startCgi(ServerCgi * cgi)
{
	slog_v("startCgi cgi=%0, ssid=%1", (uint64_t)cgi,  cgi->getSessionId().toString());
	if (cgi == NULL || cgi->getCallback() == NULL || cgi->getSendPack() == NULL)
	{
		slog_e("ServerCgiMgr::startCgi fail, cgi param err");
		return false;
	}
	int index = __getCgiInfoIndexSendCmdType(cgi->getSendPack()->m_send_cmd_type);
	if (index < 0)
	{
		slog_e("ServerCgiMgr::startCgi fail to find cgi info");
		return false;
	}
	__SessionCtx* ctx = get_map_element_by_key(m_session_ctx_map, cgi->getSessionId());
	if (ctx == NULL)
	{
		slog_e("ServerCgiMgr::startCgi fail to find session");
		return false;
	}

	if (!m_init_param.m_network->sendPackToClient(*cgi->getSendPack()))
	{
		slog_e("ServerCgiMgr::startCgi fail to send pack to client");
		return false;
	}
	cgi->setStartMs(TimeUtil::getMsTime());
	
	add_vector_unique_element(&m_cgis, cgi);
	return true;
}

void ServerCgiMgr::stopCgi(ServerCgi * cgi)
{
	slog_v("stopCgi cgi=%0, ssid=%1", (uint64_t)cgi, cgi->getSessionId().toString());
	int index = get_vector_index_by_element(m_cgis, cgi);
	if (index < 0)
		return;

	__SessionCtx* ctx = get_map_element_by_key(m_session_ctx_map, cgi->getSessionId());
	if (ctx == NULL)
	{
		slog_d("ServerCgiMgr::stopCgi fail to find session, maybe closed? ignore.");
		return;
	}

	m_init_param.m_network->cancelSendPackToClient(ctx->m_sid, cgi->getSendPack()->m_send_pack_id);
	erase_vector_element_by_index(&m_cgis, index);
}

void ServerCgiMgr::closeSession(session_id_t ssid)
{
	slog_d("closeSession ssid=%0", ssid.toString());
	if (!is_map_contain_key(m_session_ctx_map, ssid))
		return;

	socket_id_t sid = m_session_ctx_map[ssid]->m_sid;
	delete_and_erase_map_element_by_key(&m_session_ctx_map, ssid);
	m_init_param.m_callback->onServerCgiMgr_sessionClosed(sid, ssid);
}

void ServerCgiMgr::refreashSessionLife(session_id_t ssid)
{
	slog_v("refreashSessionLife ssid=%0", ssid.toString());
	__SessionCtx* ctx = get_map_element_by_key(m_session_ctx_map, ssid);
	if (ctx == NULL)
		return;

	ctx->m_refresh_life_time_ms = TimeUtil::getMsTime();
}






// message ------------------------------------------------------------------------------------------
void ServerCgiMgr::onServerNetwork_clientConnected(ServerNetwork * network, socket_id_t sid)
{
}

void ServerCgiMgr::onServerNetwork_clientDisconnected(ServerNetwork * network, socket_id_t sid)
{
}

void ServerCgiMgr::onServerNetwork_sendPackToClientEnd(ServerNetwork * network, socket_id_t sid, ServerNetwork::EErrCode err_code, uint64_t send_pack_id, ServerNetwork::RecvPack * recv_pack)
{
	int cgi_index = __getCgiIndexBySendPackId(send_pack_id);
	if (cgi_index < 0)
		return;
	ServerCgi* cgi = m_cgis[cgi_index];
	m_cgis.erase(m_cgis.begin() + cgi_index);
	cgi->setErrCode(err_code);
	cgi->setEndMs(TimeUtil::getMsTime());
	cgi->getCallback()->onServerCgi_cgiDone(cgi);
}

void ServerCgiMgr::onServerNetwork_recvPackFromClient(ServerNetwork * network, socket_id_t sid, ServerNetwork::RecvPack * recv_pack)
{
	int index = __getCgiInfoIndexByRecvCmdType(recv_pack->m_recv_cmd_type);
	if (index < 0)
	{
		delete recv_pack;
		return;
	}

	ServerCgiInfo& cgi_info = m_init_param.m_cgi_infos[index];

	if (cgi_info.m_cgi_type == EServerCgiType_c2sNotify)
	{
		session_id_t ssid = recv_pack->m_ssid;
		__SessionCtx* ctx = get_map_element_by_key(m_session_ctx_map, ssid);
		if (ctx == NULL)
		{
			ctx = __createSession(ssid, recv_pack->m_sid);
		}
		else
		{
			ctx->m_refresh_life_time_ms = TimeUtil::getMsTime();
			ctx->m_sid = recv_pack->m_sid;
		}
		std::unique_ptr<RecvPack> p(recv_pack);
		m_init_param.m_callback->onServerCgiMgr_recvC2sNotifyPack(&p);
	}
	else if (cgi_info.m_cgi_type == EServerCgiType_c2sReq_s2cResp)
	{
		session_id_t ssid = recv_pack->m_ssid;
		__SessionCtx* ctx = get_map_element_by_key(m_session_ctx_map, ssid);
		if (ctx == NULL)
		{
			ctx = __createSession(ssid, recv_pack->m_sid);
		}
		else
		{
			ctx->m_refresh_life_time_ms = TimeUtil::getMsTime();
			ctx->m_sid = recv_pack->m_sid;
		}
		std::unique_ptr<RecvPack> p(recv_pack);
		m_init_param.m_callback->onServerCgiMgr_recvC2sReqPack(&p);
	}
	else
	{
		delete recv_pack;
		slog_e("err path");
	}
}

void ServerCgiMgr::onMessage(Message* msg, bool* isHandled)
{
}

void ServerCgiMgr::onMessageTimerTick(uint64_t timer_id, void* user_data)
{
	if (timer_id != m_timer_id)
		return;

	std::vector<session_id_t> need_delete_ssids;
	for (SessionIdToSessionMap::iterator it = m_session_ctx_map.begin(); it != m_session_ctx_map.end(); ++it)
	{
		uint64_t time_span_ms = TimeUtil::getMsTime() - it->second->m_refresh_life_time_ms;
		if (time_span_ms > m_init_param.m_session_time_out_second * 1000)
		{
			need_delete_ssids.push_back(it->first);
		}
	}

	for (size_t i = 0; i < need_delete_ssids.size(); ++i)
	{
		session_id_t ssid = need_delete_ssids[i];
		socket_id_t sid = m_session_ctx_map[ssid]->m_sid;
		slog_d("session timeout, closed, sid=%0, ssid=%1", sid, ssid.toString());
		delete_and_erase_map_element_by_key(&m_session_ctx_map, ssid);
		m_init_param.m_callback->onServerCgiMgr_sessionClosed(sid, ssid);
	}
}

int ServerCgiMgr::__getCgiInfoIndexByRecvCmdType(uint32_t cmd_type)
{
	for (size_t i = 0; i < m_init_param.m_cgi_infos.size(); ++i)
	{
		ServerCgiInfo& info = m_init_param.m_cgi_infos[i];
		if (info.m_recv_cmd_type == cmd_type)
			return (int)i;
	}

	return -1;
}

int ServerCgiMgr::__getCgiInfoIndexSendCmdType(uint32_t cmd_type)
{
	for (size_t i = 0; i < m_init_param.m_cgi_infos.size(); ++i)
	{
		ServerCgiInfo& info = m_init_param.m_cgi_infos[i];
		if (info.m_send_cmd_type == cmd_type)
			return (int)i;
	}

	return -1;
}

int ServerCgiMgr::__getCgiIndexBySendPackId(uint64_t send_pack_id)
{
	for (size_t i = 0; i < m_cgis.size(); ++i)
	{
		if (m_cgis[i]->getSendPack()->m_send_pack_id == send_pack_id)
			return (int)i;
	}
	return -1;
}

ServerCgiMgr::__SessionCtx* ServerCgiMgr::__createSession(session_id_t ssid, socket_id_t sid)
{
	__SessionCtx* ctx = new __SessionCtx();
	ctx->m_ssid = ssid;
	ctx->m_sid = sid;
	ctx->m_refresh_life_time_ms = TimeUtil::getMsTime();
	m_session_ctx_map[ssid] = ctx;

	slog_d("session created, sid=%0, ssid=%1", sid, ssid.toString());
	m_init_param.m_callback->onServerCgiMgr_sessionCreated(sid, ssid);
	return ctx;
}




SSVR_NAMESPACE_END
