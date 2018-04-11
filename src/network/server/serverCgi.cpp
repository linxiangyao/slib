#include "serverCgi.h"
SSVR_NAMESPACE_BEGIN






// session_id_t ------------------------------------------------------------------------------------
//bool operator < (const session_id_t & l, const session_id_t & r)
//{
//	if (l.m_app_id < r.m_app_id)
//		return true;
//	else if (l.m_app_id > r.m_app_id)
//		return false;
//
//	return l.m_pack_session_id < r.m_pack_session_id;
//}
//
//bool operator == (const session_id_t & l, const session_id_t & r)
//{
//	return l.m_app_id == r.m_app_id && l.m_pack_session_id == r.m_pack_session_id;
//}
//
//bool operator != (const session_id_t& l, const session_id_t& r)
//{
//	return !(l == r);
//}

ServerCgi::ServerCgi() { m_send_pack = NULL; m_recv_pack = NULL; m_callback = NULL; m_start_ms = 0; m_end_ms = 0; m_err_code = ServerNetwork::EErrCode_ok; }
ServerCgi::~ServerCgi() { delete m_send_pack; delete m_recv_pack; }

session_id_t ServerCgi::getSessionId()
{
	if (m_send_pack != NULL)
		return m_send_pack->m_ssid;
	else if (m_recv_pack != NULL)
		return m_recv_pack->m_ssid;
	else
		return session_id_t();
}

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
