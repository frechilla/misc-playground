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
/// @brief
///
/// @author Faustino Frechilla
/// @history
/// Ref       Who                When         What
///           Faustino Frechilla 7-Apr-2010  Original development
/// @endhistory
///
// ============================================================================

#ifndef _SINGLETON_H_
#define _SINGLETON_H_

/// @brief Inheriting from this class you can make your class a singleton
///        so it can only be instantiated once
template <class TClass>
class Singleton
{
public:
    static TClass& Instance()
    {
        if (m_instancePtr == 0)
        {
            static TClass instance;
            m_instancePtr = &instance;
        }

        return *m_instancePtr;
    }

    ///@brief returns a pointer to the instance.
    /// If the singleton has not been initialised it will return NULL
    static TClass* GetPtr()
    {
#ifdef DEBUG
        assert(m_instancePtr != 0);
#endif

        return m_instancePtr;
    }

protected:
    Singleton(){};
    virtual ~Singleton(){};

    static TClass *m_instancePtr;
};

template <class TClass> TClass* Singleton<TClass>::m_instancePtr = 0;

#endif // _SINGLETON_H_
