// ============================================================================
/// @file  safe_queue_test.cpp
/// @brief Testing the safe queue implementation
/// Expected output:
/// 
///     0ms: main: About to create the consumer and the producer
///     0ms: main: About to call join on the producer...
///   999ms: producer: About to fill up the queue
///   999ms: producer: trying to push 5 more elements. Queue is full
///  2999ms: consumer: About to empty out the queue
///  2999ms: consumer: Sleeping for a second before popping the queue
///  3999ms: consumer: Sleeping for a second before popping the queue
///  3999ms: producer: Woken up. Someone must have popped the queue
///  4999ms: consumer: Sleeping for a second before popping the queue
///  4999ms: producer: Woken up. Someone must have popped the queue
///  5999ms: consumer: Sleeping for a second before popping the queue
///  5999ms: producer: Woken up. Someone must have popped the queue
///  6999ms: consumer: Sleeping for a second before popping the queue
///  6999ms: producer: Woken up. Someone must have popped the queue
///  7999ms: consumer: Sleeping for a second before popping the queue
///  7999ms: producer: Woken up. Someone must have popped the queue
///  7999ms: producer: Done!
///  7999ms: main: Producer thread is done. About to sleep for 10 seconds...
///  8999ms: consumer: Sleeping for a second before popping the queue
///  9999ms: consumer: Sleeping for a second before popping the queue
/// 10999ms: consumer: Sleeping for a second before popping the queue
/// 11999ms: consumer: Sleeping for a second before popping the queue
/// 12998ms: consumer: About to pop an element with TimedWaitPop
/// 12998ms: consumer: done popping an element with TimedWaitPop
/// 12998ms: consumer: About to pop an element with TimedWaitPop
/// 12998ms: consumer: done popping an element with TimedWaitPop
/// 12998ms: consumer: About to pop an element with TimedWaitPop
/// 12998ms: consumer: done popping an element with TimedWaitPop
/// 12998ms: consumer: About to pop an element with TimedWaitPop
/// 12998ms: consumer: done popping an element with TimedWaitPop
/// 12998ms: consumer: About to pop an element with TimedWaitPop
/// 12998ms: consumer: done popping an element with TimedWaitPop
/// 12998ms: consumer: Waiting one second on an empty queue for a second
/// 14014ms: consumer: done waiting on empty queue
/// 14014ms: consumer: Waiting on an empty queue until someone pushes something
/// 18029ms: main: About to push something in the queue to let the consumer finish
/// 19029ms: consumer: Done!
/// 19029ms: main: Done!
// ============================================================================


#include <iostream>
#include <chrono>
#include <memory>
#include <thread>
#include <string>
#include <assert.h>
#include <iomanip> // std::setw
#include "safe_queue.h"

#define QUEUE_SIZE 10

class SafeQueueTest
{
public:    
    SafeQueueTest():
        m_queue(QUEUE_SIZE),
        m_startTestTime(std::chrono::seconds(0)),
        m_printMutex()
    {}
    
    virtual ~SafeQueueTest()
    {}
    
    int run()
    {
        m_startTestTime = std::chrono::system_clock::now();
        
        timedPrint("main", "About to create the consumer and the producer");
        m_producerThread.reset(new std::thread(std::bind(&SafeQueueTest::runProducer, this)));
        m_consumerThread.reset(new std::thread(std::bind(&SafeQueueTest::runConsumer, this)));
        
        timedPrint("main", "About to call join on the producer...");
        m_producerThread->join();
        
        timedPrint("main", "Producer thread is done. About to sleep for 10 seconds...");
        std::this_thread::sleep_for(std::chrono::seconds(10));
        
        timedPrint("main", "About to push something in the queue to let the consumer finish");
        std::this_thread::sleep_for(std::chrono::seconds(1));
        m_queue.Push(0);
        
        m_consumerThread->join();
        timedPrint("main", "Done!");

        return 0;
    }

private:
    SafeQueue<int> m_queue;
    std::chrono::system_clock::time_point m_startTestTime;
    std::mutex m_printMutex;
    
    std::unique_ptr<std::thread> m_producerThread;
    std::unique_ptr<std::thread> m_consumerThread;

    void runProducer()
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        timedPrint("producer", "About to fill up the queue");
        for (int i = 0; i < QUEUE_SIZE; i++)
        {
            m_queue.Push(i);
        }
        
        // m_queue should be full
        assert(m_queue.TryPush(11) == false);
        
        timedPrint("producer", "trying to push 5 more elements. Queue is full");
        for (int i = 0; i < 5; i++)
        {
            m_queue.Push(i);
            timedPrint("producer", "Woken up. Someone must have popped the queue");
        }
       
        timedPrint("producer", "Done!");
    }
    
    void runConsumer()
    {
        int data;
        
        std::this_thread::sleep_for(std::chrono::seconds(3));
        
        timedPrint("consumer", "About to empty out the queue");
        for (int i = 0; i < QUEUE_SIZE; i++)
        {
            timedPrint("consumer", "Sleeping for a second before popping the queue");
            std::this_thread::sleep_for(std::chrono::seconds(1));
            m_queue.Pop(data);
            
            assert(data == i);
        }
        
        for (int i = 0; i < 5; i++)
        {
            timedPrint("consumer", "About to pop an element with TimedWaitPop");
            assert(
                m_queue.TimedWaitPop(data, std::chrono::microseconds(100)) == true);
            timedPrint("consumer", "done popping an element with TimedWaitPop");
            
            assert(data == i);
        }
        
        timedPrint("consumer", "Waiting one second on an empty queue for a second");
        assert(
            m_queue.TimedWaitPop(data, std::chrono::seconds(1)) == false);
        timedPrint("consumer", "done waiting on empty queue");
        
        timedPrint("consumer", "Waiting on an empty queue until someone pushes something");
        m_queue.Pop(data);
        timedPrint("consumer", "Done!");
    }
    
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


int main(int /*argc*/, char** /*argv*/)
{
    SafeQueueTest theSafeQueueTest;
    int theSafeQueueTestResult;
    
    theSafeQueueTestResult = theSafeQueueTest.run();
    
	return theSafeQueueTestResult;
}
