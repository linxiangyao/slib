//#include "rawClientNetwork.h"
//#include "../../util/timeUtil.h"
//SCLIENT_NAMESPACE_BEGIN
//
//
//
//RawClientNetwork::RawClientNetwork()
//{
//	slog_d("new RawClientNetwork=%0", (uint64_t)this);
//}
//
//RawClientNetwork::~RawClientNetwork()
//{
//	slog_d("delete RawClientNetwork=%0", (uint64_t)this);
//	m_init_param.m_work_looper->removeMsgHandler(this);
//}
//
//bool RawClientNetwork::init(const InitParam & param)
//{
//	slog_d("init");
//	m_init_param = param;
//	m_is_connected = false;
//	m_is_sending = false;
//	m_pack_id_seed = 0;
//
//	m_init_param.m_work_looper->addMsgHandler(this);
//	m_packer = new SimpleTcpPacker();
//
//	ITcpSocketCallbackApi::CreateClientSocketParam p;
//	p.m_svr_ip_or_name = param.m_svr_ip_or_name;
//	p.m_svr_port = param.m_svr_port;
//	p.m_callback_looper = param.m_work_looper;
//	p.m_callback_target = this;
//	if (!m_init_param.m_sapi->createClientSocket(&m_sid, p))
//	{
//		slog_e("fail to init");
//		return false;
//	}
//
//	return true;
//}
//
//bool RawClientNetwork::connect()
//{
//	slog_d("connect");
//	ScopeMutex __l(m_mutex);
//	if (!m_init_param.m_sapi->startClientSocket(m_sid))
//	{
//		slog_e("fail to connect");
//		return false;
//	}
//
//	return true;
//}
//
//void RawClientNetwork::disconnect()
//{
//	slog_d("connect");
//	ScopeMutex __l(m_mutex);
//	m_init_param.m_sapi->stopClientSocket(m_sid);
//}
//
//bool RawClientNetwork::sendPackToSvr(SendPack * send_pack)
//{
//	slog_v("sendPackToSvr");
//	ScopeMutex __l(m_mutex);
//	if (m_send_packs.size() >= m_init_param.m_max_send_pack_count_in_queue)
//		return false;
//	send_pack->m_id = ++m_pack_id_seed;
//	SendPack* p = new SendPack();
//	*p = *send_pack;
//	m_send_packs.push_back(p);
//	Message* msg = new Message();
//	msg->m_msg_type = __EMsgType_sendPacks;
//	__postMessageToSelf(msg);
//	return true;
//}
//
//void RawClientNetwork::cancelSendPackToSvr(uint64_t send_pack_id)
//{
//	slog_v("cancelSendPackToSvr");
//	ScopeMutex __l(m_mutex);
//	int index = __getSendPackIndexBySendPackId(send_pack_id);
//	if (index < 0)
//		return;
//	if (index == 0 && m_is_sending)
//		return;
//	delete_and_erase_vector_element_by_index(&m_send_packs, index);
//}
//
//std::string RawClientNetwork::getSvrIp()
//{
//	ScopeMutex __l(m_mutex);
//	return m_init_param.m_svr_ip_or_name;
//}
//
//uint32_t RawClientNetwork::getSvrPort()
//{
//	ScopeMutex __l(m_mutex);
//	return m_init_param.m_svr_port;
//}
//
//void RawClientNetwork::onMessage(Message * msg, bool * isHandled)
//{
//	if (msg->m_target != this)
//		return;
//	*isHandled = true;
//
//	ScopeMutex __l(m_mutex);
//	if (msg->m_sender == m_init_param.m_sapi)
//	{
//		switch (msg->m_msg_type)
//		{
//		case ITcpSocketCallbackApi::EMsgType_clientSocketConnected: __onMsgTcpSocketClientConnected((ITcpSocketCallbackApi::ClientSocketConnectedMsg*)msg); break;
//		case ITcpSocketCallbackApi::EMsgType_clientSocketDisconnected: __onMsgTcpSocketClientDisconnected((ITcpSocketCallbackApi::ClientSocketDisconnectedMsg*)msg); break;
//		case ITcpSocketCallbackApi::EMsgType_clientSocketRecvData: __onMsgTcpSocketClientRecvData((ITcpSocketCallbackApi::ClientSocketRecvDataMsg*)msg); break;
//		case ITcpSocketCallbackApi::EMsgType_clientSocketSendDataEnd: __onMsgTcpSocketClientSendDataEnd((ITcpSocketCallbackApi::ClientSocketSendDataEndMsg*)msg); break;
//		}
//		return;
//	}
//	else if (msg->m_sender == this)
//	{
//		switch (msg->m_msg_type)
//		{
//		case __EMsgType_sendPacks:
//			__doSendPack();
//			break;
//		}
//	}
//}
//
//void RawClientNetwork::__onMsgTcpSocketClientConnected(ITcpSocketCallbackApi::ClientSocketConnectedMsg * msg)
//{
//	if (msg->m_client_sid != m_sid)
//	{
//		slog_e("recv client connected msg, but client sid doesn't equal m_sid");
//		return;
//	}
//	slog_d("connected, svr_ip=%0, svr_port=%1", m_init_param.m_svr_ip_or_name.c_str(), m_init_param.m_svr_port);
//	m_is_connected = true;
//
//	ClientConnectedMsg* m = new ClientConnectedMsg();
//	m->m_msg_type = EMsgType_clientConnected;
//	m->m_client_sid = m_sid;
//	__postMessageToTarget(m);
//
//	__doSendPack();
//}
//
//void RawClientNetwork::__onMsgTcpSocketClientDisconnected(ITcpSocketCallbackApi::ClientSocketDisconnectedMsg * msg)
//{
//	if (msg->m_client_sid != m_sid)
//	{
//		slog_e("recv client disconnected msg, but client sid doesn't equal m_sid");
//		return;
//	}
//	slog_d("disconnected, svr_ip=%0, svr_port=%1", m_init_param.m_svr_ip_or_name.c_str(), m_init_param.m_svr_port);
//	m_is_connected = false;
//	m_is_sending = false;
//	m_recv_data.clear();
//
//	ClientDisconnectedMsg* m = new ClientDisconnectedMsg();
//	m->m_msg_type = EMsgType_clientDisconnected;
//	m->m_client_sid = m_sid;
//	__postMessageToTarget(m);
//}
//
//void RawClientNetwork::__onMsgTcpSocketClientSendDataEnd(ITcpSocketCallbackApi::ClientSocketSendDataEndMsg * msg)
//{
//	if (!m_is_connected)
//	{
//		slog_e("recv send data end msg when disconnected");
//		return;
//	}
//
//	SendPackEndMsg* m = new SendPackEndMsg();
//	m->m_msg_type = EMsgType_sendPackToSvrEnd;
//	m->m_send_pack = *m_send_packs[0];
//	__postMessageToTarget(m);
//	delete_and_erase_vector_element_by_index(&m_send_packs, 0);
//	m_is_sending = false;
//	__doSendPack();
//}
//
//void RawClientNetwork::__onMsgTcpSocketClientRecvData(ITcpSocketCallbackApi::ClientSocketRecvDataMsg * msg)
//{
//	if (!m_is_connected)
//	{
//		slog_e("recv recv data msg when disconnected");
//		return;
//	}
//
//	m_recv_data.append(msg->m_recv_data);
//	while (m_recv_data.getLen() > 0)
//	{
//		Pack pack;
//		size_t pack_len = 0;
//		EUnpackResultType r = m_packer->unpackFromBin(m_recv_data, &pack, &pack_len);
//		if (r == EUnpackResultType_ok)
//		{
//			m_recv_data.shrinkBegin(pack_len);
//			RecvPackMsg* m = new RecvPackMsg();
//			m->m_msg_type = EMsgType_recvPackFromSvr;
//			*((Pack*)&(m->m_recv_pack)) = pack;
//			m->m_recv_pack.m_id = ++m_pack_id_seed;
//			__postMessageToTarget(m);
//		}
//		else if (r == EUnpackResultType_fail)
//		{
//			m_init_param.m_sapi->stopClientSocket(m_sid);
//			slog_e("fail to unpack, stop client");
//			break;
//		}
//		else if (r == EUnpackResultType_needMoreData)
//		{
//			break;
//		}
//	}
//
//}
//
//void RawClientNetwork::__onMsgSendPack(Message * msg)
//{
//
//}
//
//void RawClientNetwork::__doSendPack()
//{
//	if (!m_is_connected)
//		return;
//	if (m_is_sending)
//		return;
//	if (m_send_packs.size() == 0)
//		return;
//
//	SendPack* send_pack = m_send_packs[0];
//	Binary bin;
//	m_packer->packToBin(*send_pack, &bin);
//	if (!m_init_param.m_sapi->sendDataFromClientSocketToSvr(m_sid, bin.getData(), bin.getLen()))
//		return;
//
//	m_is_sending = true;
//}
//
//void RawClientNetwork::__postMessageToSelf(Message * msg)
//{
//	msg->m_sender = this;
//	msg->m_target = this;
//	m_init_param.m_work_looper->postMessage(msg);
//}
//
//void RawClientNetwork::__postMessageToTarget(Message * msg)
//{
//	msg->m_sender = this;
//	msg->m_target = m_init_param.m_notify_target;
//	m_init_param.m_notify_looper->postMessage(msg);
//}
//
//int RawClientNetwork::__getSendPackIndexBySendPackId(uint64_t send_pack_id)
//{
//	for (size_t i = 0; i < m_send_packs.size(); ++i)
//	{
//		if (m_send_packs[i]->m_id == send_pack_id)
//			return (int)i;
//	}
//	return -1;
//}
//
//SCLIENT_NAMESPACE_END
