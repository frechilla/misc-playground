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
/// @file  consumer_thread_impl.h
/// @brief This file contains the Consumer Thread class implementation.
///
/// @author Faustino Frechilla
/// @history
/// Ref        Who                When        What
///            Faustino Frechilla 04-May-2009 Original development
///            Faustino Frechilla 06-Jun-2013 Ported to c++11
/// @endhistory
///
// ============================================================================

#ifndef _CONSUMERTHREADIMPL_H_
#define _CONSUMERTHREADIMPL_H_

#include <assert.h>

// wake up timeout. The consumer thread will wake up when the timeout is hit
// when there is no data to consume to check if it has been told to finish
#define CONSUMER_THREAD_TIMEOUT_USEC 1000 // (1ms = 1000us)

template <typename T>
ConsumerThread<T>::ConsumerThread(std::function<void(T)> a_consumeDelegate, std::function<void()>  a_initDelegate) :
    m_terminate(false),
    m_consumeDelegate(a_consumeDelegate),
    m_initDelegate(a_initDelegate),
    m_consumableQueue()
{
    SpawnThread();
}

template <typename T>
ConsumerThread<T>::ConsumerThread(std::size_t a_queueSize, std::function<void(T)> a_consumeDelegate, std::function<void()>  a_initDelegate) :
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
    if (m_producerThread.get())
    {
        Join();
    }
}

template <typename T>
void ConsumerThread<T>::SpawnThread()
{
    m_producerThread.reset(
        new std::thread(std::bind(&ConsumerThread::ThreadRoutine, this)));
    
    //m_producerThread->detach(); // the consumer thread will run as no joinable
}

template <typename T>
void ConsumerThread<T>::Join()
{
    m_terminate.store(true);
    
    m_producerThread->join();
    m_producerThread.reset();
}

template <typename T>
bool ConsumerThread<T>::Produce(T a_data)
{
    assert(m_producerThread.get() != 0);

    return m_consumableQueue.TryPush(a_data);
}

template <typename T>
void ConsumerThread<T>::ProduceOrBlock(T a_data)
{
    assert(m_producerThread.get() != 0);
    
    m_consumableQueue.Push(a_data);
}

template <typename T>
void ConsumerThread<T>::ThreadRoutine()
{
    // init function
    this->m_initDelegate();

    // loop to check if the thread must be terminated
    while (this->m_terminate.load() == false)
    {
        T thisElem;
        if (this->m_consumableQueue.TimedWaitPop(
            thisElem, std::chrono::microseconds(CONSUMER_THREAD_TIMEOUT_USEC)))
        {
            this->m_consumeDelegate(thisElem);
        }
    }
}

#endif /* _CONSUMERTHREADIMPL_H_ */
