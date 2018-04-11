#ifndef S_SERVER_CGI_H_
#define S_SERVER_CGI_H_
#include "serverNetwork.h"
SSVR_NAMESPACE_BEGIN






/*
example:


class ServerCgi_sendText : public ServerCgi
{
...
	virtual void setRecvPack(ServerNetwork::RecvPack* recv_pack)
	{
		ServerCgi::setRecvPack(recv_pack);
	     // parese c2sReq data
		 ...
		 m_is_c2s_req_ok = true;
		 m_c2s_req_text = (const char*)recv_pack->m_data.getData();
	}

	void initSendPack(ICallback* callback, bool is_ok)
	{
		setCallback(callback);

		ServerNetwork::SendPack* send_pack = new ServerNetwork::SendPack();
		std::string send_text = is_ok ? "ok" : "fail";
		send_pack->m_data.appen((const uint8_t*)send_text.c_str(), send_text.size() + 1);
		setSendPack(send_pack);
	}

	bool m_is_c2s_req_ok;
	std::string m_c2s_req_text;
}


...
// create cgi by recv_pack
ServerCgi_sendText* cgi = new ServerCgi_sendText();
cgi->setRecvPack(recv_pack.clone());

// process cgi
if(!cgi->m_is_c2s_req_ok)
{
	// cgi req is invalid
	return;
}
saveToDb(cgi->m_c2s_req_text);

// send resp to cgi
cgi->initSendPack(true);
serverSessionMgr->startCgi(cgi);
...

*/

class ServerCgi
{
public:
	typedef ServerNetwork::EErrCode EErrCode;
	typedef ServerNetwork::SendPack SendPack;
	typedef ServerNetwork::RecvPack RecvPack;

	class ICallback
	{
	public:
		virtual ~ICallback() {};
		virtual void onServerCgi_cgiDone(ServerCgi* cgi) = 0;
	};






	ServerCgi();
	virtual ~ServerCgi();

	virtual const ServerCgiInfo& getServerCgiInfo() const = 0;
	

	RecvPack*	getRecvPack() { return m_recv_pack; }
	SendPack*	getSendPack() { return m_send_pack; }
	ICallback*	getCallback() { return m_callback; }
	EErrCode	getErrCode() { return m_err_code; }
	uint64_t	getStartMs() { return m_start_ms; }
	uint64_t	getEndMs() { return m_end_ms; }
	bool		getIsCgiSuccess() { return m_err_code == ServerNetwork::EErrCode_ok; }
	session_id_t getSessionId();
	
	void setCallback(ICallback* callback) { m_callback = callback; }
	void setErrCode(EErrCode err_code) { m_err_code = err_code; }
	void setStartMs(uint64_t ms) { m_start_ms = ms; }
	void setEndMs(uint64_t ms) { m_end_ms = ms; }
	void setSendPack(SendPack* send_pack) { __setSendPack(send_pack); }
	void setRecvPack(RecvPack* recv_pack) { m_recv_pack = recv_pack; onSetRecvPackEnd(); }



protected:
	virtual void onSetRecvPackEnd() {}








private:
	void __setSendPack(SendPack* send_pack);


	ICallback* m_callback;
	SendPack* m_send_pack;
	RecvPack* m_recv_pack;
	ServerNetwork::EErrCode m_err_code;
	uint64_t m_start_ms;
	uint64_t m_end_ms;
};



SSVR_NAMESPACE_END
#endif


