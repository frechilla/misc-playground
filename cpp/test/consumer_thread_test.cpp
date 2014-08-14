// ============================================================================
/// @file  consumer_thread_test.cpp
/// @brief file to test the consumer thread class
/// Compiling procedure:
///   $ g++ -g -O0 -Wall -std=c++11 -D_REENTRANT -c consumer_thread_test.cpp 
///   $ g++ consumer_thread_test.o -o consumer_thread_test
///
/// Expected output: 
///    0ms: consumer1: Called to Init
///    0ms: consumer2: Called to Init2
///  999ms: main: producing: 0
///  999ms: consumer1: Consumed 0
///  999ms: main: producing: 1
///  999ms: consumer2: Consumed: 0
///  999ms: main: producing: 2
///  999ms: consumer2: Consumed: 1
///  999ms: consumer1: Consumed 1
///  999ms: consumer2: Consumed: 2
///  999ms: main: producing: 3
///  999ms: consumer1: Consumed 2
///  999ms: main: producing: 4
///  999ms: consumer2: Consumed: 3
///  999ms: consumer1: Consumed 3
///  999ms: main: producing: 5
///  999ms: consumer2: Consumed: 4
///  999ms: consumer1: Consumed 4
///  999ms: main: producing: 6
///  999ms: consumer2: Consumed: 5
///  999ms: consumer1: Consumed 5
///  999ms: main: producing: 7
///  999ms: consumer2: Consumed: 6
///  999ms: consumer1: Consumed 6
///  999ms: main: producing: 8
///  999ms: consumer2: Consumed: 7
///  999ms: consumer1: Consumed 7
///  999ms: main: producing: 9
///  999ms: consumer2: Consumed: 8
///  999ms: consumer1: Consumed 8
///  999ms: main: producing: 10
///  999ms: consumer2: Consumed: 9
///  999ms: consumer1: Consumed 9
///  999ms: main: producing: 11
///  999ms: consumer2: Consumed: 10
///  999ms: consumer1: Consumed 10
///  999ms: main: producing: 12
///  999ms: consumer2: Consumed: 11
///  999ms: consumer1: Consumed 11
///  999ms: main: producing: 13
///  999ms: consumer2: Consumed: 12
///  999ms: consumer1: Consumed 12
///  999ms: main: producing: 14
///  999ms: consumer2: Consumed: 13
///  999ms: consumer1: Consumed 13
///  999ms: main: producing: 15
///  999ms: consumer2: Consumed: 14
///  999ms: consumer1: Consumed 14
///  999ms: main: producing: 16
///  999ms: consumer2: Consumed: 15
///  999ms: consumer1: Consumed 15
///  999ms: main: producing: 17
///  999ms: consumer2: Consumed: 16
///  999ms: consumer1: Consumed 16
///  999ms: main: producing: 18
///  999ms: consumer2: Consumed: 17
///  999ms: main: producing: 19
///  999ms: consumer2: Consumed: 18
///  999ms: consumer1: Consumed 17
///  999ms: consumer2: Consumed: 19
///  999ms: consumer1: Consumed 18
///  999ms: consumer1: Consumed 19
/// 1999ms: consumer1: Consumed 1000
/// 1999ms: main: thread1 exited
/// 2015ms: main: thread2 exited
/// 2015ms: main: exiting ConsumerThreadTest::run
// ============================================================================

#include <iostream>
#include <chrono>
#include <iomanip> // std::setw
#include <sstream> // std::stringstream
#include <mutex>

#include "safe_queue.h"
#include "consumer_thread.h"

class ConsumerThreadTest
{
public:
    ConsumerThreadTest():
        m_startTestTime(std::chrono::seconds(0))
    {}
    
    virtual ~ConsumerThreadTest()
    {}

    void Init()
    {
        timedPrint("consumer1", "Called to Init");
    }

    void Consume(int a_data)
    {
        std::stringstream strStream;        
        strStream << "Consumed " << a_data;
        
        timedPrint("consumer1", strStream.str().c_str());
    }

    void Init2()
    {
        timedPrint("consumer2", "Called to Init2");
    }

    void Consume2(int a_data)
    {
        std::stringstream strStream;
        strStream << "Consumed: " << a_data;
        
        timedPrint("consumer2", strStream.str().c_str());
    }

    int run();
    
private:
    std::chrono::system_clock::time_point m_startTestTime;
    std::mutex m_printMutex;
    
    void timedPrint(const char* a_who, const char* a_msg)
    {
        auto elapsed = std::chrono::system_clock::now() - m_startTestTime;
        
        std::unique_lock<std::mutex> lk(m_printMutex);
        std::cout << std::setw(5) 
                  << std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count()
                  << "ms: " << a_who << ": " << a_msg << std::endl;
        std::cout.flush();
    }
};

int main()
{
	ConsumerThreadTest theConsumerThreadTest;
    int theConsumerThreadTestResult;
    
	theConsumerThreadTestResult = theConsumerThreadTest.run();

	return theConsumerThreadTestResult;
}

int ConsumerThreadTest::run()
{
    m_startTestTime = std::chrono::system_clock::now();
    
    // a consumer thread with the default queue size
    ConsumerThread<int> thread1(
            std::bind(&ConsumerThreadTest::Consume, this, std::placeholders::_1),
            std::bind(&ConsumerThreadTest::Init, this));

    // a consumer thread with queue size = 1
    ConsumerThread<int> thread2(
            1,
            std::bind(&ConsumerThreadTest::Consume2, this, std::placeholders::_1),
            std::bind(&ConsumerThreadTest::Init2, this));

    std::this_thread::sleep_for(std::chrono::seconds(1));

    for (int i = 0 ; i < 20 ; i++)
    {
        std::stringstream strStream;
        strStream << "producing: " << i;
        timedPrint("main", strStream.str().c_str());
        
        thread1.Produce(i);
        thread2.Produce(i);
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));

    thread1.Produce(1000);

    thread1.Join();
    timedPrint("main", "thread1 exited");
    thread2.Join();
    timedPrint("main", "thread2 exited");
    
    // we joined the consumer threads. Calling to any of these functions now
    // will assert the test
    //thread1.Produce(1001);
    //thread2.Produce(1001);

    timedPrint("main", "exiting ConsumerThreadTest::run");
    
    return 0;
}
