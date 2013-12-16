// ============================================================================
/// @file  consumer_thread.h
/// @brief This file contains the Consumer Thread class.
///
/// @copyright
/// @history
/// Ref        Who                When        What
///            Faustino Frechilla 04-May-2009 Original development
/// @endhistory
///
// ============================================================================

#ifndef CONSUMERTHREAD_H_
#define CONSUMERTHREAD_H_

#include <pthread.h>
#include "BlockingQueue.h"
#include "Delegate.h"

template <typename T>
class ConsumerThread
{
public:
    /// @brief ConsumerThread constructor
    /// The queue size will be the blocking queue default size
    /// @param a_consumeDelegate delegate to the function to be called per consumable
    /// @param a_initDelegate a delegate to the initialise function. It does nothing by default
    ConsumerThread(
            Delegate<void(T)> a_consumeDelegate,
            Delegate<void()>  a_initDelegate = MakeDelegate(&ConsumerThread::DoNothing) );
    /// @brief ConsumerThread constructor
    /// @param a_queueSize blocking queue size
    /// @param a_consumeDelegate delegate to the function to be called per consumable
    /// @param a_initDelegate a delegate to the initialise function. It does nothing by default
    ConsumerThread(
            std::size_t a_queueSize,
            Delegate<void(T)> a_consumeDelegate,
            Delegate<void()>  a_initDelegate = MakeDelegate(&ConsumerThread::DoNothing) );
    virtual ~ConsumerThread();

    /// suspends execution of the calling thread until the target thread terminates
    /// @return true if success. False otherwise
    bool Join();

    /// @brief inserts data into the consumable queue to be processed by the ConsumerThread
    /// This call can block if another thread owns the lock that protects the queue
    /// @param element to insert into the queue
    /// @return true if the element was successfully inserted into the queue. False otherwise
    bool Produce(T a_data);

private:
    /// the actual thread
    pthread_t m_thread;

    /// flag to control if the execution of the thread must terminate
    volatile bool m_terminate;

    /// Delegate to the Consume function
    Delegate<void(T)> m_consumeDelegate;

    /// Delegate to the Init function
    Delegate<void()> m_initDelegate;

    /// The queue with the data to be processed
    BlockingQueue<T> m_consumableQueue;

    /// Creates the consumer thread calling to the necessary system functions
    void SpawnThread();

    /// @brief dummy function to be used when the user doesn't specify the init function
    static void DoNothing() {};

    /// The routine that will be called by the posix thread
    static void* ThreadRoutine(void *a_ThreadParam);
};

#include "consumer_thread_impl.h"

#endif /* CONSUMERTHREAD_H_ */
