#ifndef S_MESSAGE_LOOPER_H_
#define S_MESSAGE_LOOPER_H_
#include "threadComm.h"
#include "messageType.h"
#include <list>
S_NAMESPACE_BEGIN





// thread safe
class MessageLooper
{
public:
    MessageLooper();
    ~MessageLooper();

    void loop();     // block current thread, wait message, and process message.
    void stopLoop(); // exit current thread loop. call by other thread, or by current thread(in message handeler).
    

    void addMsgHandler(IMessageHandler* msg_handler);
    void removeMsgHandler(IMessageHandler* msg_handler);
	void addMsgTimerHandler(IMessageTimerHandler* timer_handler);
	void removeMsgTimerHandler(IMessageTimerHandler* timer_handler);
    
    void postMessage(Message* msg);
	void postMessages(std::list<Message*>* msgs);
	void postMessage(Message* msg, uint32_t delay_ms);

    void removeMessageById(uint32_t msg_id);
    void removeMessage(const Message* msg);
    void removeMessagesByMsgType(int msg_type);							// msgType=-1 all message
	void removeMessagesBySender(void* sender);
	void removeMessagesByTarget(void* target);
    void removeMessagesBySenderAndMsgType(void* sender, int msg_type);  // msgType=-1 all message
	void removeMessagesByTargetAndMsgType(void* target, int msg_type);	// msgType=-1 all message of target
    
    bool hasMessage(const Message* msg);
    bool hasMessageByMsgType(int msgType);
    bool hasMessageBySenderAndMsgType(const void* sender, int msg_type);
    
	uint64_t createTimer(void* user_data);
	void releasseTimer(uint64_t timer_id);
	bool startTimer(uint64_t timer_id, uint32_t delay_ms, uint32_t circle_ms);
	void stopTimer(uint64_t timer_id);








private:
	class __Event;
	class __EventWorker;
	class __MsgHandler;
	typedef std::list<__Event*> __EventList;
	typedef std::list<__Event*>::iterator __EventListIt;
	class __Timer;
	typedef std::map<uint64_t, __Timer*> __IdToTimerMap;
	class __TimerHandler;
	class __TimerCallbackCtx;
	class __TimerWorker;


	void __onLoopWakeup();
	void __onTickAndGetNeedCallbackTimers(std::vector<__TimerCallbackCtx>* need_callback_ctxs);


	void __callbackOneEvent(__Event* e);
	void __callbackOneTimer(__TimerCallbackCtx* t);
	void __detachNeedCallbackEvents(__EventList* need_callback_events);
	int __findMsgHandlerIndex(const IMessageHandler* msgHandler);
	__EventListIt __findMsgIndex(__EventList& events, const Message* msg);
	__EventListIt __findMsgIndexById(__EventList& events, int msgId);
	__EventListIt __findRunnableIndex(__EventList& events, const IRun* r);
	void __removeEventByIndex(__EventList& events, __EventListIt it);
	int __getTimerHandlerIndexByTimerHandler(IMessageTimerHandler* handler);
	__Timer* __getTimerByTimerId(uint64_t timer_id);
	void __update_wait_time_out_ms();




    bool m_is_stop_loop;
    std::mutex m_mutex;
    std::condition_variable m_cond;
	int m_wait_time_out_ms;

	__EventList m_events;
	std::vector<__MsgHandler*> m_msg_handlers;

	uint64_t m_timer_id_seed;
	__IdToTimerMap m_id_to_timer_map;
	std::vector<__TimerHandler*> m_timer_handlers;
};



S_NAMESPACE_END
#endif
