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
/// @endhistory
/// 
// ============================================================================

#ifndef __LOCK_FREE_QUEUE_IMPL_MULTIPLE_PRODUCER_H__
#define __LOCK_FREE_QUEUE_IMPL_MULTIPLE_PRODUCER_H__

#include <assert.h> // assert()
#include <sched.h>  // sched_yield()

template <typename ELEM_T, uint32_t Q_SIZE>
bool ArrayLockFreeQueue<ELEM_T, Q_SIZE, true>::push(const ELEM_T &a_data)
{
    uint32_t currentReadIndex;
    uint32_t currentWriteIndex;
    
    do
    {
        currentWriteIndex = Base_t::m_writeIndex;
        currentReadIndex  = Base_t::m_readIndex;
        
        if (Base_t::countToIndex(currentWriteIndex + 1) ==
            Base_t::countToIndex(currentReadIndex))
        {
            // the queue is full
            return false;
        }
    // There is more than one producer. Keep looping till this thread is able 
    // to allocate space for current piece of data
    } while (!CAS(&(Base_t::m_writeIndex), currentWriteIndex, (currentWriteIndex + 1)));
    
    // Just made sure this index is reserved for this thread.
    Base_t::m_theQueue[Base_t::countToIndex(currentWriteIndex)] = a_data;
    
    // update the maximum read index after saving the piece of data. It can't
    // fail if there is only one thread inserting in the queue. It might fail 
    // if there is more than 1 producer thread because this operation has to
    // be done in the same order as the previous CAS
    while (!CAS(&(Base_t::m_maximumReadIndex), 
        currentWriteIndex, (currentWriteIndex + 1)))
    {
        // this is a good place to yield the thread in case there are more
        // software threads than hardware processors and you have more
        // than 1 producer thread
        // have a look at sched_yield (POSIX.1b)
        sched_yield();
    }

    // The value was successfully inserted into the queue
#ifdef LOCK_FREE_Q_KEEP_REAL_SIZE
    AtomicAdd(&(Base_t::m_count), 1);
#endif

    return true;
}

template <typename ELEM_T, uint32_t Q_SIZE>
bool ArrayLockFreeQueue<ELEM_T, Q_SIZE, true>::pop(ELEM_T &a_data)
{
    uint32_t currentMaximumReadIndex;
    uint32_t currentReadIndex;

    do
    {
        // to ensure thread-safety when there is more than 1 producer 
        // thread a second index is defined (m_maximumReadIndex)
        currentMaximumReadIndex = m_maximumReadIndex;        
        currentReadIndex = Base_t::m_readIndex;

        if (Base_t::countToIndex(currentReadIndex) == 
            Base_t::countToIndex(currentMaximumReadIndex))
        {
            // the queue is empty or
            // a producer thread has allocate space in the queue but is 
            // waiting to commit the data into it
            return false;
        }

        // retrieve the data from the queue
        a_data = Base_t::m_theQueue[Base_t::countToIndex(currentReadIndex)];

        // try to perfrom now the CAS operation on the read index. If we succeed
        // a_data already contains what m_readIndex pointed to before we 
        // increased it
        if (CAS(&(Base_t::m_readIndex), currentReadIndex, (currentReadIndex + 1)))
        {
            // got here. The value was retrieved from the queue. Note that the
            // data inside the m_queue array is not deleted nor reseted
#ifdef LOCK_FREE_Q_KEEP_REAL_SIZE
            AtomicSub(&(Base_t::m_count), 1);
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
