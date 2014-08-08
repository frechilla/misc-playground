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
/// @file array_lock_free_queue_impl.h
/// @brief Implementation of a circular array based lock-free queue
///
/// @author Faustino Frechilla
/// @history
/// Ref  Who                 When         What
///      Faustino Frechilla  11-Jul-2010  Original development
/// @endhistory
/// 
// ============================================================================

#ifndef __ARRAY_LOCK_FREE_QUEUE_IMPL_H__
#define __ARRAY_LOCK_FREE_QUEUE_IMPL_H__

#include <assert.h> // assert()
#include <sched.h>  // sched_yield()

template <typename ELEM_T, uint32_t Q_SIZE>
ArrayLockFreeQueue<ELEM_T, Q_SIZE>::ArrayLockFreeQueue() :
    m_writeIndex(0),
    m_readIndex(0),
    m_maximumReadIndex(0) // only for MultipleProducerThread queues
{
#ifdef ARRAY_LOCK_FREE_Q_KEEP_REAL_SIZE
    m_count = 0;
#endif
}

template <typename ELEM_T, uint32_t Q_SIZE>
ArrayLockFreeQueue<ELEM_T, Q_SIZE>::~ArrayLockFreeQueue()
{
}

template <typename ELEM_T, uint32_t Q_SIZE>
inline
uint32_t ArrayLockFreeQueue<ELEM_T, Q_SIZE>::countToIndex(uint32_t a_count)
{
    // if Q_SIZE is a power of 2 this statement could be also written as 
    // return (a_count & (Q_SIZE - 1));
    return (a_count % Q_SIZE);
}

template <typename ELEM_T, uint32_t Q_SIZE>
uint32_t ArrayLockFreeQueue<ELEM_T, Q_SIZE>::size()
{
#ifdef ARRAY_LOCK_FREE_Q_KEEP_REAL_SIZE
    return m_count;
#else
    uint32_t currentWriteIndex = m_writeIndex;
    uint32_t currentReadIndex  = m_readIndex;

    // let's think of a scenario where this function returns bogus data
    // 1. when the statement 'currentWriteIndex = m_writeIndex' is run
    // m_writeIndex is 3 and m_readIndex is 2. Real size is 1
    // 2. afterwards this thread is preemted. While this thread is inactive 2 
    // elements are inserted and removed from the queue, so m_writeIndex is 5
    // m_readIndex 4. Real size is still 1
    // 3. Now the current thread comes back from preemption and reads m_readIndex.
    // currentReadIndex is 4
    // 4. currentReadIndex is bigger than currentWriteIndex, so
    // m_totalSize + currentWriteIndex - currentReadIndex is returned, that is,
    // it returns that the queue is almost full, when it is almost empty
    
    if (currentWriteIndex >= currentReadIndex)
    {
        return (currentWriteIndex - currentReadIndex);
    }
    else
    {
        return (Q_SIZE + currentWriteIndex - currentReadIndex);
    }
#endif // ARRAY_LOCK_FREE_Q_KEEP_REAL_SIZE
}

template <typename ELEM_T, uint32_t Q_SIZE>
bool ArrayLockFreeQueue<ELEM_T, Q_SIZE>::push(const ELEM_T &a_data)
{
    uint32_t currentReadIndex;
    uint32_t currentWriteIndex;

    do
    {
        currentWriteIndex = m_writeIndex;
        currentReadIndex  = m_readIndex;
        if (countToIndex(currentWriteIndex + 1) ==
            countToIndex(currentReadIndex))
        {
            // the queue is full
            return false;
        }

    } while (!CAS(&m_writeIndex, currentWriteIndex, (currentWriteIndex + 1)));

    // We know now that this index is reserved for us. Use it to save the data
    m_theQueue[countToIndex(currentWriteIndex)] = a_data;

    // update the maximum read index after saving the data. It wouldn't fail if there is only one thread 
    // inserting in the queue. It might fail if there are more than 1 producer threads because this
    // operation has to be done in the same order as the previous CAS
    while (!CAS(&m_maximumReadIndex, currentWriteIndex, (currentWriteIndex + 1)))
    {
        // this is a good place to yield the thread in case there are more
        // software threads than hardware processors and you have more
        // than 1 producer thread
        // have a look at sched_yield (POSIX.1b)
        sched_yield();
    }

    // The value was successfully inserted into the queue
#ifdef ARRAY_LOCK_FREE_Q_KEEP_REAL_SIZE
    AtomicAdd(&m_count, 1);
#endif

    return true;
}

template <typename ELEM_T, uint32_t Q_SIZE>
bool ArrayLockFreeQueue<ELEM_T, Q_SIZE>::pop(ELEM_T &a_data)
{
    uint32_t currentMaximumReadIndex;
    uint32_t currentReadIndex;

    do
    {
        // to ensure thread-safety when there is more than 1 producer thread
        // a second index is defined (m_maximumReadIndex)
        currentReadIndex        = m_readIndex;
        currentMaximumReadIndex = m_maximumReadIndex;

        if (countToIndex(currentReadIndex) == 
            countToIndex(currentMaximumReadIndex))
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
        if (CAS(&m_readIndex, currentReadIndex, (currentReadIndex + 1)))
        {
            // got here. The value was retrieved from the queue. Note that the
            // data inside the m_queue array is not deleted nor reseted
#ifdef ARRAY_LOCK_FREE_Q_KEEP_REAL_SIZE
            AtomicSub(&m_count, 1);
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

#endif // __ARRAY_LOCK_FREE_QUEUE_IMPL_H__

