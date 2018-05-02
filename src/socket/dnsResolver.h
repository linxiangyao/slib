#ifndef S_DNS_RESOLVER_H_
#define S_DNS_RESOLVER_H_
#include "socketApi.h"
S_NAMESPACE_BEGIN


/*
resovle ip text to ip.


NOTE:
	dns typical is a singleton

TODO: 
	1. change to getaddrinfo_a
	2. addNotifyLooper support target?
*/

class DnsResolver : public IMessageHandler
{
public:
	enum EMsgType
	{
		EMsgType_resolveEnd = 56164781,
	};

	class DnsRecord
	{
	public:
		std::string m_name;
		std::vector<Ip> m_ips;
	};

	class Msg_ResolveEnd : public Message
	{
	public:
		Msg_ResolveEnd(DnsResolver* resolver, bool is_ok, const DnsRecord& record)
		{
			m_msg_type = EMsgType_resolveEnd;
			m_sender = resolver;
			m_is_ok = is_ok;
			m_record = record;
		}

		bool m_is_ok;
		DnsRecord m_record;
	};
	

	DnsResolver();
	~DnsResolver();

	bool init(MessageLooper* env_loop = nullptr);
	void stop();
	bool addNotifyLooper(MessageLooper* notify_loop, void* notify_target);
	void removeNotifyLooper(MessageLooper* notify_loop, void* notify_target);
	bool getDnsRecordByName(const std::string& name, DnsRecord* record);
	bool startResolve(const std::string& name);






private:
	class __WorkRun;


	virtual void onMessage(Message * msg, bool * is_handled) override;


	void __stop();
	void __doResolve();
	void __notifyResolveEnd(bool is_ok, const DnsRecord& record);

	Mutex m_mutex;
	MessageLooper* m_env_loop;
	MessageLoopThread* m_env_thread;
	std::vector<Thread*> m_resolve_threads;
	std::map<std::string, DnsRecord> m_records;
	std::vector<std::string> m_to_resolve_names;
	MsgLoopNotifySet* m_notify_set;
};




S_NAMESPACE_END
#endif
