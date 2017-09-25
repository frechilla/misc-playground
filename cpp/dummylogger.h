// ============================================================================
// Copyright (c) 2017 Faustino Frechilla
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
/// @file dummylogger.h
/// @brief A simple log based on C++ streams
///
/// @author Faustino Frechilla
/// @history
/// Ref       Who                When         What
///           Faustino Frechilla 01-Apr-2017  Original development
/// @endhistory
///
// ============================================================================

#pragma once

#include <iostream>
#include "singleton.h"


/// @brief 
/// This is a singleton class to log messages using C++ streams
///
/// Example of usage:
/// DummyLogger::instance() << "This is a log message" << std::endl;
class DummyLogger : public Singleton<DummyLogger>
{
public:
    /// @brief getStream returns a reference to the stream being used by this 
    ///        logger instance
    /// @return
    inline std::ostream& getStream() const
    {
        return _stream;
    }

    /// @brief operator<< operator used to forward log messages into the output stream
    /// @param pf
    /// @return
    DummyLogger& operator<<(std::ostream& (*pf)(std::ostream&))
    {
        pf(_stream);
        return *this;
    }

private:
    /// @brief the strem where log messages will be forwarded
    std::ostream& _stream;

    DummyLogger(): _stream(std::cout)
    {}
    ~DummyLogger()
    {}

    // usage of "friend" is discouraged, but it is the only way to design a templatized singleton
    friend class Singleton<DummyLogger>;
    
    // template method needed to be able to log strings ending with std::endl
    template <typename T>
    friend DummyLogger& operator<<(DummyLogger& log, T const& val);
   
private:
    // prevent copying of this singleton
    DummyLogger(const DummyLogger& src);
    DummyLogger& operator =(const DummyLogger& src);

}; // class DummyLogger

template <typename T>
DummyLogger& operator<<(DummyLogger& log, T const& val)
{
    log._stream << val;
    return log;
}
