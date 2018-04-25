#include "testS.h"
#include "testFun.h"
#include "testBinary.h"
#include "testVariant.h"
#include "testFileUtil.h"
#include "testStringUtil.h"
#include "testP.h"
#include "testLog.h"
#include "testThread.h"
#include "testMisc.h"
#include "testSocket.h"
#include "testSocketBlockClientSpeed.h"
#include "testSocketBlockSvrSpeed.h"
#include "testSocketCallbackClient.h"
#include "testSocketCallbackClientSpeed.h"
#include "testSocketCallbackSvr.h"
#include "testSocketCallbackSvrSpeed.h"
#include "testDnsResolver.h"
#include "testClientNetwork.h"
#include "testServerNetwork.h"
//#include "testJson.h"
//#include "testSocketSvr.h"
//#include "testWin.h"
//#include "testLinux.h"
//#include "testMac.h"
//#include "../3rd/inc/openssl/aes.h"
#include "testSqlite.h"
using namespace std;
USING_NAMESPACE_S


int main(int argc, char** argv)
{
	printf("test start **************************************************************\n");
	//__testMsgLoopThread();
	//__testMsgLoopThreadPerformance();
	//__testDns();
	//__testDnsApi();
	__testDnsResolver();
	
	//__testBinary();
	//__testVarint();
	//__testStringUtil();
	//__testFileUtil();
	//__testLog();
	//__testThread();
	//__testMisc();
	//__testSqlite();
	//__ClientCgi_CheckVersion c;
	//__ClientSendPackBuilder b;
	//__testStPacker();
	//__testClientNetwork();

	printf("test end **************************************************************\n");
	return 0;
}




























//__tetAes();
//uint64_t t = TimeUtil::getMsTime();
//MyObj* obj = new MyObj();
//obj->addRef();
//obj->subRef();
//obj->subRef();


//
//class MyObj : public IRef
//{
//public:
//	MyObj()
//	{
//		m_ref_ptr.init(this);
//	}
//
//	ImplRef(MyObj);
//};
//
//MyObj::~MyObj() { printf("~MyObj()"); }
////#define MSG_LEN AES_BLOCK_SIZE * 2
//#define BUF_LEN AES_BLOCK_SIZE
//
//bool aes_encode_str_to_bin(const std::string& in_str, Binary* out_data, const std::string& key_str)
//{
//	AES_KEY aes_enc_key;
//	if (AES_set_encrypt_key((const unsigned char*)key_str.c_str(), 128, &aes_enc_key) < 0)
//	{
//		return false;
//	}
//
//	unsigned char iv[AES_BLOCK_SIZE];
//	memset(iv, 0x1, AES_BLOCK_SIZE);
//
//	size_t buf_len = (in_str.size() + AES_BLOCK_SIZE - 1) / AES_BLOCK_SIZE * AES_BLOCK_SIZE;
//	//buf_len = AES_BLOCK_SIZE * 2;
//	unsigned char* buf = new unsigned char[buf_len];
//	memset(buf, 'k', buf_len);
//
//	size_t str_len = in_str.size();
//	AES_cbc_encrypt((const unsigned char*)in_str.c_str(), buf, str_len, &aes_enc_key, iv, AES_ENCRYPT);
//	out_data->append(buf, buf_len);
//	delete[] buf;
//	return true;
//}
//
//bool aes_decode_bin_to_str(const Binary& in_data, std::string* out_str, const std::string& key_str)
//{
//	if (in_data.getLen() % AES_BLOCK_SIZE != 0)
//		return false;
//	AES_KEY aes_dec_key;
//	if (AES_set_decrypt_key((const unsigned char*)key_str.c_str(), 128, &aes_dec_key) < 0)
//	{
//		return false;
//	}
//
//	unsigned char iv[AES_BLOCK_SIZE];
//	memset(iv, 0x1, AES_BLOCK_SIZE);
//
//	size_t buf_len = in_data.getLen();
//	unsigned char* buf = new unsigned char[buf_len];
//	memset(buf, 'x', buf_len);
//	AES_cbc_encrypt(in_data.getData(), (unsigned char*)buf, buf_len, &aes_dec_key, iv, AES_DECRYPT);
//	*out_str += (const char*)buf;
//
//	delete[] buf;
//	return true;
//}
//
//void __tetAes()
//{
//
//	std::string key_str = "1234567812345678";
//	Binary bin;
//	if (!aes_encode_str_to_bin("1", &bin, key_str))
//		return;
//	std::string str;
//	aes_decode_bin_to_str(bin, &str, key_str);
//	cout << "ok" << endl;
//	//std::string key_str = "1234567812345678";
//	//unsigned char iv[AES_BLOCK_SIZE];
//
//	//unsigned char encoded_msg[BUF_LEN];
//	//{
//	//	memset(encoded_msg, 0, BUF_LEN);
//	//	memset(iv, 0, AES_BLOCK_SIZE);
//	//	AES_KEY aes_enc_key;
//	//	if (AES_set_encrypt_key((const unsigned char*)key_str.c_str(), 128, &aes_enc_key) < 0)
//	//	{
//	//		return;
//	//	}
//	//	std::string msg = "1234567812345678";
//	//	//for (int i = 0; i < 5; ++i)
//	//	//{
//	//	//	msg += "hello";
//	//	//}
//
//	//	AES_cbc_encrypt((const unsigned char*)msg.c_str(), encoded_msg, msg.size(), &aes_enc_key, iv, AES_ENCRYPT);
//	//}
//
//
//	//char decoded_msg[BUF_LEN];
//	//{
//	//	memset(decoded_msg, 0, BUF_LEN);
//	//	memset(iv, 0, AES_BLOCK_SIZE);
//	//	AES_KEY aesd_dec_key;
//	//	if (AES_set_decrypt_key((const unsigned char*)key_str.c_str(), 128, &aesd_dec_key) < 0)
//	//	{
//	//		return;
//	//	}
//
//	//	AES_cbc_encrypt(encoded_msg, (unsigned char*)decoded_msg, BUF_LEN, &aesd_dec_key, iv, AES_DECRYPT);
//	//}
//}

