#include "simpleTcpPacker.h"
//#include "../../../3rd/enc/openssl/openssl/md5.h"
S_NAMESPACE_BEGIN
static uint16_t __TCP_PACK_HEAD_LEN = 47;




SimpleTcpPacker::SimpleTcpPacker(uint32_t magic_num, uint16_t app_id, uint16_t ver)
{
	m_magic_num = magic_num;
	m_app_id = app_id;
	m_ver = ver;
}

SimpleTcpPacker::~SimpleTcpPacker()
{

}

bool SimpleTcpPacker::packToBin(const Pack& pack, Binary* bin)
{
	if (bin == NULL)
		return false;

	Binary head;
	head.append((uint32_t)m_magic_num);
	head.append(SocketUtil::hToNs(m_app_id));
	head.append(SocketUtil::hToNl(m_ver));
	head.append(SocketUtil::hToNs(__TCP_PACK_HEAD_LEN));
	head.append(SocketUtil::hToNl((uint32_t)(pack.m_body.getLen() + __TCP_PACK_HEAD_LEN)));
	head.append(SocketUtil::hToNl(pack.m_head.m_uin));
	head.append(pack.m_head.m_session_id_bin, 16);
	head.append(SocketUtil::hToNl(pack.m_head.m_cmd_type));
	head.append(SocketUtil::hToNl(pack.m_head.m_seq));
	head.append(pack.m_head.m_err);
	head.append((uint8_t)0);
	head.append((uint8_t)0);

	bin->append(head);
	bin->append(pack.m_body);

	return true;
}

void SimpleTcpPacker::unpackClientRecvPack(const byte_t* raw_data, size_t raw_data_len, ClientNetwork::UnpackResult* result)
{
	ScopeMutex _l(m_mutex);
	Pack* pack = new Pack();
	size_t unpack_raw_data_len = 0;
	EUnpackResultType result_type = __unpackFromBin(raw_data, raw_data_len, &unpack_raw_data_len, pack);
	result->m_result_type = result_type;
	if (result_type == EUnpackResultType_ok)
	{
		result->m_recv_cmd_type = pack->m_head.m_cmd_type;
		result->m_recv_seq = pack->m_head.m_seq;
		result->m_unpack_raw_data_len = unpack_raw_data_len;
		result->m_recv_ext = pack;
	}
	else
	{
		delete pack;
	}
}

void SimpleTcpPacker::unpackServerRecvPack(const byte_t* raw_data, size_t raw_data_len, ServerNetwork::UnpackResult* result)
{
	ScopeMutex _l(m_mutex);
	Pack* pack = new Pack();
	size_t unpack_raw_data_len = 0;
	EUnpackResultType result_type = __unpackFromBin(raw_data, raw_data_len, &unpack_raw_data_len, pack);
	result->m_result_type = result_type;
	if (result_type == EUnpackResultType_ok)
	{
		result->m_recv_cmd_type = pack->m_head.m_cmd_type;
		result->m_recv_seq = pack->m_head.m_seq;
		result->m_unpack_raw_data_len = unpack_raw_data_len;
		result->m_recv_ext = pack;
		memcpy(result->m_ssid.m_data, pack->m_head.m_session_id_bin, 16);
	}
	else
	{
		delete pack;
	}
}












EUnpackResultType SimpleTcpPacker::__unpackFromBin(const byte_t* raw_data, size_t raw_data_len, size_t* unpack_raw_data_len, Pack* pack)
{
	if (raw_data == NULL || unpack_raw_data_len == NULL || pack == NULL)
		return EUnpackResultType_fail;
	if (raw_data_len < __TCP_PACK_HEAD_LEN)
		return EUnpackResultType_needMoreData;

	BinaryReader head;
	head.attach(raw_data, raw_data_len);

	uint32_t magic_num = *(uint32_t*)head.read(4);
	if (magic_num != m_magic_num)
		return EUnpackResultType_fail;

	uint16_t app_id = SocketUtil::nToHs(*(uint16_t*)head.read(2));
	if (app_id != m_app_id)
		return EUnpackResultType_fail;

	uint32_t ver = SocketUtil::nToHl(*(uint32_t*)head.read(4));
	if (ver != m_ver)
		return EUnpackResultType_fail;

	SocketUtil::nToHs(*(uint16_t*)head.read(2)); // uint16_t head_len =
	uint32_t total_len = SocketUtil::nToHl(*(uint32_t*)head.read(4));
	if (raw_data_len < total_len)
		return EUnpackResultType_needMoreData;

	uint32_t uin = SocketUtil::nToHl(*(uint32_t*)head.read(4));
	byte_t session_id[16];
	memcpy(session_id, head.read(16), 16);
	uint32_t cmd_type = SocketUtil::nToHl(*(uint32_t*)head.read(4));
	uint32_t seq = SocketUtil::nToHl(*(uint32_t*)head.read(4));
	uint8_t err = *(uint8_t*)head.read(1);
//	uint8_t encrypt_method = *(uint8_t*)
    head.read(1);
//	uint8_t compress_method = *(uint8_t*)
    head.read(1);


	//pack->m_head.m_magic_num = magic_num;
	//pack->m_head.m_app_id = app_id;
	//pack->m_head.m_ver = ver;
	pack->m_head.m_uin = uin;
	memcpy(pack->m_head.m_session_id_bin, session_id, 16);
	pack->m_head.m_cmd_type = cmd_type;
	pack->m_head.m_seq = seq;
	pack->m_head.m_err = err;

	const byte_t* body = raw_data + __TCP_PACK_HEAD_LEN;
	size_t body_len = total_len - __TCP_PACK_HEAD_LEN;
	pack->m_body.append(body, body_len);

	*unpack_raw_data_len = total_len;
	return EUnpackResultType_ok;
}








S_NAMESPACE_END
