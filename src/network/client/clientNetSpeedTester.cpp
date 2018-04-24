#include "clientNetSpeedTester.h"
SCLIENT_NAMESPACE_BEGIN


ClientNetSpeedTester::ClientNetSpeedTester()
{
	slog_d("new ClientNetSpeedTester=%0", (uint64_t)this);
	m_is_running = false;
}

ClientNetSpeedTester::~ClientNetSpeedTester()
{
	//slog_d("delete ClientNetSpeedTester=%0", (uint64_t)this);
	//{
	//	ScopeMutex __l(m_mutex);
	//	for (size_t i = 0; i < m_client_ctx_vector.size(); ++i)
	//	{
	//		m_init_param.m_sapi->releaseClientSocket(m_client_ctx_vector[i]->m_sid);
	//		delete m_client_ctx_vector[i];
	//	}
	//}
}

bool ClientNetSpeedTester::init(const InitParam& param)
{
	ScopeMutex __l(m_mutex);

	if (param.m_notify_looper == nullptr || param.m_notify_target == nullptr || param.m_sapi == nullptr
		|| param.m_work_looper == nullptr ||  param.m_dns_resolver == nullptr || param.m_svr_infos.size() == 0)
		return false;

	m_init_param = param;

	for (size_t i = 0; i < m_init_param.m_svr_infos.size(); ++i)
	{
		__ClientCtx* client = new __ClientCtx(m_init_param.m_svr_infos[i], &m_init_param);
		m_client_ctx_vector.push_back(client);

	}

	return true;
}

bool ClientNetSpeedTester::start()
{
	ScopeMutex __l(m_mutex);
	if (m_is_running)
		return true;
	m_is_running = true;
	slog_d("speed_test start");

	m_init_param.m_work_looper->addMsgHandler(this);

	Message* msg = new Message();
	msg->m_msg_type = EMsgType_onTestStart;
	__postMsgToTarget(msg);

	__doTest();
	return true;
}

void ClientNetSpeedTester::stop()
{
	ScopeMutex __l(m_mutex);
	slog_d("speed_test stop");
	__stop();
}




void ClientNetSpeedTester::onMessage(Message * msg, bool* isHandled)
{
	ScopeMutex __l(m_mutex);
	if (msg->m_target != this)
		return;
	if (!m_is_running)
		return;
	*isHandled = true;

	switch (msg->m_msg_type)
	{
	case ITcpSocketCallbackApi::EMsgType_clientSocketConnected:
		__onMessage_clientConnected(*(ITcpSocketCallbackApi::ClientSocketConnectedMsg*)msg);
		break;

	case ITcpSocketCallbackApi::EMsgType_clientSocketDisconnected:
		__onMessage_clientDisconnected(*(ITcpSocketCallbackApi::ClientSocketDisconnectedMsg*)msg);
		break;

	case MSG_TYPE_DnsResolver_resolveEnd:
		__onMessage_dnsResolved(msg);
		break;
	}
}

void ClientNetSpeedTester::__onMessage_clientConnected(const ITcpSocketCallbackApi::ClientSocketConnectedMsg & msg)
{
	for (size_t i = 0; i < m_client_ctx_vector.size(); ++i)
	{
		m_client_ctx_vector[i]->onClientConnected(msg.m_client_sid);
	}
	__checkIsDone();
}

void ClientNetSpeedTester::__onMessage_clientDisconnected(const ITcpSocketCallbackApi::ClientSocketDisconnectedMsg & msg)
{
	for (size_t i = 0; i < m_client_ctx_vector.size(); ++i)
	{
		m_client_ctx_vector[i]->onClientDisconnected(msg.m_client_sid);
	}
	__checkIsDone();
}

void ClientNetSpeedTester::__onMessage_dnsResolved(Message * msg)
{
	DnsResolver::Msg_ResolveEnd* m = (DnsResolver::Msg_ResolveEnd*)msg;
	__ClientCtx* client = __getClientCtxBySvrName(m->m_record.m_name);
	if (client == nullptr)
		return;
	client->onDnsResolveEnd(m->m_record);
}

void ClientNetSpeedTester::__checkIsDone()
{
	if (!m_is_running)
		return;

	bool is_done = true;
	for (size_t i = 0; i < m_client_ctx_vector.size(); ++i)
	{
		if (!m_client_ctx_vector[i]->getIsDone())
		{
			is_done = false;
			break;
		}
	}
	if (is_done)
	{
		slog_d("speed_test done, stop");
		__stop();
	}
}

void ClientNetSpeedTester::__doTest()
{
	for (size_t i = 0; i < m_client_ctx_vector.size(); ++i)
	{
		m_client_ctx_vector[i]->doTest();
	}
}

void ClientNetSpeedTester::__stop()
{
	/*if (!m_is_running)
		return;
	m_is_running = false;

	m_init_param.m_work_looper->removeMsgHandler(this);

	for (size_t i = 0; i < m_client_ctx_vector.size(); ++i)
	{
		m_init_param.m_sapi->stopClientSocket(m_client_ctx_vector[i]->m_sid);
	}

	Message* msg = new Message();
	msg->m_msg_type = EMsgType_onTestEnd;
	__postMsgToTarget(msg);*/
}

