#ifndef S_SERVR_COMM_H_
#define S_SERVR_COMM_H_
#include "../comm/networkComm.h"
#include "../../socket/socketLib.h"
#include "../../util/stringUtil.h"
#include "../../log/log.h"
#include "../../util/timeUtil.h"


#ifndef SSVR_NAMESPACE_BEGIN
#define SSVR_NAMESPACE_BEGIN namespace S{
#define SSVR_NAMESPACE_END }
#define USING_NAMESPACE_SSVR using namespace S;
//#define SSVR_NAMESPACE_BEGIN namespace S{ namespace svr {
//#define SSVR_NAMESPACE_END }}
//#define USING_NAMESPACE_SSVR using namespace S::svr;
#endif

#define ServerSessionManager_MSG_BEGIN 815657
#define SESSION_HANDLER_MSG_BEGIN 886261





enum EServerCgiType
{
	EServerCgiType_c2sReq_s2cResp,		// client send req pack, and sever send resp pack. warn: no test
	EServerCgiType_c2sNotify,			// client send notify pack
	EServerCgiType_s2cReq_c2sResp,		// server send req pack, and client send resp pack
	EServerCgiType_s2cPush,				// svr send push pack
};





class ServerCgiInfo
{
public:
	ServerCgiInfo() { m_cgi_type = EServerCgiType_c2sReq_s2cResp; m_recv_cmd_type = 0; m_send_cmd_type = 0; }

	uint32_t getC2sReqCmdType() const
	{
		if (m_cgi_type != EServerCgiType_c2sReq_s2cResp)
			return 0;
		return m_recv_cmd_type;
	}

	uint32_t getS2cRespCmdType() const
	{
		if (m_cgi_type != EServerCgiType_c2sReq_s2cResp)
			return 0;
		return m_send_cmd_type;
	}

	uint32_t getS2cPushCmdType() const
	{
		if (m_cgi_type != EServerCgiType_s2cPush)
			return 0;
		return m_send_cmd_type;
	}

	uint32_t getS2cReqCmdType() const
	{
		if (m_cgi_type != EServerCgiType_s2cReq_c2sResp)
			return 0;
		return m_send_cmd_type;
	}

	uint32_t getC2sRespCmdType() const
	{
		if (m_cgi_type != EServerCgiType_s2cReq_c2sResp)
			return 0;
		return m_recv_cmd_type;
	}

	EServerCgiType m_cgi_type;
	uint32_t m_recv_cmd_type;
	uint32_t m_send_cmd_type;
};




#endif
