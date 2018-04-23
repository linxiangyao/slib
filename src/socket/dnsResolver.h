#ifndef S_DNS_RESOLVER_H_
#define S_DNS_RESOLVER_H_
#include "socketApi.h"
S_NAMESPACE_BEGIN



class DnsResolver : public IMessageHandler
{
public:
	class DnsRecord
	{
	public:
		std::string m_name;
		std::vector<Ip> m_ips;
	};

	class ICallback
	{
	public:
		virtual ~ICallback() {}
		virtual void onDnsResolver_resolveEnd(DnsResolver* resolver, bool is_ok, const DnsRecord& record) = 0;
	};




	DnsResolver();
	~DnsResolver();

	bool init(MessageLooper* env_loop, ICallback* callback);
	bool getIpByName(const std::string& name, DnsRecord* record);
	bool startResolve(const std::string& name);






private:
	class __WorkRun;


	virtual void onMessage(Message * msg, bool * is_handled) override;


	void __doResolve();
	bool __strToIp(const std::string& str, in_addr* ip_v4);
	bool __strToIp(const std::string& str, in6_addr* ip_v6);


	ICallback* m_callback;
	MessageLooper* m_env_loop;
	std::vector<Thread*> m_threads;
	std::map<std::string, DnsRecord> m_records;
	std::vector<std::string> m_to_resolve_names;
};




S_NAMESPACE_END
#endif
