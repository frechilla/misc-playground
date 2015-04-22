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
///      Faustino Frechilla 19-Mar-2014 Copy/move constructor, operator= and move assignment
/// @endhistory
///
// ============================================================================

#ifndef _SAFEQUEUEIMPL_H_
#define _SAFEQUEUEIMPL_H_

template <typename T>
SafeQueue<T>::SafeQueue(std::size_t a_maxSize):
    m_theQueue(),
    m_maximumSize(a_maxSize),
    m_mutex(),
    m_cond()
{
}

template <typename T>
SafeQueue<T>::~SafeQueue()
{
}

template <typename T>
SafeQueue<T>::SafeQueue(const SafeQueue<T>& a_src):
    m_theQueue(),
    m_maximumSize(0),
    m_mutex(),
    m_cond()
{
    // copying a safe queue involves only copying the data (m_theQueue and
    // m_maximumSize). This object has not been instantiated yet so nobody can
    // be trying to perform push or pop operations on it, but we need to 
    // acquire a_src.m_mutex before copying its data into m_theQueue and
    // m_maximumSize
    std::unique_lock<std::mutex> lk(a_src.m_mutex);
    
    this->m_maximumSize = a_src.m_maximumSize;
    this->m_theQueue = a_src.m_theQueue;
}

template <typename T>
const SafeQueue<T>& SafeQueue<T>::operator=(const SafeQueue<T> &a_src)
{
    if (this != &a_src)
    {
        // lock both mutexes at the same time to avoid deadlocks
        std::unique_lock<std::mutex> this_lk(this->m_mutex, std::defer_lock);
        std::unique_lock<std::mutex> src_lk (a_src.m_mutex, std::defer_lock);
        std::lock(this_lk, src_lk);
        
        // will we need to wake up waiting threads after copying the source
        // queue?
        bool wakeUpWaitingThreads = WakeUpSignalNeeded(a_src);  
        
        // copy data from the left side of the operator= into this intance
        this->m_maximumSize = a_src.m_maximumSize;
        this->m_theQueue = a_src.m_theQueue;
        
        // time now to wake up threads waiting for data to be inserted
        // or extracted
        if (wakeUpWaitingThreads)
        {            
            this->m_cond.notify_all();
        }
    }
    
    return *this;
}

template <typename T>
SafeQueue<T>::SafeQueue(SafeQueue<T>&& a_src):
    m_theQueue(a_src.m_theQueue),       // implicit std::move(a_src.m_theQueue) 
    m_maximumSize(a_src.m_maximumSize), // move constructor called implicitly
    m_mutex(), // instantiate a new mutex
    m_cond()   // instantiate a new conditional variable
{
    // This object has not been instantiated yet. We can assume no one is using 
    // its mutex. 
    // Also, a_src is a temporary object so there is no need to acquire
    // its mutex. 
    // Things can therefore be safely moved without the need for any mutex or 
    // conditional variable
}

template <typename T>
SafeQueue<T>& SafeQueue<T>::operator=(SafeQueue<T> &&a_src)
{
    if (this != &a_src)
    {
        // make sure we hold this mutex before moving things around. a_src is
        // a temporary object so no need to hold its mutex
        std::unique_lock<std::mutex> lk(this->m_mutex);

        // will we need to wake up waiting threads after copying the source
        // queue?
        bool wakeUpWaitingThreads = WakeUpSignalNeeded(a_src);        
        
        // process data from the temporary copy into this intance
        this->m_maximumSize = std::move(a_src.m_maximumSize);
        this->m_theQueue = std::move(a_src.m_theQueue);
        
        // time now to wake up threads waiting for data to be inserted
        // or extracted
        if (wakeUpWaitingThreads)
        {
            this->m_cond.notify_all();
        }
    }
    
    return *this;
}

template <typename T>
bool SafeQueue<T>::WakeUpSignalNeeded(const SafeQueue<T> &a_src) const
{
    if (this->m_theQueue.empty() && (!a_src.m_theQueue.empty()))
    {
        // threads waiting for stuff to be popped off the queue
        return true;
    }
    else if ((this->m_theQueue.size() >= this->m_maximumSize) && 
             (a_src.m_theQueue.size() < a_src.m_maximumSize))
    {
        // threads waiting for stuff to be pushed into the queue
        return true;
    }
    
    return false;
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
    
    auto wakeUpTime = std::chrono::steady_clock::now() + a_microsecs;
    if (m_cond.wait_until(lk, wakeUpTime, 
        [this](){return (m_theQueue.size() > 0);}))
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
