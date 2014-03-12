// ============================================================================
// Copyright (c) 2009-2013 Faustino Frechilla
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
/// @file  vtimer.h
/// @brief This file contains a virtual timer class.
///
/// This timer calls a callback function when a timeout expires. It does not
/// maintain the current time by itself, an object of this class needs
/// to be updated with the current time periodically to ensure the callback
/// is called as expected.
/// An object of this class is not thead-safe by itself. If used in a
/// multi-thread system it will need to be protected from outside
///
/// Your compiler must have support for c++11. Example of usage:
///
/// void MyCallback(uint32_t a_currentTime);
/// /* ... */
///
/// Timer<uint32_t> virtual_timer(
///     std::bind(&GlobalCallback, std::placeholders::_1), 15);
///  virtual_timer.Update(0);
///  virtual_timer.Update(30);
/// /* ... */
///
/// @author Faustino Frechilla
/// @history
/// Ref        Who                When        What
///            Faustino Frechilla 12-Jun-2009 Original development
///            Faustino Frechilla 06-Jul-2013 Ported to c++11. Templates
/// @endhistory
///
// ============================================================================

#ifndef _VTIMER_H_
#define _VTIMER_H_

#include <stdint.h>   // types (uint64_t...)
#include <functional> // std::function
#include <assert.h>

/// @brief a Virtual Timer
/// Calls a callback function when a timeout expires. It does not
/// have a real timer by itself, an object of this class needs to be updated 
/// with the current time periodically to ensure the callback is called as 
/// expected.
///
/// This class is templatized so the time type depends on the particular
/// instantiation. Bear in mind TIME_TYPE must have at least support
/// for copy constructor, operator=, operator>=, operator== and operator+ 
/// and it should also be able to be initialised with a 0: "TIME_TYPE obj(0)"
template <typename TIME_TYPE>
class VTimer
{
public:
    typedef std::function<void(const TIME_TYPE&)> VTimerCallback_t;

    /// @brief constructor of a virtual timer
    /// @param a_callback Callback function that will get called on expiration
    /// @param period of time between calls. It must be greater or equal to 0
    VTimer(VTimerCallback_t a_callback, TIME_TYPE a_period):
        m_callback(a_callback),
        m_nextExpiryTime(0),
        m_period(a_period)
    {
        assert(a_period >= 0);
    }

    /// @brief destructor
    virtual ~VTimer()
    {}

    /// @brief update the current time
    /// If the timeout expires the callback function will get called from the
    /// same thread that updated the current time with the current time
    /// The expiration time is calculated the first time this function
    /// is called based on the parameter "a_current". (the first expiration
    /// time will be set to a_current + m_period)
    /// 
    /// The callback won't get called under the following circumstances:
    ///   - The first time Update is called the internal attributes are 
    ///     initialised (but the callback is not called)
    ///   - When "a_currentTime" is set to 0 the callback won't ever get called
    ///   - If "a_currentTime" goes backwards in time
    /// @param a_currentTime
    inline void Update(TIME_TYPE a_currentTime)
    {
        assert(a_currentTime >= 0);

        if (m_nextExpiryTime == 0)
        {
            if ((a_currentTime + m_period) == 0)
            {
                // corner case. Period is "0" and Update is initialised with
                // time 0. m_nextExpiryTime is initalised to 1
                m_nextExpiryTime = 1;
            }
            else
            {
                m_nextExpiryTime = a_currentTime + m_period;
            }
        }
        else if (a_currentTime >= m_nextExpiryTime)
        {
            m_nextExpiryTime = a_currentTime + m_period;
            m_callback(a_currentTime);
        }
    }

private:
    /// a pointer to the callback function
    VTimerCallback_t m_callback;
    
    /// timestamp of the next time the callback function will get called
    TIME_TYPE m_nextExpiryTime;
    
    /// the timeout period
    TIME_TYPE m_period;
};

#endif /* _VTIMER_H_ */
