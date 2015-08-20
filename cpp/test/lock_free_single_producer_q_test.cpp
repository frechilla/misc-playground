// ============================================================================
/// @file  lock_free_single_producer_q_test.cpp
/// @brief Testing the circular array based lock free queue implementation
///        (single producer implementation)
/// Compiling procedure:
///   $ g++ -g -O0 -Wall -std=c++11 -D_REENTRANT -c lock_free_queue_test.cpp 
///   $ g++ lock_free_queue_test.o -o lock_free_queue_test -pthread -std=c++11
///
/// Expected output: 
///     0ms: main: About to create the consumer and the producer
///    15ms: main: About to call join on the producer...
///  1015ms: producer: About to fill up the queue
///  1015ms: producer: trying to push 5 more elements. Queue is full
///  3015ms: consumer: About to empty out the queue
///  3015ms: consumer: Sleeping for a second before popping the queue
///  4016ms: producer: Success!. Someone must have popped the queue
///  4016ms: consumer: Sleeping for a second before popping the queue
///  5016ms: consumer: Sleeping for a second before popping the queue
///  5016ms: producer: Success!. Someone must have popped the queue
///  6016ms: consumer: Sleeping for a second before popping the queue
///  6016ms: producer: Success!. Someone must have popped the queue
///  7016ms: consumer: Sleeping for a second before popping the queue
///  7016ms: producer: Success!. Someone must have popped the queue
///  8016ms: consumer: Sleeping for a second before popping the queue
///  8016ms: producer: Success!. Someone must have popped the queue
///  8016ms: producer: Done!
///  8016ms: main: Producer thread is done. About to sleep for 10 seconds...
///  9016ms: consumer: Sleeping for a second before popping the queue
/// 10016ms: consumer: Sleeping for a second before popping the queue
/// 11016ms: consumer: Sleeping for a second before popping the queue
/// 12016ms: consumer: Sleeping for a second before popping the queue
/// 13016ms: consumer: About to pop another element
/// 13016ms: consumer: done popping
/// 13016ms: consumer: About to pop another element
/// 13016ms: consumer: done popping
/// 13016ms: consumer: About to pop another element
/// 13016ms: consumer: done popping
/// 13016ms: consumer: About to pop another element
/// 13016ms: consumer: done popping
/// 13016ms: consumer: About to pop another element
/// 13016ms: consumer: done popping
/// 13016ms: consumer: done waiting on empty queue
/// 13016ms: consumer: Looping on an empty queue until someone pushes something
/// 18017ms: main: About to push something in the queue to let the consumer finish
/// 19017ms: consumer: Done!
/// 19017ms: main: Done!
// ============================================================================

#include <iostream>
#include <chrono>
#include <memory>
#include <thread>
#include <mutex>
#include <string>
#include <assert.h>
#include <iomanip> // std::setw
#include "lock_free_queue.h"

#define QUEUE_SIZE 15

class ArrayLockFreeQueueTest
{
public:

    typedef ArrayLockFreeQueue<
        int, 
        QUEUE_SIZE + 1,
        ArrayLockFreeQueueSingleProducer> TestQueueType_t;

    ArrayLockFreeQueueTest():
        m_queue(),
        m_startTestTime(std::chrono::seconds(0)),
        m_printMutex()
    {}
    
    virtual ~ArrayLockFreeQueueTest()
    {}
    
    int run()
    {
        m_startTestTime = std::chrono::system_clock::now();
        
        timedPrint("main", "About to create the consumer and the producer");
        m_producerThread.reset(new std::thread(std::bind(&ArrayLockFreeQueueTest::runProducer, this)));
        m_consumerThread.reset(new std::thread(std::bind(&ArrayLockFreeQueueTest::runConsumer, this)));
        
        timedPrint("main", "About to call join on the producer...");
        m_producerThread->join();
        
        timedPrint("main", "Producer thread is done. About to sleep for 10 seconds...");
        std::this_thread::sleep_for(std::chrono::seconds(10));
        
        timedPrint("main", "About to push something in the queue to let the consumer finish");
        std::this_thread::sleep_for(std::chrono::seconds(1));
        m_queue.push(0);
        
        m_consumerThread->join();
        timedPrint("main", "Done!");

        return 0;
    }
    
    const TestQueueType_t& GetReferenceToQueue(){return m_queue;}
    //TestQueueType_t GetCopyOfQueue(){return m_queue;}

private:
    TestQueueType_t m_queue;
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
            m_queue.push(i);
        }
        
        // m_queue should be full
        assert(m_queue.size() == QUEUE_SIZE);
        
        timedPrint("producer", "trying to push 5 more elements. Queue is full");
        for (int i = 0; i < 5; i++)
        {
            while (m_queue.push(i) == false) 
                ;
            timedPrint("producer", "Success!. Someone must have popped the queue");
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
            while (m_queue.pop(data) == false)
                ;
            
            assert(data == i);
        }
        
        for (int i = 0; i < 5; i++)
        {
            timedPrint("consumer", "About to pop another element");
            assert(m_queue.pop(data) == true);
            timedPrint("consumer", "done popping");
            
            assert(data == i);
        }
        
        assert(m_queue.pop(data) == false);
        timedPrint("consumer", "done waiting on empty queue");
        
        timedPrint("consumer", "Looping on an empty queue until someone pushes something");
        while (m_queue.pop(data) == false)
            ;
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
    int singleProducerResult;
    ArrayLockFreeQueueTest singleProducerTest;

    singleProducerResult = singleProducerTest.run();
    
	return singleProducerResult;
}
