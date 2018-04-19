#include "clientNetSpeedTester.h"
SCLIENT_NAMESPACE_BEGIN



bool ClientNetSpeedTester::parseTestResultFromMsg(TestResult * r, Message * msg)
{
	r->m_svr_ip_or_name = msg->m_args.getString("svr_ip_or_name");
	r->m_svr_port = msg->m_args.getUint32("svr_port");
	r->m_send_bytes_per_second = msg->m_args.getUint64("send_bytes_per_second");
	r->m_recv_bytes_per_second = msg->m_args.getUint64("recv_bytes_per_second");
	r->m_is_connected = msg->m_args.getUint8("is_connected");

	return true;
}




ClientNetSpeedTester::ClientNetSpeedTester()
{
	slog_d("new ClientNetSpeedTester=%0", (uint64_t)this);
	m_is_running = false;
}

ClientNetSpeedTester::~ClientNetSpeedTester()
{
	slog_d("delete ClientNetSpeedTester=%0", (uint64_t)this);
	{
		ScopeMutex __l(m_mutex);
		for (size_t i = 0; i < m_client_ctx_vector.size(); ++i)
		{
			m_init_param.m_sapi->releaseClientSocket(m_client_ctx_vector[i]->m_sid);
			delete m_client_ctx_vector[i];
		}
	}
}

bool ClientNetSpeedTester::init(const InitParam& param)
{
	ScopeMutex __l(m_mutex);

	if (param.m_notify_looper == NULL || param.m_notify_target == NULL || param.m_sapi == NULL || param.m_work_looper == NULL || param.m_svr_infos.size() == 0)
		return false;

	m_init_param = param;

	for (size_t i = 0; i < m_init_param.m_svr_infos.size(); ++i)
	{
		__ClientCtx* c = new __ClientCtx();
		c->m_svr_info = m_init_param.m_svr_infos[i];
		{
			ITcpSocketCallbackApi::CreateClientSocketParam param;
			param.m_callback_looper = m_init_param.m_work_looper;
			param.m_callback_target = this;
			param.m_svr_ip_or_name = c->m_svr_info.m_svr_ip_or_name;
			param.m_svr_port = c->m_svr_info.m_svr_port;
			if (!m_init_param.m_sapi->createClientSocket(&c->m_sid, param))
			{
				delete c;
				return false;
			}				
		}
		m_client_ctx_vector.push_back(c);
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

	for (size_t i = 0; i < m_client_ctx_vector.size(); ++i)
	{
		if (!m_init_param.m_sapi->startClientSocket(m_client_ctx_vector[i]->m_sid))
			return false;
		m_client_ctx_vector[i]->m_connect_state = __EClientConncectState_connecting;
	}

	Message* msg = new Message();
	msg->m_msg_type = EMsgType_onTestStart;
	__postMsgToTarget(msg);

	m_init_param.m_work_looper->addMsgHandler(this);
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
	}
}

void ClientNetSpeedTester::__onMessage_clientConnected(const ITcpSocketCallbackApi::ClientSocketConnectedMsg & msg)
{
	socket_id_t sid = msg.m_client_sid;
	int index = __getClientCtxIndexBySid(sid);
	if (index < 0)
		return;
	slog_d("speed_test connected");
	__ClientCtx* ctx = m_client_ctx_vector[index];
	ctx->m_connect_state = __EClientConncectState_connected;

	Message* m = new Message();
	m->m_msg_type = EMsgType_onOneSvrResultUpdate;
	m->m_args.setString("svr_ip_or_name", ctx->m_svr_info.m_svr_ip_or_name);
	m->m_args.setUint32("svr_port", ctx->m_svr_info.m_svr_port);
	m->m_args.setUint64("send_bytes_per_second", ctx->m_send_bytes_per_second);
	m->m_args.setUint64("recv_bytes_per_second", ctx->m_recv_bytes_per_second);
	m->m_args.setUint8("is_connected", true);
	__postMsgToTarget(m);
}

void ClientNetSpeedTester::__onMessage_clientDisconnected(const ITcpSocketCallbackApi::ClientSocketDisconnectedMsg & msg)
{
	socket_id_t sid = msg.m_client_sid;
	int index = __getClientCtxIndexBySid(sid);
	if (index < 0)
		return;
	slog_d("speed_test disconnected");
	__ClientCtx* ctx = m_client_ctx_vector[index];
	ctx->m_connect_state = __EClientConncectState_disconnected;

	Message* m = new Message();
	m->m_msg_type = EMsgType_onOneSvrResultUpdate;
	m->m_args.setString("svr_ip_or_name", ctx->m_svr_info.m_svr_ip_or_name);
	m->m_args.setUint32("svr_port", ctx->m_svr_info.m_svr_port);
	m->m_args.setUint64("send_bytes_per_second", ctx->m_send_bytes_per_second);
	m->m_args.setUint64("recv_bytes_per_second", ctx->m_recv_bytes_per_second);
	m->m_args.setUint8("is_connected", false);
	__postMsgToTarget(m);

	bool is_done = true;
	for (size_t i = 0; i < m_client_ctx_vector.size(); ++i)
	{
		if (m_client_ctx_vector[i]->m_connect_state != __EClientConncectState_disconnected)
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

void ClientNetSpeedTester::__stop()
{
	if (!m_is_running)
		return;
	m_is_running = false;

	m_init_param.m_work_looper->removeMsgHandler(this);

	for (size_t i = 0; i < m_client_ctx_vector.size(); ++i)
	{
		m_init_param.m_sapi->stopClientSocket(m_client_ctx_vector[i]->m_sid);
	}

	Message* msg = new Message();
	msg->m_msg_type = EMsgType_onTestEnd;
	__postMsgToTarget(msg);
}

void ClientNetSpeedTester::__postMsgToTarget(Message * msg)
{
	msg->m_sender = this;
	msg->m_target = m_init_param.m_notify_target;
	m_init_param.m_notify_looper->postMessage(msg);
}

int ClientNetSpeedTester::__getClientCtxIndexBySid(socket_id_t sid)
{
	for (size_t i = 0; i < m_client_ctx_vector.size(); ++i)
	{
		if (sid == m_client_ctx_vector[i]->m_sid)
			return (int)i;
	}
	return -1;
}



SCLIENT_NAMESPACE_END
