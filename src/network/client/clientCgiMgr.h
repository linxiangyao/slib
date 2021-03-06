#ifndef S_CLIENT_CGI_MGR_H_
#define S_CLIENT_CGI_MGR_H_
#include "clientNetwork.h"
#include "clientCgi.h"
SCLIENT_NAMESPACE_BEGIN



/*

client cgi manager.
start/stop client cgi, and call back when cgi end.


NOTE: 
	mgr run in message looper and will callback in the same looper.
	so there is no lock.

*/
class ClientCgiMgr : public ClientNetwork::ICallback
{
public:
	typedef ClientNetwork::SendPack SendPack;
	typedef ClientNetwork::RecvPack RecvPack;

	class ICallback
	{
	public:
		virtual ~ICallback() {}
		virtual void onClientCgiMgr_recvS2cPushPack(std::unique_ptr<RecvPack>* recv_pack) = 0;
		virtual void onClientCgiMgr_recvS2cReqPack(std::unique_ptr<RecvPack>* recv_pack) = 0;
	};

	class InitParam
	{
	public:
		InitParam() { m_callback = NULL; m_network = NULL;  }
		ICallback* m_callback;
		ClientNetwork* m_network;
		std::vector<ClientCgiInfo> m_cgi_infos;
	};



	ClientCgiMgr();
	~ClientCgiMgr();

	bool init(const InitParam& param);
	bool startCgi(ClientCgi* cgi);
	void stopCgi(ClientCgi* cgi);
	



private:
	virtual void onClientNetworkStatred(ClientNetwork* network) override;
	virtual void onClientNetworkStopped(ClientNetwork* network) override;
	virtual void onClientNetworkConnectStateChanged(ClientNetwork* network, ClientNetwork::EConnectState state) override;
	virtual void onClientNetworkSendPackEnd(ClientNetwork* network, ClientNetwork::EErrType err_type, int err_code, uint64_t send_pack_id, std::unique_ptr<RecvPack>* recv_pack) override;
	virtual void onClientNetworkRecvPack(ClientNetwork* network, std::unique_ptr<RecvPack>* recv_pack) override;


	int __getCgiIndexBySendPackId(uint64_t send_pack_id);
	int __getCgiInfoIndexByRecvPackCmdType(uint32_t recv_pack_cmd_type);


	std::vector<ClientCgi*> m_cgis;
	InitParam m_init_param;
};




SCLIENT_NAMESPACE_END
#endif

