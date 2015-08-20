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
/// @file lock_free_queue.h
/// @brief Definition of a circular array based lock-free queue
/// See http://www.codeproject.com/Articles/153898/Yet-another-implementation-of-a-lock-free-circular
/// for more info
///
/// @author Faustino Frechilla
/// @history
/// Ref  Who                 When         What
///      Faustino Frechilla  11-Jul-2010  Original development
///      Faustino Frechilla  08-Aug-2014  Support for single producer through LOCK_FREE_Q_SINGLE_PRODUCER #define
///      Faustino Frechilla  11-Aug-2014  LOCK_FREE_Q_SINGLE_PRODUCER removed. Single producer handled in template
///      Faustino Frechilla  12-Aug-2014  inheritance (specialisation) based on templates.
///      Faustino Frechilla  10-Aug-2015  Ported to c++11. Removed volatile keywords (using std::atomic)
/// @endhistory
/// 
// ============================================================================

#ifndef _LOCK_FREE_QUEUE_H__
#define _LOCK_FREE_QUEUE_H__

#include <stdint.h>     // uint32_t
#include <atomic>

// default Queue size
#define LOCK_FREE_Q_DEFAULT_SIZE 65536 // (2^16)

// define this macro if calls to "size" must return the real size of the 
// queue. If it is undefined  that function will try to take a snapshot of 
// the queue, but returned value might be bogus
//#define _WITH_LOCK_FREE_Q_KEEP_REAL_SIZE

// forward declarations for default template values
//
template <typename ELEM_T, uint32_t Q_SIZE>
class ArrayLockFreeQueueSingleProducer;
template <typename ELEM_T, uint32_t Q_SIZE>
class ArrayLockFreeQueueMultipleProducers;


/// @brief Lock-free queue based on a circular array
/// No allocation of extra memory for the nodes handling is needed, but it has 
/// to add extra overhead (extra CAS operation) when inserting to ensure the 
/// thread-safety of the queue when the queue type is not 
/// ArrayLockFreeQueueSingleProducer.
///
/// examples of instantiation:
///   ArrayLockFreeQueue<int> q; // queue of ints of default size (65535 - 1)
///                              // and defaulted to single producer
///   ArrayLockFreeQueue<int, 10000> q;
///                              // queue of ints of size (10000 - 1) and
///                              // defaulted to single producer
///   ArrayLockFreeQueue<int, 100, ArrayLockFreeQueueMultipleProducers> q;
///                              // queue of ints of size (100 - 1) with support
///                              // for multiple producers
///
/// ELEM_T represents the type of elementes pushed and popped from the queue
/// Q_SIZE size of the queue. The actual size of the queue is (Q_SIZE-1)
///        This number should be a power of 2 to ensure 
///        indexes in the circular queue keep stable when the uint32_t 
///        variable that holds the current position rolls over from FFFFFFFF
///        to 0. For instance
///        2    -> 0x02 
///        4    -> 0x04
///        8    -> 0x08
///        16   -> 0x10
///        (...) 
///        1024 -> 0x400
///        2048 -> 0x800
///
///        if queue size is not defined as requested, let's say, for
///        instance 100, when current position is FFFFFFFF (4,294,967,295)
///        index in the circular array is 4,294,967,295 % 100 = 95. 
///        When that value is incremented it will be set to 0, that is the 
///        last 4 elements of the queue are not used when the counter rolls
///        over to 0
/// Q_TYPE type of queue implementation. ArrayLockFreeQueueSingleProducer and 
///        ArrayLockFreeQueueMultipleProducers are supported (single producer
///        by default)
template <
    typename ELEM_T, 
    uint32_t Q_SIZE = LOCK_FREE_Q_DEFAULT_SIZE, 
    template <typename T, uint32_t S> class Q_TYPE = ArrayLockFreeQueueSingleProducer >
class ArrayLockFreeQueue
{
public:    
    /// @brief constructor of the class
    ArrayLockFreeQueue();
    
    /// @brief destructor of the class. 
    /// Note it is not virtual since it is not expected to inherit from this
    /// template
    ~ArrayLockFreeQueue();

    /// @brief returns the current number of items in the queue
    /// It tries to take a snapshot of the size of the queue, but in busy environments
    /// this function might return bogus values. 
    ///
    /// If a reliable queue size must be kept you might want to have a look at 
    /// the preprocessor variable in this header file called '_WITH_LOCK_FREE_Q_KEEP_REAL_SIZE'
    /// it enables a reliable size though it hits overall performance of the queue 
    /// (when the reliable size variable is on it's got an impact of about 20% in time)
    inline uint32_t size();
    
    /// @brief return true if the queue is full. False otherwise
    /// It tries to take a snapshot of the size of the queue, but in busy 
    /// environments this function might return bogus values. See help in method
    /// ArrayLockFreeQueue::size
    inline bool full();

    /// @brief push an element at the tail of the queue
    /// @param the element to insert in the queue
    /// Note that the element is not a pointer or a reference, so if you are using large data
    /// structures to be inserted in the queue you should think of instantiate the template
    /// of the queue as a pointer to that large structure
    /// @return true if the element was inserted in the queue. False if the queue was full
    inline bool push(const ELEM_T &a_data);

