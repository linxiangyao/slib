//#include "messageLoopTimer.h"
//#include "../util/timeUtil.h"
//#include "../log/log.h"
//S_NAMESPACE_BEGIN
//
//
//MessageLoopTimer::MessageLoopTimer()
//{
//	static std::mutex s_timer_mutex;
//	static uint64_t s_timer_id = 0;
//
//	{
//		ScopeMutex m(s_timer_mutex);
//		m_id = ++s_timer_id;
//	}
//
//	//m_is_running = false;
//	m_callback = NULL;
//	m_looper = NULL;
//	m_id = 0;
//
//	//m_delay_ms = 0;
//	//m_circle_ms = 0;
//	//m_start_time_ms = 0;
//	//m_last_callback_time_ms = 0;
//}
//
//MessageLoopTimer::~MessageLoopTimer()
//{
//	stop();
//	m_looper->releasseTimer(m_id);
//}
//
//bool MessageLoopTimer::init(ICallback* callback, void* user_data, MessageLooper* looper)
//{
//	//m_ref_ptr.init(this, false);
//	if (callback == NULL || looper == NULL)
//	{
//		slog_e("param is invalid");
//		return false;
//	}
//	m_callback = callback;
//	//m_user_data = user_data;
//	m_looper = looper;
//	m_id = m_looper->createTimer(user_data);
//	return true;
//}
//
//bool MessageLoopTimer::start(uint32_t delay_ms, uint32_t circle_ms)
//{
//	//if (m_is_running)
//	//	return true;
//	//m_is_running = true;
//	//m_delay_ms = delay_ms;
//	//m_circle_ms = circle_ms;
//	//m_start_time_ms = TimeUtil::getMsTime();
//	m_looper->addMsgTimerHandler(this);
//	return m_looper->startTimer(m_id, delay_ms, circle_ms);
//}
//
//void MessageLoopTimer::stop()
//{
//	//if (!m_is_running)
//	//	return;
//	//m_is_running = false;
//	m_looper->removeMsgTimerHandler(this);
//	//m_start_time_ms = 0;
//	//m_last_callback_time_ms = 0;
//	m_looper->stopTimer(m_id);
//}
//
//uint64_t MessageLoopTimer::getTimerId()
//{ 
//	return m_id;
//}
//
//void MessageLoopTimer::onMessageTimerTick(uint64_t timer_id, void* user_data)
//{
//	if (m_id != timer_id)
//		return;
//	//if (!m_is_running)
//	//	return;
//	m_callback->onMessageLoopTimerTick(timer_id, user_data);
//
//	//uint64_t first_call_time_ms = m_start_time_ms + m_delay_ms;
//	//uint64_t cur_time_ms = TimeUtil::getMsTime();
//	//if (cur_time_ms < first_call_time_ms)
//	//	return;
//
//	//size_t callback_count = 0;
//	//if (m_last_callback_time_ms < first_call_time_ms)
//	//{
//	//	callback_count++;
//	//	m_last_callback_time_ms = first_call_time_ms;
//	//}
//
//	//if (m_circle_ms > 0)
//	//{
//	//	callback_count += (size_t)((cur_time_ms - m_last_callback_time_ms) / m_circle_ms);
//	//}
//
//
//	//m_last_callback_time_ms = first_call_time_ms + ((cur_time_ms - first_call_time_ms) / m_circle_ms);
//
//	//if (m_circle_ms == 0)
//	//{
//	//	stop();
//	//}
//	//
//	////for (size_t i = 0; i < callback_count; ++i)
//	//if(callback_count > 0)
//	//{
//	//	m_callback->onMessageLoopTimerTick(m_id, m_user_data);
//	//}
//}
//
//
//
//S_NAMESPACE_END
