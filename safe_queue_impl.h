// ============================================================================
// Copyright (c) 2009-2010 Faustino Frechilla
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
/// @brief Implementation of a thread-safe queue based on glib system calls
/// It internally contains a std::queue which is protected from concurrent
/// access by glib mutextes and conditional variables
///
/// @author Faustino Frechilla
/// @history
/// Ref  Who                 When         What
///      Faustino Frechilla 04-May-2009 Original development (based on pthreads)
///      Faustino Frechilla 19-May-2010 Ported to glib. Removed pthread dependency
/// @endhistory
///
// ============================================================================

#ifndef __SAFEQUEUEIMPL_H__
#define __SAFEQUEUEIMPL_H__

#include <assert.h>

#define NANOSECONDS_PER_SECOND 1000000000

template <typename T>
SafeQueue<T>::SafeQueue(std::size_t a_maxSize) :
    m_maximumSize(a_maxSize)
{
    if (!g_thread_supported ())
    {
        // glib thread system hasn't been initialized yet
        g_thread_init(NULL);
    }

    m_mutex = g_mutex_new();
    m_cond  = g_cond_new();

    assert(m_mutex != NULL);
    assert(m_cond != NULL);
}

template <typename T>
SafeQueue<T>::~SafeQueue()
{
    g_cond_free(m_cond);
    g_mutex_free(m_mutex);
}

template <typename T>
bool SafeQueue<T>::IsEmpty()
{
    bool rv;

    g_mutex_lock(m_mutex);
    rv = m_theQueue.empty();
    g_mutex_unlock(m_mutex);

    return rv;
}

template <typename T>
bool SafeQueue<T>::Push(const T &a_elem)
{
    g_mutex_lock(m_mutex);

    while (m_theQueue.size() >= m_maximumSize)
    {
        g_cond_wait(m_cond, m_mutex);
    }

    bool queueEmpty = m_theQueue.empty();

    m_theQueue.push(a_elem);

    if (queueEmpty)
    {
        // wake up threads waiting for stuff
        g_cond_broadcast(m_cond);
    }

    g_mutex_unlock(m_mutex);

    return true;
}

template <typename T>
bool SafeQueue<T>::TryPush(const T &a_elem)
{
    g_mutex_lock(m_mutex);

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
        g_cond_broadcast(m_cond);
    }

    g_mutex_unlock(m_mutex);

    return rv;
}

template <typename T>
void SafeQueue<T>::Pop(T &out_data)
{
    g_mutex_lock(m_mutex);

    while (m_theQueue.empty())
    {
        g_cond_wait(m_cond, m_mutex);
    }

    bool queueFull = (m_theQueue.size() >= m_maximumSize) ? true : false;

    out_data = m_theQueue.front();
    m_theQueue.pop();

    if (queueFull)
    {
        // wake up threads waiting for stuff
        g_cond_broadcast(m_cond);
    }

    g_mutex_unlock(m_mutex);
}

template <typename T>
bool SafeQueue<T>::TryPop(T &out_data)
{
    g_mutex_lock(m_mutex);

    bool rv = false;
    if (!m_theQueue.empty())
    {
        bool queueFull = (m_theQueue.size() >= m_maximumSize) ? true : false;

        out_data = m_theQueue.front();
        m_theQueue.pop();

        if (queueFull)
        {
            // wake up threads waiting for stuff
            g_cond_broadcast(m_cond);
        }

        rv = true;
    }

    g_mutex_unlock(m_mutex);

    return rv;
}

template <typename T>
bool SafeQueue<T>::TimedWaitPop(T &data, glong microsecs)
{
    g_mutex_lock(m_mutex);

    // adding microsecs to now
    GTimeVal abs_time;
    g_get_current_time(&abs_time);
    g_time_val_add(&abs_time, microsecs);

    gboolean retcode = TRUE;
    while (m_theQueue.empty() && (retcode != FALSE))
    {
        // Returns TRUE if cond was signalled, or FALSE on timeout
        retcode = g_cond_timed_wait(m_cond, m_mutex, &abs_time);
    }

    bool rv = false;
    bool queueFull = (m_theQueue.size() >= m_maximumSize) ? true : false;
    if (retcode != FALSE)
    {
        data = m_theQueue.front();
        m_theQueue.pop();

        rv = true;
    }

    if (rv && queueFull)
    {
        // wake up threads waiting for stuff
        g_cond_broadcast(m_cond);
    }

    g_mutex_unlock(m_mutex);

    return rv;
}

#endif // __SAFEQUEUEIMPL_H__