    /// @brief pop the element at the head of the queue
    /// @param a reference where the element in the head of the queue will be saved to
    /// Note that the a_data parameter might contain rubbish if the function returns false
    /// @return true if the element was successfully extracted from the queue. False if the queue was empty
    inline bool pop(ELEM_T &a_data);

protected:
    /// @brief the actual queue. methods are forwarded into the real 
    ///        implementation
    Q_TYPE<ELEM_T, Q_SIZE> m_qImpl;

private:
    /// @brief disable copy constructor declaring it private
    ArrayLockFreeQueue<ELEM_T, Q_SIZE, Q_TYPE>(
        const ArrayLockFreeQueue<ELEM_T, Q_SIZE, Q_TYPE> &a_src);
};

/// @brief implementation of an array based lock free queue with support for a
///        single producer
/// This class is prevented from being instantiated directly (all members and
/// methods are private). To instantiate a single producer lock free queue 
/// you must use the ArrayLockFreeQueue fachade:
///   ArrayLockFreeQueue<int, 100, ArrayLockFreeQueueSingleProducer> q;
template <typename ELEM_T, uint32_t Q_SIZE>
class ArrayLockFreeQueueSingleProducer
{
    // ArrayLockFreeQueue will be using this' private members
    template <
        typename ELEM_T_, 
        uint32_t Q_SIZE_, 
        template <typename T_, uint32_t S_> class Q_TYPE>
    friend class ArrayLockFreeQueue;

private:
    /// @brief constructor of the class
    ArrayLockFreeQueueSingleProducer();
    virtual ~ArrayLockFreeQueueSingleProducer();
    
    inline uint32_t size();
    
    inline bool full();
    
    bool push(const ELEM_T &a_data);
    
    bool pop(ELEM_T &a_data);
    
    /// @brief calculate the index in the circular array that corresponds
    /// to a particular "count" value
    inline uint32_t countToIndex(uint32_t a_count);

private:    
    /// @brief array to keep the elements
    ELEM_T m_theQueue[Q_SIZE];

    /// @brief where a new element will be inserted
    std::atomic<uint32_t> m_writeIndex;

    /// @brief where the next element where be extracted from
    std::atomic<uint32_t> m_readIndex;

#ifdef _WITH_LOCK_FREE_Q_KEEP_REAL_SIZE
    /// @brief number of elements in the queue
    std::atomic<uint32_t> m_count;
#endif

private:
    /// @brief disable copy constructor declaring it private
    ArrayLockFreeQueueSingleProducer<ELEM_T, Q_SIZE>(
        const ArrayLockFreeQueueSingleProducer<ELEM_T, Q_SIZE> &a_src);
};

/// @brief implementation of an array based lock free queue with support for 
///        multiple producers
/// This class is prevented from being instantiated directly (all members and
/// methods are private). To instantiate a multiple producers lock free queue 
/// you must use the ArrayLockFreeQueue fachade:
///   ArrayLockFreeQueue<int, 100, ArrayLockFreeQueueMultipleProducers> q;
template <typename ELEM_T, uint32_t Q_SIZE>
class ArrayLockFreeQueueMultipleProducers
{
    // ArrayLockFreeQueue will be using this' private members
    template <
        typename ELEM_T_, 
        uint32_t Q_SIZE_, 
        template <typename T_, uint32_t S_> class Q_TYPE>
    friend class ArrayLockFreeQueue;

private:
    /// @brief constructor of the class
    ArrayLockFreeQueueMultipleProducers();
    
    virtual ~ArrayLockFreeQueueMultipleProducers();
    
    inline uint32_t size();
    
    inline bool full();
    
    bool push(const ELEM_T &a_data);   
    
    bool pop(ELEM_T &a_data);
    
    /// @brief calculate the index in the circular array that corresponds
    /// to a particular "count" value
    inline uint32_t countToIndex(uint32_t a_count);
    
private:    
    /// @brief array to keep the elements
    ELEM_T m_theQueue[Q_SIZE];

    /// @brief where a new element will be inserted
    std::atomic<uint32_t> m_writeIndex;

    /// @brief where the next element where be extracted from
    std::atomic<uint32_t> m_readIndex;
    
    /// @brief maximum read index for multiple producer queues
    /// If it's not the same as m_writeIndex it means
    /// there are writes pending to be "committed" to the queue, that means,
    /// the place for the data was reserved (the index in the array) but  
    /// data is still not in the queue, so the thread trying to read will have 
    /// to wait for those other threads to save the data into the queue
    ///
    /// note this is only used for multiple producers
    std::atomic<uint32_t> m_maximumReadIndex;

#ifdef _WITH_LOCK_FREE_Q_KEEP_REAL_SIZE
    /// @brief number of elements in the queue
    std::atomic<uint32_t> m_count;
#endif

private:
    /// @brief disable copy constructor declaring it private
    ArrayLockFreeQueueMultipleProducers<ELEM_T, Q_SIZE>(
        const ArrayLockFreeQueueMultipleProducers<ELEM_T, Q_SIZE> &a_src);
};

// include implementation files
#include "lock_free_queue_impl.h"
#include "lock_free_queue_impl_single_producer.h"
#include "lock_free_queue_impl_multiple_producer.h"

#endif // _LOCK_FREE_QUEUE_H__
