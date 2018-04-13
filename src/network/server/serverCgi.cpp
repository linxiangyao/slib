#include "serverCgi.h"
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

ServerCgi::ServerCgi()
{ 
	m_send_pack = NULL;
	m_recv_pack = NULL;
	m_callback = NULL;
	m_start_ms = 0;
	m_end_ms = 0;
	m_err_code = ServerNetwork::EErrCode_ok;
}

ServerCgi::~ServerCgi() 
{ 
	delete m_send_pack;
	delete m_recv_pack;
}

ServerCgi::RecvPack * ServerCgi::getRecvPack() { return m_recv_pack; }

ServerCgi::SendPack * ServerCgi::getSendPack() { return m_send_pack; }

ServerCgi::ICallback * ServerCgi::getCallback() { return m_callback; }

ServerCgi::EErrCode ServerCgi::getErrCode() { return m_err_code; }

uint64_t ServerCgi::getStartMs() { return m_start_ms; }

uint64_t ServerCgi::getEndMs() { return m_end_ms; }

bool ServerCgi::getIsCgiSuccess() { return m_err_code == ServerNetwork::EErrCode_ok; }

session_id_t ServerCgi::getSessionId()
{
	if (m_send_pack != NULL)
		return m_send_pack->m_ssid;
	else if (m_recv_pack != NULL)
		return m_recv_pack->m_ssid;
	else
		return session_id_t();
}

void ServerCgi::setCallback(ICallback * callback) { m_callback = callback; }

void ServerCgi::setErrCode(EErrCode err_code) { m_err_code = err_code; }

void ServerCgi::setStartMs(uint64_t ms) { m_start_ms = ms; }

void ServerCgi::setEndMs(uint64_t ms) { m_end_ms = ms; }

void ServerCgi::setSendPack(SendPack * send_pack) { __setSendPack(send_pack); }

void ServerCgi::setRecvPack(RecvPack * recv_pack) { m_recv_pack = recv_pack; onSetRecvPackEnd(); }





void ServerCgi::__setSendPack(SendPack * send_pack)
{
	m_send_pack = send_pack;
	if (send_pack == NULL)
		return;
	send_pack->m_send_cmd_type = getServerCgiInfo().m_send_cmd_type;

	if (m_recv_pack == NULL)
		return;
	if (getServerCgiInfo().m_cgi_type == EServerCgiType_c2sReq_s2cResp)
	{
		send_pack->m_send_seq = m_recv_pack->m_recv_seq;
		send_pack->m_ssid = m_recv_pack->m_ssid;
		send_pack->m_sid = m_recv_pack->m_sid;
	}
}








SSVR_NAMESPACE_END
