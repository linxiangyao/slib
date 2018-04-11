#ifndef S_MESSAGE_LOOP_THREAD_H_
#define S_MESSAGE_LOOP_THREAD_H_
#include "messageLooper.h"
#include "thread.h"
S_NAMESPACE_BEGIN



class MessageLoopThread : public IThread
{
public:
	MessageLoopThread(IMessageLoopHandler* handler, bool is_auto_delete_handler = true);
    ~MessageLoopThread();

    virtual bool start();
    virtual void stop();
    virtual void stopAndJoin();
    virtual void join();

    virtual EThreadState getState();
    virtual IThreadRun* getRun();
    MessageLooper* getLooper();
	IMessageLoopHandler* getMessageLoopHandler();


private:
	class __MessageLoopRunnable;

    IThread* m_thread;
	IMessageLoopHandler* m_handler;
	bool m_is_auto_delete_handler;
    __MessageLoopRunnable* m_runnable;
};





S_NAMESPACE_END
#endif
