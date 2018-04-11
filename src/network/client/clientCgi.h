#ifndef S_CLIENT_CGI_H_
#define S_CLIENT_CGI_H_
#include "clientNetwork.h"
SCLIENT_NAMESPACE_BEGIN



/*
ClientCgi of client is a no status network sevice, there are 2 cases: 
1. client send a req pack, then server send a resp pack.
2. client send a notify pack to sever, and sever don't need to send a pack to client.
 
example, user send text from client to svr:
class ClientCgi_SendTextMessage : pubilc ClientCgi
{
public:
    bool init(const std::string& text, const std::string& to_user, ClientCgi::ICallback* callback)
    {
        // set send_pack
        SendPack* send_pack = new SendPack();
        send_pack->m_cmd_type = 1; 
		send_pack->m_data.append((byte_t*)text.c_str(), text.size()); 
		send_pack->m_data.append(...); 
		...
		setSendPack(send_pack);

		// set callback
		setCallback(callback);
    }

	static const ClientCgiInfo & s_getCgiInfo()
	{
		static bool is_init = false;
		if(is_init)
			return;
		is_init = true;
		s_cgi_info.m_send_cmd_type = 1;
		s_cgi_info.m_recv_cmd_type = 2;
		s_cgi_info.m_network_types = EClientCgiNetworkType_tcp;
		s_cgi_info.m_is_need_auth_session = false;
		return s_cgi_info;
	}

	virtual const ClientCgiInfo & getCgiInfo() const
	{
		return s_getCgiInfo();
	}
	
	// parse recv pack
	virtual void onSetRecvPackEnd()
	{
		if(getRecvPack() == NULL)
			return;

		m_s2c_resp_str = (const char*)getRecvPack()->m_data.getData();
		if(recv_str != "ok i am svr")
			return;
		m_s2c_resp_is_ok = true;
	}

	static ClientCgiInfo s_cgi_info;
	std::string m_s2c_resp_str;
	bool m_s2c_resp_is_ok;
}


class SendTextMessageLogic : public ClientCgi::ICallback
{
	...

	bool startSendTextToSvr()
	{
		if(m_send_text_cgi != null)
			return;
		m_send_text_cgi = new ClientCgi_SendTextMessage("hello", "ryan", this);
		return clientCgiMgr.start(cgi);			
	}

	virtual void onClientCgi_cgiDone(ClientCgi* cgi)
	{
		if(cgi == m_send_text_cgi)
		{
			if(!cgi->getIsSuccess())
				return;
			if(!cgi->m_s2c_resp_is_ok)
				return;
			// send text ok, do things such as save to db ...
		}
	}

	ClientCgi_SendTextMessage* m_send_text_cgi;
}


cgi run in message looper, and will callback in the same message looper.
you can run network in one thread(e.g. network componet thread), and run cgi in another thread(e.g. logic thread).

*/
class ClientCgi
{
public:
	typedef ClientNetwork::SendPack SendPack;
	typedef ClientNetwork::RecvPack RecvPack;
	typedef ClientNetwork::EErrType EErrType;

	class ICallback
	{
	public:
		virtual ~ICallback() {}
		virtual void onClientCgi_cgiDone(ClientCgi* cgi) = 0;
	};






	ClientCgi() { m_callback = NULL; m_send_pack = NULL; m_recv_pack = NULL; m_err_type = ClientNetwork::EErrType_ok; m_err_code = 0; m_start_ms = 0; m_end_ms = 0; }
	virtual ~ClientCgi() { delete m_send_pack; delete m_recv_pack; }

	virtual const ClientCgiInfo& getCgiInfo() const = 0;



	RecvPack*	getRecvPack() { return m_recv_pack; }
	SendPack*	getSendPack() { return m_send_pack; }
	ICallback*	getCallback() { return m_callback; }
	EErrType	getErrType() { return m_err_type; }
	int			getErrCode() { return m_err_code; }
	bool		getIsSuccess() { return getErrType() == ClientNetwork::EErrType_ok && getErrCode() == 0; }
	uint64_t	getStartMs() { return m_start_ms; }
	uint64_t	getEndMs() { return m_end_ms; }


	void		setSendPack(SendPack* send_pack) { m_send_pack = send_pack; }
	void		setRecvPack(RecvPack* recv_pack) { m_recv_pack = recv_pack; onSetRecvPackEnd(); }
	void		setCallback(ICallback* callback) { m_callback = callback; }
	void		setErrType(EErrType err_type) { m_err_code = err_type; }
	void		setErrCode(int err_code) { m_err_code = err_code; }
	void		setStartMs(uint64_t ms) { m_start_ms = ms; }
	void		setEndMs(uint64_t ms) { m_end_ms = ms; }



protected:
	virtual void onSetRecvPackEnd() {}



private:
	ICallback* m_callback;
	SendPack* m_send_pack;
	RecvPack* m_recv_pack;
	bool m_is_keep_send_data_after_sent;
	EErrType m_err_type;
	int m_err_code;
	uint64_t m_start_ms;
	uint64_t m_end_ms;
};




SCLIENT_NAMESPACE_END
#endif
