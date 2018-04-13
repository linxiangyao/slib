#ifndef S_SERVER_CGI_H_
#define S_SERVER_CGI_H_
#include "serverNetwork.h"
SSVR_NAMESPACE_BEGIN



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

	// need implement
	virtual const ServerCgiInfo& getServerCgiInfo() const = 0;

	RecvPack*	getRecvPack();
	SendPack*	getSendPack();
	ICallback*	getCallback();
	EErrCode	getErrCode();
	uint64_t	getStartMs();
	uint64_t	getEndMs();
	bool		getIsCgiSuccess();
	session_id_t getSessionId();
	
	void setCallback(ICallback* callback);
	void setErrCode(EErrCode err_code);
	void setStartMs(uint64_t ms);
	void setEndMs(uint64_t ms);
	void setSendPack(SendPack* send_pack);
	void setRecvPack(RecvPack* recv_pack);


protected:
	// override
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