void ClientNetSpeedTester::__postMsgToTarget(Message * msg)
{
	msg->m_sender = this;
	msg->m_target = m_init_param.m_notify_target;
	m_init_param.m_notify_looper->postMessage(msg);
}

int ClientNetSpeedTester::__getClientCtxIndexBySvrName(const std::string & svr_ip_or_name)
{
	for (size_t i = 0; i < m_client_ctx_vector.size(); ++i)
	{
		if (m_client_ctx_vector[i]->m_svr_info.m_svr_ip_or_name == svr_ip_or_name)
			return i;
	}
	return -1;
}

ClientNetSpeedTester::__ClientCtx * ClientNetSpeedTester::__getClientCtxBySvrName(const std::string & svr_ip_or_name)
{
	int index = __getClientCtxIndexBySvrName(svr_ip_or_name);
	if (index < 0)
		return nullptr;
	else
		return m_client_ctx_vector[index];
}












ClientNetSpeedTester::__ClientCtx::__ClientCtx(const SvrInfo& svr_info, InitParam* param)
{
	m_svr_info = svr_info;
	m_init_param = param;
}

ClientNetSpeedTester::__ClientCtx::~__ClientCtx()
{
	doStop();
}

void ClientNetSpeedTester::__ClientCtx::doTest()
{
	__doDns();
}

void ClientNetSpeedTester::__ClientCtx::doStop()
{
	for (size_t i = 0; i < m_ip_records.size(); ++i)
	{
		m_init_param->m_sapi->releaseClientSocket(m_ip_records[i].m_sid);
	}
	m_ip_records.clear();
}

bool ClientNetSpeedTester::__ClientCtx::getIsDone()
{
	for (size_t i = 0; i < m_ip_records.size(); ++i)
	{
		if (m_ip_records[i].m_connect_state == __EClientConncectState_connecting)
			return false;
	}
	return true;
}

void ClientNetSpeedTester::__ClientCtx::onDnsResolveEnd(const DnsResolver::DnsRecord & dns_record)
{
	if (dns_record.m_name != m_svr_info.m_svr_ip_or_name)
		return;
	doTest();
}

void ClientNetSpeedTester::__ClientCtx::onClientConnected(socket_id_t sid)
{
	int ip_index = __getIpIndexBySid(sid);
	if (ip_index < 0)
		return;
	slog_d("speed_test connected");
	__OneIpTestRecord* record = &m_ip_records[ip_index];
	record->m_connect_state = __EClientConncectState_connected;
	__notifyOneIpTestResult(*record);
}

void ClientNetSpeedTester::__ClientCtx::onClientDisconnected(socket_id_t sid)
{
	int ip_index = __getIpIndexBySid(sid);
	if (ip_index < 0)
		return;
	__OneIpTestRecord* record = &m_ip_records[ip_index];
	record->m_connect_state = __EClientConncectState_disconnected;
	__notifyOneIpTestResult(*record);
}

void ClientNetSpeedTester::__ClientCtx::__doDns()
{
	if (m_ip_records.size() > 0)
		return;

	DnsResolver::DnsRecord dns_record;
	if (m_init_param->m_dns_resolver->getIpByName(m_svr_info.m_svr_ip_or_name, &dns_record))
	{
		__addDnsRecord(dns_record);
		return;
	}

	m_init_param->m_dns_resolver->startResolve(m_svr_info.m_svr_ip_or_name);
}

void ClientNetSpeedTester::__ClientCtx::__postMsg(Message * msg) 
{
	msg->m_sender = this;
	msg->m_target = m_init_param->m_notify_target;
	m_init_param->m_notify_looper->postMessage(msg);
}

void ClientNetSpeedTester::__ClientCtx::__notifyOneIpTestResult(const __OneIpTestRecord & record)
{
	Msg_oneTestResult* m = new Msg_oneTestResult();
	m->m_is_connected = record.m_connect_state == __EClientConncectState_connected;
	m->m_svr_ip_or_name = m_svr_info.m_svr_ip_or_name;
	m->m_svr_ip = record.m_ip;
	SocketUtil::ipToStr(m->m_svr_ip, &m->m_svr_ip_str);
	m->m_svr_port = m_svr_info.m_svr_port;
	__postMsg(m);
}

void ClientNetSpeedTester::__ClientCtx::__addDnsRecord(const DnsResolver::DnsRecord& dns_record)
{
	if (m_ip_records.size() > 0)
		return;

	for (size_t i = 0; i < dns_record.m_ips.size(); ++i)
	{
		__OneIpTestRecord test_record;
		test_record.m_connect_state = __EClientConncectState_connecting;
		test_record.m_ip = dns_record.m_ips[i];

		ITcpSocketCallbackApi::CreateClientSocketParam param;
		param.m_callback_looper = m_init_param->m_work_looper;
		param.m_callback_target = this;
		SocketUtil::ipToStr(test_record.m_ip, &param.m_svr_ip);
		param.m_svr_port = m_svr_info.m_svr_port;
		if (!m_init_param->m_sapi->createClientSocket(&test_record.m_sid, param))
		{
		}
		m_ip_records.push_back(test_record);
	}
}


SCLIENT_NAMESPACE_END
