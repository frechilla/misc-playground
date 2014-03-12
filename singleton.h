// ============================================================================
// Copyright (c) 2010-2014 Faustino Frechilla
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
/// @file singleton.h
/// @brief A template to create singleton classes
///
/// This file contains a template that can be used to turn your class into a
/// singleton only by inheritance
///
/// Your compiler must have support for c++11
///
/// @author Faustino Frechilla
/// @history
/// Ref       Who                When         What
///           Faustino Frechilla 07-Apr-2010  Original development
///           Faustino Frechilla 12-Mar-2014  Thread-safe using c++11
/// @endhistory
///
// ============================================================================

#ifndef _SINGLETON_H_
#define _SINGLETON_H_

#include <atomic>

/// @brief A templatised class for singletons
/// Inherit from this class if you wish to make your class a singleton, so only
/// an instance of it is created. The instance must be accessed using the
/// Instance method. Example of usage:
/// 
/// // using the template to "singletonize" your class
/// class MySingleton: public Singleton<MySingleton>
/// { 
/// public:
///     /* ... */
///     void MyMethod();
///     /* ... */
/// private:
///     /* ... */     
///     friend class Singleton<MySingleton>;
///     MySingleton();
///     ~MySingleton();
/// };
///
/// /* ... */
///
/// // accessing your brand new singleton
/// MySingleton::Instance().MyMethod();
/// 
template <class TClass>
class Singleton
{
public:
    /// @brief Gives access to the instance wrapped by this singleton
    /// @return a reference to the instance wrapped by the singleton
    static TClass& Instance()
    {
        if (m_instancePtr == 0)
        {
            // In the rare event that two threads come into this section only 
            // one will acquire the spinlock and build the actual instance
            while(std::atomic_exchange_explicit(&m_lock, true, std::memory_order_acquire))
            {
                ; // spin until acquired
            }
            
            if (m_instancePtr == 0)
            {
                // This is the thread that will build the real instance since
                // it hasn-t been instantiated yet
                m_instancePtr = new TClass();
            }
            
            // Release spinlock
            std::atomic_store_explicit(&m_lock, false, std::memory_order_release);
        }

        return *m_instancePtr;
    }

    /// @brief Gives access to the singleton instance using a pointer
    /// @return a pointer to the instance wrapped by the singleton
    static TClass* GetPtr()
    {
        return &(Singleton<TClass>::Instance());
    }

protected:
    Singleton(){};
    virtual ~Singleton(){};
    
    // the actual instance wrapped around the singleton
    static TClass *m_instancePtr;
    // a spinlock to make this singleton implementation thread-safe
    static std::atomic<bool> m_lock;
};

template <class TClass> TClass* Singleton<TClass>::m_instancePtr = 0;
template <class TClass> std::atomic<bool> Singleton<TClass>::m_lock(false);

#endif /* _SINGLETON_H_ */
