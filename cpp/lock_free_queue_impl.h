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
/// @file lock_free_queue_impl.h
/// @brief Implementation of a circular array based lock-free queue
/// See http://www.codeproject.com/Articles/153898/Yet-another-implementation-of-a-lock-free-circular
/// for more info
///
/// @author Faustino Frechilla
/// @history
/// Ref  Who                 When         What
///      Faustino Frechilla  11-Jul-2010  Original development
///      Faustino Frechilla  08-Aug-2014  Support for single producer through LOCK_FREE_Q_SINGLE_PRODUCER #define
///      Faustino Frechilla  11-Aug-2014  LOCK_FREE_Q_SINGLE_PRODUCER removed. Single/multiple producer handled in templates
///      Faustino Frechilla  12-Aug-2014  inheritance (specialisation) based on templates.
/// @endhistory
/// 
// ============================================================================

#ifndef __LOCK_FREE_QUEUE_IMPL_H__
#define __LOCK_FREE_QUEUE_IMPL_H__

#include <assert.h> // assert()

template <
    typename ELEM_T, 
    uint32_t Q_SIZE, 
    template <typename T, uint32_t S> class Q_TYPE>
ArrayLockFreeQueue<ELEM_T, Q_SIZE, Q_TYPE>::ArrayLockFreeQueue():
    m_qImpl()
{
}

template <
    typename ELEM_T, 
    uint32_t Q_SIZE, 
    template <typename T, uint32_t S> class Q_TYPE>
ArrayLockFreeQueue<ELEM_T, Q_SIZE, Q_TYPE>::~ArrayLockFreeQueue()
{
}

template <
    typename ELEM_T, 
    uint32_t Q_SIZE, 
    template <typename T, uint32_t S> class Q_TYPE>
inline uint32_t ArrayLockFreeQueue<ELEM_T, Q_SIZE, Q_TYPE>::size()
{
    return m_qImpl.size();
}  

template <
    typename ELEM_T, 
    uint32_t Q_SIZE, 
    template <typename T, uint32_t S> class Q_TYPE>
inline bool ArrayLockFreeQueue<ELEM_T, Q_SIZE, Q_TYPE>::full()
{
    return m_qImpl.full();
}  

template <
    typename ELEM_T, 
    uint32_t Q_SIZE, 
    template <typename T, uint32_t S> class Q_TYPE>
inline bool ArrayLockFreeQueue<ELEM_T, Q_SIZE, Q_TYPE>::push(const ELEM_T &a_data)
{
    return m_qImpl.push(a_data);
}

template <
    typename ELEM_T, 
    uint32_t Q_SIZE, 
    template <typename T, uint32_t S> class Q_TYPE>
inline bool ArrayLockFreeQueue<ELEM_T, Q_SIZE, Q_TYPE>::pop(ELEM_T &a_data)
{
    return m_qImpl.pop(a_data);
}

#endif // __LOCK_FREE_QUEUE_IMPL_H__
