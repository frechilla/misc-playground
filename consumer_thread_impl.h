// ============================================================================
/// @file  consumer_thread_impl.h
/// @brief This file contains the Consumer Thread class.
///
/// @copyright
/// @history
/// Ref        Who                When        What
///            Faustino Frechilla 04-May-2009 Original development
/// @endhistory
///
// ============================================================================

#ifndef CONSUMERTHREADIMPL_H_
#define CONSUMERTHREADIMPL_H_

#define CONSUMER_THREAD_TIMEOUT_SEC  0
#define CONSUMER_THREAD_TIMEOUT_NSEC 1000000 // 1 millisecond (10^6 nanosecs = 1 millisecond)

template <typename T>
ConsumerThread<T>::ConsumerThread(Delegate<void(T)> a_consumeDelegate, Delegate<void()>  a_initDelegate) :
    m_terminate(false),
    m_consumeDelegate(a_consumeDelegate),
    m_initDelegate(a_initDelegate),
    m_consumableQueue()
{
    SpawnThread();
}

template <typename T>
ConsumerThread<T>::ConsumerThread(std::size_t a_queueSize, Delegate<void(T)> a_consumeDelegate, Delegate<void()>  a_initDelegate) :
    m_terminate(false),
    m_consumeDelegate(a_consumeDelegate),
    m_initDelegate(a_initDelegate),
    m_consumableQueue(a_queueSize)
{
    SpawnThread();
}

template <typename T>
ConsumerThread<T>::~ConsumerThread()
{
    void* dataReturned;
    m_terminate = true;

    if (pthread_join(m_thread, &dataReturned) != 0)
    {
        //TODO log the error
    }
}

template <typename T>
void ConsumerThread<T>::SpawnThread()
{
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE); // make the attribute have JOINABLE option

    if (pthread_create(&m_thread, &attr, ConsumerThread<T>::ThreadRoutine, reinterpret_cast<void*>(this)) != 0)
    {
        //TODO error creating the thread
    }
}

template <typename T>
bool ConsumerThread<T>::Join()
{
    void* dataReturned;
    m_terminate = true;


    if (pthread_join(m_thread, &dataReturned) != 0)
    {
        return false;
    }

    return true;
}

template <typename T>
bool ConsumerThread<T>::Produce(T a_data)
{
    return m_consumableQueue.Push(a_data);
}

template <typename T>
void* ConsumerThread<T>::ThreadRoutine(void *a_ThreadParam)
{
    ConsumerThread<T>* thisThread = reinterpret_cast< ConsumerThread<T>* >(a_ThreadParam);

    // init function
    thisThread->m_initDelegate();

    struct timespec waitTime;
    waitTime.tv_sec  = CONSUMER_THREAD_TIMEOUT_SEC;
    waitTime.tv_nsec = CONSUMER_THREAD_TIMEOUT_NSEC;
    // loop to check if the thread must be terminated
    while (thisThread->m_terminate == false)
    {
        T thisElem;
        if (thisThread->m_consumableQueue.TimedWaitPop(thisElem, waitTime))
        {
            thisThread->m_consumeDelegate(thisElem);
        }
    }

    return NULL;
}

#endif /* CONSUMERTHREADIMPL_H_ */
