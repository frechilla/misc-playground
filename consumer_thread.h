// ============================================================================
// Copyright (c) 2009-2013 Faustino Frechilla
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
/// @file  consumer_thread.h
/// @brief This file contains the Consumer Thread class.
///
/// Your compiler must have support for c++11. This is an example of how to 
/// compile an application that makes use of this consumer thread with gcc 4.8:
///   $ g++ -g -O0 -Wall -std=c++11 -D_REENTRANT -c app.cpp
///   $ g++ app.o -o app
///
/// @author Faustino Frechilla
/// @history
/// Ref        Who                When        What
///            Faustino Frechilla 04-May-2009 Original development
///            Faustino Frechilla 06-Jun-2013 Ported to c++11
/// @endhistory
///
// ============================================================================

#ifndef _CONSUMERTHREAD_H_
#define _CONSUMERTHREAD_H_

#include <thread>
#include <atomic>
#include <functional>
#include "safe_queue.h"

template <typename T>
class ConsumerThread
{
public:
    /// @brief ConsumerThread constructor
    /// The queue size will be set to the safe queue's default
    /// @param a_consumeDelegate delegate to the function to be called per consumable
    /// @param a_initDelegate a delegate to the initialise function. It does nothing by default
    ///        This function will get called from the context of the consumer thread
    ConsumerThread(
        std::function<void(T)> a_consumeDelegate,
        std::function<void( )> a_initDelegate = std::bind(&ConsumerThread::DoNothing) );
    /// @brief ConsumerThread constructor
    /// @param a_queueSize safe queue size
    /// @param a_consumeDelegate delegate to the function to be called per consumable
    /// @param a_initDelegate a delegate to the initialise function. It does nothing by default
    ///        This function will get called from the context of the consumer thread
    ConsumerThread(
        std::size_t a_queueSize,
        std::function<void(T)> a_consumeDelegate,
        std::function<void( )> a_initDelegate = std::bind(&ConsumerThread::DoNothing) );

    virtual ~ConsumerThread();

    /// @brief Tell the consumer thread to finish and wait until it does so
    /// suspends execution of the calling thread until the target thread terminates
    void Join();

    /// @brief inserts data into the consumable queue to be processed by the ConsumerThread
    /// This call can block for a short period of time if another thread owns the lock that 
    /// protects the queue. If the a_data can-t be pushed into the queue (it is full) the 
    /// function will return as soon as possible
    /// @param element to insert into the queue
    /// @return true if the element was successfully inserted into the queue. False otherwise
    bool Produce(T a_data);
    
    /// @brief inserts data into the consumable queue to be processed by the ConsumerThread
    /// This call will block until a_data can be pushed into the queue
    /// @param element to insert into the queue
    void ProduceOrBlock(T a_data);

private:
    /// the worker thread
    std::unique_ptr<std::thread> m_producerThread;

    /// flag to control if the execution of the thread must terminate
    std::atomic<bool> m_terminate;

    /// Delegate to the Consume function
    std::function<void(T)> m_consumeDelegate;

    /// Delegate to the Init function
    std::function<void()> m_initDelegate;

    /// The queue with the data to be processed
    SafeQueue<T> m_consumableQueue;

    /// Creates the consumer thread calling to the necessary system functions
    void SpawnThread();

    /// brief the routine that will be run by the consumer thread
    void ThreadRoutine();

    /// @brief dummy function 
    /// To be used when the user doesn't specify the init function
    static void DoNothing() {};
};

#include "consumer_thread_impl.h"

#endif /* _CONSUMERTHREAD_H_ */
