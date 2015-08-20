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
///      Faustino Frechilla  10-Aug-2015  Ported to c++11. Removed volatile keywords (using std::atomic)
/// @endhistory
/// 
// ============================================================================

#ifndef __LOCK_FREE_QUEUE_IMPL_SINGLE_PRODUCER_H__
#define __LOCK_FREE_QUEUE_IMPL_SINGLE_PRODUCER_H__

#include <assert.h> // assert()

template <typename ELEM_T, uint32_t Q_SIZE>
ArrayLockFreeQueueSingleProducer<ELEM_T, Q_SIZE>::ArrayLockFreeQueueSingleProducer():
    m_writeIndex(0), // initialisation is not atomic
    m_readIndex(0)   // 
#ifdef _WITH_LOCK_FREE_Q_KEEP_REAL_SIZE
    ,m_count(0)      // 
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
#ifdef _WITH_LOCK_FREE_Q_KEEP_REAL_SIZE
    return m_count.load();
#else
    uint32_t currentWriteIndex = m_writeIndex.load();
    uint32_t currentReadIndex  = m_readIndex.load();

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
bool ArrayLockFreeQueueSingleProducer<ELEM_T, Q_SIZE>::full()
{
#ifdef _WITH_LOCK_FREE_Q_KEEP_REAL_SIZE
    return (m_count.load() == (Q_SIZE - 1));
#else
    uint32_t currentWriteIndex = m_writeIndex.load();
    uint32_t currentReadIndex  = m_readIndex.load();
    
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
bool ArrayLockFreeQueueSingleProducer<ELEM_T, Q_SIZE>::push(const ELEM_T &a_data)
{
    uint32_t currentWriteIndex;
    
    // no need to loop. There is only one producer (this thread)
    currentWriteIndex = m_writeIndex.load();
    
    if (countToIndex(currentWriteIndex + 1) == 
            countToIndex(m_readIndex.load()))
    {
        // the queue is full
        return false;
    }
    
    // up to this point we made sure there is space in the Q for more data
    m_theQueue[countToIndex(currentWriteIndex)] = a_data;
    
    // increment write index 
    m_writeIndex.fetch_add(1);

    // The value was successfully inserted into the queue
#ifdef _WITH_LOCK_FREE_Q_KEEP_REAL_SIZE
    m_count.fetch_add(1);
#endif

    return true;
}

template <typename ELEM_T, uint32_t Q_SIZE>
bool ArrayLockFreeQueueSingleProducer<ELEM_T, Q_SIZE>::pop(ELEM_T &a_data)
{
    uint32_t currentReadIndex;

    do
    {
        currentReadIndex = m_readIndex.load();

        if (countToIndex(currentReadIndex) == 
                countToIndex(m_writeIndex.load()))
        {
            // queue is empty
            return false;
        }

        // retrieve the data from the queue
        a_data = m_theQueue[countToIndex(currentReadIndex)];

        // try to perfrom now the CAS operation on the read index. If we succeed
        // a_data already contains what m_readIndex pointed to before we 
        // increased it.
        // using compare_exchange_strong because it isn't allowed to fail spuriously
        // (act as if *this != expected, even if they are equal) 
        // When the compare_exchange operation is in a loop the weak version 
        // will yield better performance on some platforms (but here we'd have to
        // load m_writeIndex all over again, better not to fail spuriously)
        if (m_readIndex.compare_exchange_strong(
                currentReadIndex, (currentReadIndex + 1)))
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

#endif // __LOCK_FREE_QUEUE_IMPL_SINGLE_PRODUCER_H__

