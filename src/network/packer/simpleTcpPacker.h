#ifndef S_SIMPLE_TCP_PACKER_H_
#define S_SIMPLE_TCP_PACKER_H_
#include "../client/clientNetwork.h"
#include "../server/serverNetwork.h"
S_NAMESPACE_BEGIN


/*
send pack & recv pack are the same

class __RawPackHead
{
public:
	uint32_t m_magic_num;
	uint16_t m_app_id;
	uint32_t m_ver;
	uint16_t m_head_len;
	uint32_t m_total_len;
	uint32_t m_uin;
	byte_t m_session_id[16];
	uint32_t m_cmd_type;
	uint32_t m_seq;
	uint8_t m_err;
	uint8_t m_encrypt_method;
	uint8_t m_compress_method;
};



unpackClientRecvPack/unpackServerRecvPack:
	if the magic_num in raw data is not equal to SimpleTcpPacker.m_magic_num, fun will return EUnpackResultType_fail. app_id, ver are the same.
*/

class SimpleTcpPacker : public ClientNetwork::IUnpacker, public ServerNetwork::IUnpacker
{
public:
	class PackHead
	{
	public:
		PackHead() { m_uin = 0; m_cmd_type = 0; m_seq = 0; m_err = 0; memset(m_session_id_bin, 0, 16); }

		uint32_t m_uin;
		uint32_t m_cmd_type;
		uint32_t m_seq;
		byte_t m_session_id_bin[16];
		uint8_t m_err;
	};

	class Pack : public ClientNetwork::IRecvPackExt, public ServerNetwork::IRecvPackExt
	{
	public:
		PackHead m_head;
		Binary m_body;
	};





	SimpleTcpPacker(uint32_t magic_num = (uint32_t)0x12295238, uint16_t app_id = 1, uint16_t ver = 1);
	~SimpleTcpPacker();

	bool packToBin(const Pack& pack, Binary* bin);
	virtual void unpackClientRecvPack(const byte_t* raw_data, size_t raw_data_len, ClientNetwork::UnpackResult* result) override;
	virtual void unpackServerRecvPack(const byte_t* raw_data, size_t raw_data_len, ServerNetwork::UnpackResult* result) override;









private:
	EUnpackResultType __unpackFromBin(const byte_t* raw_data, size_t raw_data_len, size_t* unpack_raw_data_len, Pack* pack);

	uint32_t m_magic_num;
	uint16_t m_app_id;
	uint16_t m_ver;
	Mutex m_mutex;
};

typedef SimpleTcpPacker StPacker;













S_NAMESPACE_END
#endif


//bool packClientSendPack(ClientNetwork::SendPack* send_pack, const byte_t* send_pack_body, size_t send_pack_body_len);
//bool packServerSendPack(ServerNetwork::SendPack* send_pack, const byte_t* send_pack_body, size_t send_pack_body_len);

