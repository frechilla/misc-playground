// ============================================================================
// Copyright (c) 2010 Faustino Frechilla
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
/// @file lock_free_queue_impl_multiple_producer.h
/// @brief Implementation of a circular array based lock-free queue
/// See http://www.codeproject.com/Articles/153898/Yet-another-implementation-of-a-lock-free-circular
/// for more info
///
/// @author Faustino Frechilla
/// @history
/// Ref  Who                 When         What
///      Faustino Frechilla  11-Aug-2014  Original development. File containing only specifics for multiple producers
///      Faustino Frechilla  12-Aug-2014  inheritance (specialisation) based on templates
///      Faustino Frechilla  10-Aug-2015  Ported to c++11. Removed volatile keywords (using std::atomic)
/// @endhistory
/// 
// ============================================================================

#ifndef __LOCK_FREE_QUEUE_IMPL_MULTIPLE_PRODUCER_H__
#define __LOCK_FREE_QUEUE_IMPL_MULTIPLE_PRODUCER_H__

#include <assert.h> // assert()
#include <sched.h>  // sched_yield()

template <typename ELEM_T, uint32_t Q_SIZE>
ArrayLockFreeQueueMultipleProducers<ELEM_T, Q_SIZE>::ArrayLockFreeQueueMultipleProducers():
    m_writeIndex(0),      // initialisation is not atomic
    m_readIndex(0),       //
    m_maximumReadIndex(0) //
#ifdef _WITH_LOCK_FREE_Q_KEEP_REAL_SIZE
    ,m_count(0)           //
#endif
{}

template <typename ELEM_T, uint32_t Q_SIZE>
ArrayLockFreeQueueMultipleProducers<ELEM_T, Q_SIZE>::~ArrayLockFreeQueueMultipleProducers()
{}

template <typename ELEM_T, uint32_t Q_SIZE>
inline 
uint32_t ArrayLockFreeQueueMultipleProducers<ELEM_T, Q_SIZE>::countToIndex(uint32_t a_count)
{
    // if Q_SIZE is a power of 2 this statement could be also written as 
    // return (a_count & (Q_SIZE - 1));
    return (a_count % Q_SIZE);
}

template <typename ELEM_T, uint32_t Q_SIZE>
inline 
uint32_t ArrayLockFreeQueueMultipleProducers<ELEM_T, Q_SIZE>::size()
{
#ifdef _WITH_LOCK_FREE_Q_KEEP_REAL_SIZE

    return m_count.load();
#else

    uint32_t currentWriteIndex = m_maximumReadIndex.load();
    uint32_t currentReadIndex  = m_readIndex.load();

    // let's think of a scenario where this function returns bogus data
    // 1. when the statement 'currentWriteIndex = m_maximumReadIndex' is run
    // m_maximumReadIndex is 3 and m_readIndex is 2. Real size is 1
    // 2. afterwards this thread is preemted. While this thread is inactive 2 
    // elements are inserted and removed from the queue, so m_maximumReadIndex 
    // is 5 and m_readIndex 4. Real size is still 1
    // 3. Now the current thread comes back from preemption and reads m_readIndex.
    // currentReadIndex is 4
    // 4. currentReadIndex is bigger than currentWriteIndex, so
    // m_totalSize + currentWriteIndex - currentReadIndex is returned, that is,
    // it returns that the queue is almost full, when it is almost empty
    //
    if (countToIndex(currentWriteIndex) >= countToIndex(currentReadIndex))
    {
        return (currentWriteIndex - currentReadIndex);
    }
    else
    {
        return (Q_SIZE + currentWriteIndex - currentReadIndex);
    }
#endif // _WITH_LOCK_FREE_Q_KEEP_REAL_SIZE
}

template <typename ELEM_T, uint32_t Q_SIZE>
inline 
bool ArrayLockFreeQueueMultipleProducers<ELEM_T, Q_SIZE>::full()
{
#ifdef _WITH_LOCK_FREE_Q_KEEP_REAL_SIZE

    return (m_count.load() == (Q_SIZE - 1));
#else

    uint32_t currentWriteIndex = m_writeIndex;
    uint32_t currentReadIndex  = m_readIndex;
    
    if (countToIndex(currentWriteIndex + 1) == countToIndex(currentReadIndex))
    {
        // the queue is full
        return true;
    }
    else
    {
        // not full!
        return false;
    }
#endif // _WITH_LOCK_FREE_Q_KEEP_REAL_SIZE
}

