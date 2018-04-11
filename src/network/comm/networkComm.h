#ifndef S_NETWORK_COMM_H_
#define S_NETWORK_COMM_H_
#include <vector>
#include <stdio.h>
#include "../../socket/socketLib.h"
S_NAMESPACE_BEGIN

enum EUnpackResultType
{
	EUnpackResultType_ok,
	EUnpackResultType_fail,
	EUnpackResultType_needMoreData,
};


//
//
//
//class Pack
//{
//public:
//	class SessionId
//	{
//	public:
//		SessionId() { memset(m_data, 0, 16); }
//
//		bool isEmpty() const
//		{
//			for (size_t i = 0; i < 16; ++i)
//			{
//				if (m_data[i] != 0)
//					return false;
//			}
//			return true;
//		}
//
//		std::string toString() const
//		{
//			return StringUtil::toString(m_data, 16);
//		}
//
//		byte_t m_data[16];
//	};
//
//	Pack() { m_id = 0; m_app_id = 0; m_ver = 0; m_uin = 0; m_seq = 0; m_cmd_type = 0; m_err = 0; }
//
//	uint64_t m_id;
//	uint16_t m_app_id;
//	uint32_t m_ver;
//	uint32_t m_uin;
//	SessionId m_session_id;
//	uint32_t m_cmd_type;
//	uint32_t m_seq;
//	uint8_t m_err;
//	Binary m_data;
//};
//
//bool operator < (const Pack::SessionId& l, const Pack::SessionId& r);
//bool operator == (const Pack::SessionId& l, const Pack::SessionId& r);
//
//
//enum EUnpackResultType
//{
//	EUnpackResultType_ok,
//	EUnpackResultType_fail,
//	EUnpackResultType_needMoreData,
//};
//
//
//class IPacker
//{
//public:
//	virtual ~IPacker() {}
//	virtual bool packToBin(const Pack& pack, Binary* bin) = 0;
//	virtual EUnpackResultType unpackFromBin(const Binary& bin, Pack* pack, size_t* pack_data_len) = 0;
//};
//
//
//
//
//
//
//
//
//
//enum EEncryptMethod
//{
//	EEncryptMethod_none = 0,
//	EEncryptMethod_aes = 1,
//};
//enum ECompressMethod
//{
//	ECompressMethod_none = 0,
//	ECompressMethod_gzip = 1,
//};
//
//
//// a simple tcp packer
//class SimpleTcpPacker : public IPacker
//{
//public:
//    SimpleTcpPacker(EEncryptMethod encrypt_method = EEncryptMethod_none, ECompressMethod compress_method = ECompressMethod_none);
//    ~SimpleTcpPacker();
//    
//    virtual bool packToBin(const Pack& pack, Binary* bin);
//    virtual EUnpackResultType unpackFromBin(const Binary& bin, Pack* pack, size_t* pack_data_len);
//    
//    
//private:
//    EEncryptMethod m_encrypt_method;
//    ECompressMethod m_compress_method;
//};
//
//
//
//
//
//// plain text, that means not header, no encrypt, no compress
//class SimplePlainPacker : public IPacker
//{
//public:
//    virtual bool packToBin(const Pack& pack, Binary* bin);
//    virtual EUnpackResultType unpackFromBin(const Binary& bin, Pack* pack, size_t* pack_data_len);
//};
//
//
//
//
//
//
S_NAMESPACE_END
#endif
