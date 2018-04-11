#include "clientCgiMgr.h"
SCLIENT_NAMESPACE_BEGIN



ClientCgiMgr::ClientCgiMgr()
{
	slog_d("new ClientCgiMgr=%0", (uint64_t)this);
}

ClientCgiMgr::~ClientCgiMgr()
{
	slog_d("delete ClientCgiMgr=%0", (uint64_t)this);
	delete_and_erase_collection_elements(&m_cgis);
	//m_init_param.m_work_looper->removeMsgHandler(this);
}

bool ClientCgiMgr::init(const InitParam & param)
{
	slog_d("init ClientCgiMgr");
	if (param.m_network == NULL || param.m_cgi_infos.size() == 0 || param.m_callback == NULL)
	{
		slog_e("param err");
		return false;
	}

	m_init_param = param;
	//m_init_param.m_work_looper->addMsgHandler(this);
	return true;
}

bool ClientCgiMgr::startCgi(ClientCgi* cgi)
{
	slog_v("start cgi, cgi=%0", (uint64_t)cgi);
	if (cgi == NULL || cgi->getCallback() == NULL || cgi->getSendPack() == NULL)
	{
		slog_e("cgi param err");
		return false;
	}

	int index = get_vector_index_by_element(m_cgis, cgi);
	if (index >= 0)
		return true;

	if (!m_init_param.m_network->sendPack(*cgi->getSendPack()))
	{
		slog_e("fail to send pack");
		return false;
	}

	cgi->setStartMs(TimeUtil::getMsTime());
		
	m_cgis.push_back(cgi);
	return true;
}

void ClientCgiMgr::stopCgi(ClientCgi* cgi)
{
	slog_v("start cgi, cgi=%0", (uint64_t)cgi);
	int index = get_vector_index_by_element(m_cgis, cgi);
	if (index < 0)
		return;

	m_init_param.m_network->cancelSendPack(cgi->getSendPack()->m_send_pack_id);
	delete_and_erase_vector_element_by_index(&m_cgis, index);
}





void ClientCgiMgr::onClientNetworkStatred(ClientNetwork * network)
{

}

void ClientCgiMgr::onClientNetworkStopped(ClientNetwork * network)
{

}

void ClientCgiMgr::onClientNetworkConnectStateChanged(ClientNetwork * network, ClientNetwork::EConnectState state)
{

}

void ClientCgiMgr::onClientNetworkSendPackEnd(ClientNetwork * network, ClientNetwork::EErrType err_type, int err_code, uint64_t send_pack_id, std::unique_ptr<RecvPack>* recv_pack)
{
	int cgi_index = __getCgiIndexBySendPackId(send_pack_id);
	if (cgi_index < 0)
	{
		slog_e("fail to find cgi index, send_pack_id=%0", send_pack_id);
		return;
	}

	ClientCgi* cgi = m_cgis[cgi_index];
	cgi->setErrType(err_type);
	cgi->setErrCode(err_code);
	if (cgi->getCgiInfo().m_cgi_type == EClientCgiType_c2sReq_s2cResp)
	{
		if (cgi->getIsSuccess())
		{
			cgi->setRecvPack(recv_pack->release());
		}
	}
	else if (cgi->getCgiInfo().m_cgi_type == EClientCgiType_c2sNotify)
	{
		// nothing to do
	}
	else if (cgi->getCgiInfo().m_cgi_type == EClientCgiType_s2cReq_cs2Resp)
	{
		// nothing to do
	}
	else
	{
		slog_e("invalid path");
	}
	cgi->setEndMs(TimeUtil::getMsTime());
	slog_v("call back cgi=%0", (uint64_t)cgi);
	m_cgis.erase(m_cgis.begin() + cgi_index);
	cgi->getCallback()->onClientCgi_cgiDone(cgi);
}

void ClientCgiMgr::onClientNetworkRecvPack(ClientNetwork * network, std::unique_ptr<RecvPack>* recv_pack)
{
	RecvPack* p = recv_pack->get();
	int cgi_info_index = __getCgiInfoIndexByRecvPackCmdType(p->m_recv_cmd_type);
	if (cgi_info_index < 0)
	{
		slog_e("recv unkonw pack, recv_cmd_type=%0", p->m_recv_cmd_type);
		return;
	}
	
	ClientCgiInfo& info = m_init_param.m_cgi_infos[cgi_info_index];
	if (info.m_cgi_type == EClientCgiType_s2cPush)
	{
		m_init_param.m_callback->onClientCgiMgr_recvS2cPushPack(recv_pack);
	}
	else if (info.m_cgi_type == EClientCgiType_s2cReq_cs2Resp)
	{
		m_init_param.m_callback->onClientCgiMgr_recvS2cReqPack(recv_pack);
	}
	else
	{
		slog_e("invalid path");
	}
}


int ClientCgiMgr::__getCgiIndexBySendPackId(uint64_t send_pack_id)
{
	for (size_t i = 0; i < m_cgis.size(); ++i)
	{
		if (send_pack_id == m_cgis[i]->getSendPack()->m_send_pack_id)
			return (int)i;
	}
	return -1;
}



SCLIENT_NAMESPACE_END
