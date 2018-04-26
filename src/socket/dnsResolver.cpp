#include "dnsResolver.h"
S_NAMESPACE_BEGIN


class DnsResolver::__WorkRun : public IThreadRun
{
public:
#define MSG_TYPE_DnsResolver__WorkRun__resolveEnd 53316731
	class GetAddrinfoEndMessage : public Message
	{
	public:
		GetAddrinfoEndMessage(void* sender, void* target, bool is_ok, const DnsRecord& record)
		{
			m_msg_type = MSG_TYPE_DnsResolver__WorkRun__resolveEnd;
			m_is_ok = is_ok;
			m_record = record;
			m_sender = sender;
			m_target = target;
		}

		bool m_is_ok;
		DnsRecord m_record;
	};

	__WorkRun(MessageLooper* notify_looper, DnsResolver* notify_target, const std::string& name)
	{
		m_notify_looper = notify_looper;
		m_notify_target = notify_target;
		m_name = name;
	}



private:
	virtual void run() override
	{
		DnsRecord record;
		record.m_name = m_name;

		addrinfo addr_hint;
		memset(&addr_hint, 0, sizeof(addr_hint));
		addr_hint.ai_family = AF_INET;
		addr_hint.ai_socktype = SOCK_STREAM;

		addrinfo* addr_result = NULL;
		if (getaddrinfo(m_name.c_str(), NULL, &addr_hint, &addr_result) != 0)
		{
			slog_e("DnsResolver::__WorkRun::run fail to getaddrinfo %0", m_name);
			__notify(false, record);
			return;
		}

		for (addrinfo* a = addr_result; a != NULL; a = a->ai_next)
		{
			if (a->ai_family == AF_INET) 
			{
				in_addr ip_v4 = ((sockaddr_in*)a->ai_addr)->sin_addr;
				record.m_ips.push_back(Ip(ip_v4));

				char ip_str[100];
				if (inet_ntop(AF_INET, &ip_v4, ip_str, sizeof(ip_str)) != nullptr)
					slog_d("resolve %0 ip=%1", m_name, ip_str);
			}
			else if (a->ai_family == AF_INET6)
			{
				in6_addr ip_v6 = ((sockaddr_in6*)a->ai_addr)->sin6_addr;
				record.m_ips.push_back(Ip(ip_v6));

				char ip_str[100];
				if (inet_ntop(AF_INET6, &ip_v6, ip_str, sizeof(ip_str)) != nullptr)
					slog_d("resolve %0 ip=%1", m_name, ip_str);
			}
		}

		__notify(true, record);
	}

	virtual void stop() override
	{
	}

	void __notify(bool is_ok, const DnsRecord& r)
	{
		GetAddrinfoEndMessage* msg = new GetAddrinfoEndMessage(this, m_notify_target, is_ok, r);
		m_notify_looper->postMessage(msg);
	}

	std::string m_name;
	MessageLooper* m_notify_looper;
	DnsResolver* m_notify_target;
};





DnsResolver::DnsResolver()
{
	slog_d("new DnsResolver=%0", (uint64_t)this);
	m_env_loop = nullptr;
	m_env_thread = nullptr;
}

DnsResolver::~DnsResolver() 
{
	slog_d("delete DnsResolver=%0", (uint64_t)this);
	ScopeMutex _l(m_mutex);
	m_env_loop->removeMsgHandler(this);

	delete_and_erase_collection_elements(&m_resolve_threads);
	delete m_env_thread;
}

bool DnsResolver::init(MessageLooper * work_loop) 
{
	slog_d("DnsResolver init");
	ScopeMutex _l(m_mutex);
	if (work_loop == nullptr)
	{
		m_env_thread = new MessageLoopThread(nullptr, false);
		if (!m_env_thread->start())
		{
			slog_e("DnsResolver::init fail to m_env_thread->start");
			return false;
		}
			
		m_env_loop = m_env_thread->getLooper();
	}
	else
	{
		m_env_loop = work_loop;
	}
	m_env_loop->addMsgHandler(this);
	return true;
}

void DnsResolver::stop()
{
	slog_d("DnsResolver stop");
	ScopeMutex _l(m_mutex);
	delete_and_erase_collection_elements(&m_resolve_threads);
	m_env_loop->removeMessagesBySender(this);
}

bool DnsResolver::addNotifyLooper(MessageLooper * notify_loop)
{
	ScopeMutex _l(m_mutex);
	if (is_vector_contain_element(m_notify_loops, notify_loop))
		return true;
	m_notify_loops.push_back(notify_loop);
	return false;
}

void DnsResolver::removeNotifyLooper(MessageLooper * notify_loop)
{
	ScopeMutex _l(m_mutex);
	erase_vector_element_by_value(&m_notify_loops, notify_loop);
}

bool DnsResolver::getIpByName(const std::string& name, DnsRecord* record)
{
	ScopeMutex _l(m_mutex);
	auto it = m_records.find(name);
	if (it == m_records.end())
	{
		in_addr ip_v4;
		if (SocketUtil::strToIpV4(name, &ip_v4))
		{
			record->m_ips.push_back(Ip(ip_v4));
			return true;
		}

		in6_addr ip_v6;
		if(SocketUtil::strToIpV6(name, &ip_v6))
		{
			record->m_ips.push_back(Ip(ip_v6));
			return true;
		}

		return false;
	}

	*record = it->second;
	return true;
}

bool DnsResolver::startResolve(const std::string & name)
{
	ScopeMutex _l(m_mutex);
	m_records.erase(name);

	m_to_resolve_names.push_back(name);
	__doResolve();
	return true;
}

void DnsResolver::onMessage(Message * msg, bool * is_handled)
{
	ScopeMutex _l(m_mutex);
	if (msg->m_target == this && msg->m_msg_type == MSG_TYPE_DnsResolver__WorkRun__resolveEnd)
	{
		*is_handled = true;

		__WorkRun::GetAddrinfoEndMessage* m = (__WorkRun::GetAddrinfoEndMessage*)msg;
		if (m->m_is_ok)
		{
			m_records[m->m_record.m_name] = m->m_record;
		}
		__notifyResolveEnd(m->m_is_ok, m->m_record);

		for (size_t i = 0; i < m_resolve_threads.size(); ++i)
		{
			if (m_resolve_threads[i]->getRun() == m->m_sender)
			{
				delete_and_erase_vector_element_by_index(&m_resolve_threads, (int)i);
				break;
			}
		}

		__doResolve();
	}
}

void DnsResolver::__doResolve()
{
	if (m_to_resolve_names.size() == 0)
		return;
	if (m_resolve_threads.size() > 5)
		return;

	std::string name = m_to_resolve_names[0];
	m_to_resolve_names.erase(m_to_resolve_names.begin() + 0);

	Thread* t = new Thread(new __WorkRun(m_env_loop, this, name));
	m_resolve_threads.push_back(t);
	t->start();
}

void DnsResolver::__notifyResolveEnd(bool is_ok, const DnsRecord & record)
{
	for (size_t i = 0; i < m_notify_loops.size(); ++i)
	{
		Msg_ResolveEnd* msg = new Msg_ResolveEnd(this, is_ok, record);
		m_notify_loops[i]->postMessage(msg);
	}
}




S_NAMESPACE_END
