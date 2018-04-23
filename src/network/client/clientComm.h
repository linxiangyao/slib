#ifndef S_CLIENT_COMM_H_
#define S_CLIENT_COMM_H_
#include "../comm/networkComm.h"
#include "../../util/stringUtil.h"
#include "../../util/timeUtil.h"

#ifndef SCLIENT_NAMESPACE_BEGIN
#define SCLIENT_NAMESPACE_BEGIN namespace S{ 
#define SCLIENT_NAMESPACE_END }
#define USING_NAMESPACE_SCLIENT using namespace S;
//#define SCLIENT_NAMESPACE_BEGIN namespace S{ namespace client {
//#define SCLIENT_NAMESPACE_END }}
//#define USING_NAMESPACE_SCLIENT using namespace S::client;
#endif

SCLIENT_NAMESPACE_BEGIN





enum EClientCgiPriority
{
	EClientCgiPriority_lowest = 0,
	EClientCgiPriority_log = 10,
	EClientCgiPriority_statistics = 20,
	EClientCgiPriority_downloadNormalRes = 30,
	EClientCgiPriority_downloadUserRes = 50,
	EClientCgiPriority_downloadUserHeadProtrait = 58,
	EClientCgiPriority_voip = 70,
	EClientCgiPriority_signaling = 80,		// normal cgi is singnaling
	EClientCgiPriority_auth = 88,			// note: auth must be the higher priority than all the cgi which need auth session
	EClientCgiPriority_highest = 100,
};


enum EClientCgiType
{
	EClientCgiType_c2sReq_s2cResp,	// client send req and server send resp pack
	EClientCgiType_s2cReq_cs2Resp,	// server send req pack, and client send resp pack
	EClientCgiType_c2sNotify,		// client send notify pack
	EClientCgiType_s2cPush,			// svr send notify pack
};



class ClientCgiInfo
{
public:
	ClientCgiInfo()
	{ 
		m_cgi_type = EClientCgiType_c2sReq_s2cResp;
		m_send_cmd_type = 0;
		m_recv_cmd_type = 0;
		m_priority = EClientCgiPriority_signaling;
		m_max_pack_count_in_queue = 100;
		//m_network_types = EClientCgiNetworkType_tcp | EClientCgiNetworkType_http;
		m_cgi_time_out_ms = 60 * 1000;
	}
	
	EClientCgiType m_cgi_type;
	uint32_t m_send_cmd_type;
	uint32_t m_recv_cmd_type;		// if need recv pack, set recv cmd type.
	uint8_t m_priority;				// 0 - 100, see EClientCgiPriority for example, you can define your self priority.
	//int m_network_types;
	uint32_t m_max_pack_count_in_queue;
	uint32_t m_cgi_time_out_ms;
};



SCLIENT_NAMESPACE_END
#endif