template <typename ELEM_T, uint32_t Q_SIZE>
bool ArrayLockFreeQueueMultipleProducers<ELEM_T, Q_SIZE>::push(const ELEM_T &a_data)
{
    uint32_t currentWriteIndex;
    
    do
    {
        currentWriteIndex = m_writeIndex.load();
        
        if (countToIndex(currentWriteIndex + 1) == countToIndex(m_readIndex.load()))
        {
            // the queue is full
            return false;
        }
    // There is more than one producer. Keep looping till this thread is able 
    // to allocate space for current piece of data
    //
    // using compare_exchange_strong because it isn't allowed to fail spuriously
    // When the compare_exchange operation is in a loop the weak version
    // will yield better performance on some platforms, but here we'd have to
    // load m_writeIndex all over again
    } while (!m_writeIndex.compare_exchange_strong(
                currentWriteIndex, (currentWriteIndex + 1)));
    
    // Just made sure this index is reserved for this thread.
    m_theQueue[countToIndex(currentWriteIndex)] = a_data;
    
    // update the maximum read index after saving the piece of data. It can't
    // fail if there is only one thread inserting in the queue. It might fail 
    // if there is more than 1 producer thread because this operation has to
    // be done in the same order as the previous CAS
    //
    // using compare_exchange_weak because they are allowed to fail spuriously
    // (act as if *this != expected, even if they are equal), but when the
    // compare_exchange operation is in a loop the weak version will yield
    // better performance on some platforms.
    while (!m_maximumReadIndex.compare_exchange_weak(
                currentWriteIndex, (currentWriteIndex + 1)))
    {
        // this is a good place to yield the thread in case there are more
        // software threads than hardware processors and you have more
        // than 1 producer thread
        // have a look at sched_yield (POSIX.1b)
        //sched_yield();
    }

    // The value was successfully inserted into the queue
#ifdef _WITH_LOCK_FREE_Q_KEEP_REAL_SIZE
    m_count.fetch_add(1);
#endif

    return true;
}

template <typename ELEM_T, uint32_t Q_SIZE>
bool ArrayLockFreeQueueMultipleProducers<ELEM_T, Q_SIZE>::pop(ELEM_T &a_data)
{
    uint32_t currentReadIndex;

    do
    {
        currentReadIndex = m_readIndex.load();

        // to ensure thread-safety when there is more than 1 producer 
        // thread a second index is defined (m_maximumReadIndex)
        if (countToIndex(currentReadIndex) == countToIndex(m_maximumReadIndex.load()))
        {
            // the queue is empty or
            // a producer thread has allocate space in the queue but is 
            // waiting to commit the data into it
            return false;
        }

        // retrieve the data from the queue
        a_data = m_theQueue[countToIndex(currentReadIndex)];

        // try to perfrom now the CAS operation on the read index. If we succeed
        // a_data already contains what m_readIndex pointed to before we 
        // increased it
        if (m_readIndex.compare_exchange_strong(currentReadIndex, (currentReadIndex + 1)))
        {
            // got here. The value was retrieved from the queue. Note that the
            // data inside the m_queue array is not deleted nor reseted
#ifdef _WITH_LOCK_FREE_Q_KEEP_REAL_SIZE
            m_count.fetch_sub(1);
#endif
            return true;
        }
        
        // it failed retrieving the element off the queue. Someone else must
        // have read the element stored at countToIndex(currentReadIndex)
        // before we could perform the CAS operation        

    } while(1); // keep looping to try again!

    // Something went wrong. it shouldn't be possible to reach here
    assert(0);

    // Add this return statement to avoid compiler warnings
    return false;    
}

#endif // __LOCK_FREE_QUEUE_IMPL_MULTIPLE_PRODUCER_H__
