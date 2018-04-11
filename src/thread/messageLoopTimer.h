//#ifndef S_MESSAGE_LOOP_TIMER_H_
//#define S_MESSAGE_LOOP_TIMER_H_
//#include "messageLooper.h"
//S_NAMESPACE_BEGIN
//
//
//
//
//class MessageLoopTimer : public IMessageTimerHandler
//{
//public:
//	class ICallback
//	{
//	public:
//		virtual ~ICallback() {}
//		virtual void onMessageLoopTimerTick(uint64_t timer_id, void* user_data) = 0;
//	};
//
//	MessageLoopTimer();
//    ~MessageLoopTimer();
//
//	bool init(ICallback* callback, void* user_data, MessageLooper* looper);
//    bool start(uint32_t delay_ms, uint32_t circle_ms = 0);
//    void stop();
//
//	uint64_t getTimerId();
//
//
//private:
//	virtual void onMessageTimerTick(uint64_t timer_id, void* user_data);
//
//
//	//bool m_is_running;
//	uint64_t m_id;
//	ICallback* m_callback;
//    MessageLooper* m_looper;
//};
//
//
//
//S_NAMESPACE_END
//#endif
