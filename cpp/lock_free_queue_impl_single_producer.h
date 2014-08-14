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
/// @file lock_free_queue_impl_single_producer.h
/// @brief Implementation of a circular array based lock-free queue
/// See http://www.codeproject.com/Articles/153898/Yet-another-implementation-of-a-lock-free-circular
/// for more info
///
/// @author Faustino Frechilla
/// @history
/// Ref  Who                 When         What
///      Faustino Frechilla  11-Aug-2014  Original development. File containing only specifics for single producer
///      Faustino Frechilla  12-Aug-2014  inheritance (specialisation) based on templates
/// @endhistory
/// 
// ============================================================================

#ifndef __LOCK_FREE_QUEUE_IMPL_SINGLE_PRODUCER_H__
#define __LOCK_FREE_QUEUE_IMPL_SINGLE_PRODUCER_H__

#include <assert.h> // assert()

template <typename ELEM_T, uint32_t Q_SIZE>
ArrayLockFreeQueueSingleProducer<ELEM_T, Q_SIZE>::ArrayLockFreeQueueSingleProducer():
    m_writeIndex(0),
    m_readIndex(0)
#ifdef LOCK_FREE_Q_KEEP_REAL_SIZE
    ,m_count(0)
#endif
{}

template <typename ELEM_T, uint32_t Q_SIZE>
ArrayLockFreeQueueSingleProducer<ELEM_T, Q_SIZE>::~ArrayLockFreeQueueSingleProducer()
{}

template <typename ELEM_T, uint32_t Q_SIZE>
inline 
uint32_t ArrayLockFreeQueueSingleProducer<ELEM_T, Q_SIZE>::countToIndex(uint32_t a_count)
{
    // if Q_SIZE is a power of 2 this statement could be also written as 
    // return (a_count & (Q_SIZE - 1));
    return (a_count % Q_SIZE);
}

template <typename ELEM_T, uint32_t Q_SIZE>
inline 
uint32_t ArrayLockFreeQueueSingleProducer<ELEM_T, Q_SIZE>::size()
{
#ifdef LOCK_FREE_Q_KEEP_REAL_SIZE
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
    //
    if (currentWriteIndex >= currentReadIndex)
    {
        return (currentWriteIndex - currentReadIndex);
    }
    else
    {
        return (Q_SIZE + currentWriteIndex - currentReadIndex);
    }
#endif // LOCK_FREE_Q_KEEP_REAL_SIZE
}

template <typename ELEM_T, uint32_t Q_SIZE>
inline 
bool ArrayLockFreeQueueSingleProducer<ELEM_T, Q_SIZE>::full()
{
#ifdef LOCK_FREE_Q_KEEP_REAL_SIZE
    return (m_count == (Q_SIZE - 1));
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
#endif // LOCK_FREE_Q_KEEP_REAL_SIZE
}

template <typename ELEM_T, uint32_t Q_SIZE>
bool ArrayLockFreeQueueSingleProducer<ELEM_T, Q_SIZE>::push(const ELEM_T &a_data)
{
    uint32_t currentReadIndex;
    uint32_t currentWriteIndex;
    
    // no need to loop. There is only one producer (this thread)
    currentWriteIndex = m_writeIndex;
    currentReadIndex  = m_readIndex;
    
    if (countToIndex(currentWriteIndex + 1) ==
        countToIndex(currentReadIndex))
    {
        // the queue is full
        return false;
    }
    
    // up to this point we made sure there is space in the Q for more data
    m_theQueue[countToIndex(currentWriteIndex)] = a_data;
    
    // increment atomically write index. Now a consumer thread can read
    // the piece of data that was just stored 
    AtomicAdd(&m_writeIndex, 1);

    // The value was successfully inserted into the queue
#ifdef LOCK_FREE_Q_KEEP_REAL_SIZE
    AtomicAdd(&m_count, 1);
#endif

    return true;
}

template <typename ELEM_T, uint32_t Q_SIZE>
bool ArrayLockFreeQueueSingleProducer<ELEM_T, Q_SIZE>::pop(ELEM_T &a_data)
{
    uint32_t currentMaximumReadIndex;
    uint32_t currentReadIndex;

    do
    {
        // m_maximumReadIndex isn't used when the queue is set up as
        // single-producer. The maximum read index is described by the 
        // current write index
        currentMaximumReadIndex = m_writeIndex;        
        currentReadIndex = m_readIndex;

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
        if (CAS(&(m_readIndex), 
            currentReadIndex, (currentReadIndex + 1)))
        {
            // got here. The value was retrieved from the queue. Note that the
            // data inside the m_queue array is not deleted nor reseted
#ifdef LOCK_FREE_Q_KEEP_REAL_SIZE
            AtomicSub(&(m_count), 1);
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

#endif // __LOCK_FREE_QUEUE_IMPL_SINGLE_PRODUCER_H__
