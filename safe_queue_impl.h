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
/// @file safe_queue_impl.h
/// @brief Implementation of a thread-safe queue based on c++11 std calls
/// It internally contains a std::queue which is protected from concurrent
/// access by std mutexes and conditional variables
///
/// @author Faustino Frechilla
/// @history
/// Ref  Who                 When         What
///      Faustino Frechilla 04-May-2009 Original development (based on pthreads)
///      Faustino Frechilla 19-May-2010 Ported to glib. Removed pthread dependency
///      Faustino Frechilla 06-Jun-2013 Ported to c++11. Removed glib dependency
///      Faustino Frechilla 13-Dec-2013 Handling spurious wakeups in TimedWaitPop
/// @endhistory
///
// ============================================================================

#ifndef _SAFEQUEUEIMPL_H_
#define _SAFEQUEUEIMPL_H_

#include <assert.h>

template <typename T>
SafeQueue<T>::SafeQueue(std::size_t a_maxSize) :
    m_maximumSize(a_maxSize)
{
}

template <typename T>
SafeQueue<T>::~SafeQueue()
{
}

template <typename T>
bool SafeQueue<T>::IsEmpty() const
{
    std::lock_guard<std::mutex> lk(m_mutex);
    return m_theQueue.empty();
}

template <typename T>
void SafeQueue<T>::Push(const T &a_elem)
{
    std::unique_lock<std::mutex> lk(m_mutex);

    while (m_theQueue.size() >= m_maximumSize)
    {
        m_cond.wait(lk);
    }

    bool queueEmpty = m_theQueue.empty();

    m_theQueue.push(a_elem);

    if (queueEmpty)
    {
        // wake up threads waiting for stuff
        m_cond.notify_all();
    }
}

template <typename T>
bool SafeQueue<T>::TryPush(const T &a_elem)
{
    std::lock_guard<std::mutex> lk(m_mutex);

    bool rv = false;
    bool queueEmpty = m_theQueue.empty();

    if (m_theQueue.size() < m_maximumSize)
    {
        m_theQueue.push(a_elem);
        rv = true;
    }

    if (queueEmpty)
    {
        // wake up threads waiting for stuff
        m_cond.notify_all();
    }

    return rv;
}

template <typename T>
void SafeQueue<T>::Pop(T &out_data)
{
    std::unique_lock<std::mutex> lk(m_mutex);

    while (m_theQueue.empty())
    {
        m_cond.wait(lk);
    }

    bool queueFull = (m_theQueue.size() >= m_maximumSize) ? true : false;

    out_data = m_theQueue.front();
    m_theQueue.pop();

    if (queueFull)
    {
        // wake up threads waiting for stuff
        m_cond.notify_all();
    }
}

template <typename T>
bool SafeQueue<T>::TryPop(T &out_data)
{
    std::lock_guard<std::mutex> lk(m_mutex);

    bool rv = false;
    if (!m_theQueue.empty())
    {
        bool queueFull = (m_theQueue.size() >= m_maximumSize) ? true : false;

        out_data = m_theQueue.front();
        m_theQueue.pop();

        if (queueFull)
        {
            // wake up threads waiting for stuff
            m_cond.notify_all();
        }

        rv = true;
    }

    return rv;
}

template <typename T>
bool SafeQueue<T>::TimedWaitPop(T &data, std::chrono::microseconds a_microsecs)
{
    std::unique_lock<std::mutex> lk(m_mutex);
    
    auto wakeUpTime = std::chrono::steady_clock::now + a_microsecs;
    if (m_cond.wait_until(lk, wakeUpTime, [](){!m_theQueue.empty();}))
    {
        // wait_until returns false if the predicate (3rd parameter) still 
        // evaluates to false after the rel_time timeout expired
        // we are in this side of the if-clause because the queue is not empty
        // (so the 3rd parameter evaluated to true)
        bool queueFull = (m_theQueue.size() >= m_maximumSize) ? true : false;
        
        data = m_theQueue.front();
        m_theQueue.pop();
        
        if (queueFull)
        {
            // wake up threads waiting to insert things into the queue. 
            // The queue used to be full, now it's not. 
            m_cond.notify_all();
        }
        
        return true;
    }
    else
    {
        // timed-out and the queue is still empty
        return false;
    }
}

#endif /* _SAFEQUEUEIMPL_H_ */
