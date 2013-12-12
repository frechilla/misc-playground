// ============================================================================
// Copyright (c) 2009 Faustino Frechilla
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
/// @brief This file contains the implementation of a thread queue class.
///
/// @copyright
/// @history
/// Ref        Who                When        What
///            Faustino Frechilla 04-May-2009 Original development
/// @endhistory
///
// ============================================================================

#ifndef __SAFEQUEUEIMPL_H__
#define __SAFEQUEUEIMPL_H__

#include <errno.h>
#include <time.h>
#include <stdint.h> // for uint64_t

#define NANOSECONDS_PER_SECOND 1000000000

template <typename T>
SafeQueue<T>::SafeQueue(std::size_t a_maxSize) :
    m_maximumSize(a_maxSize)
{
    if (pthread_mutex_init(&m_mutex, NULL) != 0)
    {
        //TODO couldn't create
    }

    if (pthread_cond_init(&m_cond, NULL) != 0)
    {
        //TODO couldn't create
    }
}

template <typename T>
SafeQueue<T>::~SafeQueue()
{
    pthread_cond_destroy(&m_cond);
    pthread_mutex_destroy(&m_mutex);
}

template <typename T>
bool SafeQueue<T>::IsEmpty()
{
    bool rv;

    pthread_mutex_lock(&m_mutex);

    rv = m_theQueue.empty();

    pthread_mutex_unlock(&m_mutex);

    return rv;
}

template <typename T>
bool SafeQueue<T>::Push(T a_elem)
{
    if (pthread_mutex_lock(&m_mutex) != 0)
    {
        return false;
    }

    int retcode = 0;
    while (m_theQueue.size() >= m_maximumSize)
    {
        retcode = pthread_cond_wait(&m_cond, &m_mutex);
    }

    bool queueEmpty = m_theQueue.empty();

    m_theQueue.push(a_elem);

    pthread_mutex_unlock(&m_mutex);

    if (queueEmpty)
    {
        // signal after releasing the mutex
        pthread_cond_broadcast(&m_cond);
    }

    return true;
}

template <typename T>
bool SafeQueue<T>::TryPush(T a_elem)
{
    if (pthread_mutex_lock(&m_mutex) != 0)
    {
        return false;
    }

    bool rv = false;
    bool queueEmpty = m_theQueue.empty();

    if (m_theQueue.size() < m_maximumSize)
    {
        m_theQueue.push(a_elem);

        rv = true;
    }

    pthread_mutex_unlock(&m_mutex);

    if (queueEmpty)
    {
        // signal after releasing the mutex
        pthread_cond_broadcast(&m_cond);
    }

    return rv;
}

template <typename T>
T SafeQueue<T>::Pop()
{
    pthread_mutex_lock(&m_mutex);

    int retcode = 0;
    while (m_theQueue.empty())
    {
        retcode = pthread_cond_wait(&m_cond, &m_mutex);
    }

    bool queueFull = (m_theQueue.size() >= m_maximumSize) ? true : false;

    T data = m_theQueue.front();
    m_theQueue.pop();

    pthread_mutex_unlock(&m_mutex);

    if (queueFull)
    {
        // signal after releasing the mutex
        pthread_cond_broadcast(&m_cond);
    }

    return data;
}

template <typename T>
bool SafeQueue<T>::TryPop(T &data)
{
    pthread_mutex_lock(&m_mutex);

    bool rv = false;
    if (!m_theQueue.empty())
    {
        data = m_theQueue.front();
        m_theQueue.pop();

        rv = true;
    }

    pthread_mutex_unlock(&m_mutex);

    return rv;
}

template <typename T>
bool SafeQueue<T>::TimedWaitPop(T &data, const struct timespec& a_abstime)
{
    pthread_mutex_lock(&m_mutex);

    struct timespec timeout;
    clock_gettime(CLOCK_REALTIME, &timeout);

    timeout.tv_sec  += a_abstime.tv_sec;
    timeout.tv_nsec += a_abstime.tv_nsec;
    while (timeout.tv_nsec > NANOSECONDS_PER_SECOND)
    {
        timeout.tv_sec++;
        timeout.tv_nsec -= NANOSECONDS_PER_SECOND;
    }

    int retcode = 0;
    while (m_theQueue.empty() && (retcode != ETIMEDOUT))
    {
        retcode = pthread_cond_timedwait(&m_cond, &m_mutex, &timeout);
    }

    bool rv = false;
    bool queueFull = (m_theQueue.size() >= m_maximumSize) ? true : false;
    if (retcode != ETIMEDOUT)
    {
        data = m_theQueue.front();
        m_theQueue.pop();

        rv = true;
    }

    pthread_mutex_unlock(&m_mutex);

    if ( rv && queueFull )
    {
        // signal after releasing the mutex
        pthread_cond_broadcast(&m_cond);
    }

    return rv;
}

#endif // __SAFEQUEUEIMPL_H__