//
//class IUserStateCallback
//{
//public:
//	virtual ~IUserStateCallback() {}
//
//	virtual void onLogout() = 0;
//	virtual void onLogon(bool is_ok) = 0;
//};
//
////class MyCallbackHelper : public InvokeDispatcher<IMyCallback*>
////{
////public:
////	DECLARE_INVOKE_FUN_0(onLogout)
////	DECLARE_INVOKE_FUN_1(onLogon, const bool&, is_ok)
////};
//
//
//
//class UserStateCallbackToMsg : public CallbackToMsg<IUserStateCallback>
//{
//public:
//	virtual void onLogout()
//	{
//		Message* msg = new Message();
//		msg->m_msg_type = 1;
//		postMessage(msg);
//	}
//
//	virtual void onLogon(bool is_ok)
//	{
//		Message* msg = new Message();
//		msg->m_msg_type = 2;
//		msg->m_args.setInt8("is_ok", is_ok);
//		postMessage(msg);
//	}
//};
//
//class UserStateMsgToCallback : public MsgToCallback<IUserStateCallback>
//{
//public:
//	virtual void doCallback(Message * msg)
//	{
//		if (msg->m_msg_type == 1)
//		{
//			bool is_ok = msg->m_args.getInt8("is_ok");
//			getCallback()->onLogon(is_ok);
//		}
//
//		if (msg->m_msg_type == 2)
//		{
//			getCallback()->onLogout();
//		}
//	}
//};
//
//
//void __xxsdfkjsk()
//{
//
//	//MyCallbackHelper helper;
//	//helper.invoke_onLogon(true);
//	//helper.invoke_onLogout();
//
//	//LoginStateCallbackHolder h1(NULL, NULL);
//	//LoginStateCallbackHolder h2(NULL, NULL);
//	//CallbackThreadSwitcher switcher(NULL);
//	//switcher.registCallback(&h1);
//	//switcher.registCallback(&h2);
//	//h1.onLogon(true);
//
//	//School<Teacher> s1;
//	//School<Student> s2;
//	//s1.m_school_name = 1;
//	//s2.m_school_name = "hello";
//
//	//std::vector<int*> v;
//	//std::vector<int*>::iterator it = v.begin();
//
//	//delete_and_clear_collection(&v);
//
//
//	EventPoint<UserStateCallbackToMsg, UserStateMsgToCallback> event_point;
//	event_point.init(NULL, NULL);
//	event_point.registCallback(NULL);
//}
//
//
//template<typename T>
//class Person
//{
//public:
//	typedef T typeT;
//
//	T m_person_name;
//};
//
//class Student : public Person<std::string>
//{
//
//};
//
//
//class Teacher : public Person<int>
//{
//
//};
//
//template<typename Person>
//class School
//{
//public:
//	typename Person::typeT m_school_name;
//};

