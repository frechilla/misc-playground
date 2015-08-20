// ============================================================================
/// @file  lock_free_multiple_producers_q_test.cpp
/// @brief Testing the circular array based lock free queue implementation
///        Multiple producers implementation
/// Compiling procedure:
///   $ g++ -g -O0 -Wall -std=c++11 -D_REENTRANT -c lock_free_queue_test.cpp 
///   $ g++ lock_free_queue_test.o -o lock_free_queue_test -pthread -std=c++11
///
/// Expected output: 
///    0ms: main: About to create 3 consumers and 3 producers
///    0ms: main: About to call join on the producers...
/// 1000ms: producer2: About to fill up the queue
/// 1000ms: producer3: About to fill up the queue
/// 1000ms: producer1: About to fill up the queue
/// 1000ms: producer3: trying to push 2 more elements. Queue is full
/// 1000ms: producer1: trying to push 2 more elements. Queue is full
/// 1000ms: producer2: trying to push 2 more elements. Queue is full
/// 3000ms: consumer2: About to empty out the queue
/// 3000ms: consumer2: Sleeping for a second before popping the queue
/// 3000ms: consumer3: About to empty out the queue
/// 3000ms: consumer3: Sleeping for a second before popping the queue
/// 3000ms: consumer1: About to empty out the queue
/// 3000ms: consumer1: Sleeping for a second before popping the queue
/// 4000ms: consumer2: Sleeping for a second before popping the queue
/// 4000ms: producer2: Success!. Someone must have popped the queue
/// 4000ms: consumer1: Sleeping for a second before popping the queue
/// 4000ms: consumer3: Sleeping for a second before popping the queue
/// 4000ms: producer1: Success!. Someone must have popped the queue
/// 4000ms: producer3: Success!. Someone must have popped the queue
/// 5000ms: consumer2: Sleeping for a second before popping the queue
/// 5000ms: consumer1: Sleeping for a second before popping the queue
/// 5000ms: producer1: Success!. Someone must have popped the queue
/// 5000ms: producer1: Done!
/// 5000ms: producer3: Success!. Someone must have popped the queue
/// 5000ms: producer3: Done!
/// 5000ms: consumer3: Sleeping for a second before popping the queue
/// 5000ms: producer2: Success!. Someone must have popped the queue
/// 5000ms: producer2: Done!
/// 5000ms: main: Producer threads are done. About to sleep for 10 seconds...
/// 6000ms: consumer2: Sleeping for a second before popping the queue
/// 6000ms: consumer1: Sleeping for a second before popping the queue
/// 6000ms: consumer3: Sleeping for a second before popping the queue
/// 7000ms: consumer2: Sleeping for a second before popping the queue
/// 7000ms: consumer1: Sleeping for a second before popping the queue
/// 7000ms: consumer3: Sleeping for a second before popping the queue
/// 8000ms: consumer2: About to pop another element
/// 8000ms: consumer2: done popping
/// 8000ms: consumer2: About to pop another element
/// 8000ms: consumer2: done popping
/// 8000ms: consumer2: done waiting on empty queue
/// 8000ms: consumer2: Looping on an empty queue until someone pushes something
/// 8000ms: consumer2: Done!
/// 8000ms: consumer1: About to pop another element
/// 8000ms: consumer1: done popping
/// 8000ms: consumer1: About to pop another element
/// 8000ms: consumer1: done popping
/// 8000ms: consumer1: done waiting on empty queue
/// 8000ms: consumer1: Looping on an empty queue until someone pushes something
/// 8000ms: consumer1: Done!
/// 8000ms: consumer3: About to pop another element
///15000ms: main: About to push something in the queue to let the consumers finish
///16001ms: consumer3: done popping
///16001ms: consumer3: About to pop another element
///16001ms: consumer3: done popping
///16001ms: consumer3: done waiting on empty queue
///16001ms: consumer3: Looping on an empty queue until someone pushes something
///16001ms: consumer3: Done!
///16001ms: main: Done!
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

const std::string CONSUMER1 = std::string("consumer1");
const std::string CONSUMER2 = std::string("consumer2");
const std::string CONSUMER3 = std::string("consumer3");

const std::string PRODUCER1 = std::string("producer1");
const std::string PRODUCER2 = std::string("producer2");
const std::string PRODUCER3 = std::string("producer3");

class ArrayLockFreeQueueTest
{
public:

    typedef ArrayLockFreeQueue<
        int, 
        QUEUE_SIZE + 1,
        ArrayLockFreeQueueMultipleProducers> TestQueueType_t;

    ArrayLockFreeQueueTest():
        m_queue(),
        m_startTestTime(std::chrono::seconds(0)),
        m_printMutex()
    {}
    
    virtual ~ArrayLockFreeQueueTest()
    {}
    
    int run()
    {
        int data;
        m_startTestTime = std::chrono::system_clock::now();
        
        timedPrint("main", "About to create 3 consumers and 3 producers");
        m_producerThread1.reset(new std::thread(
            std::bind(&ArrayLockFreeQueueTest::runProducer, this, PRODUCER1)));
        m_producerThread2.reset(new std::thread(
            std::bind(&ArrayLockFreeQueueTest::runProducer, this, PRODUCER2)));
        m_producerThread3.reset(new std::thread(
            std::bind(&ArrayLockFreeQueueTest::runProducer, this, PRODUCER3)));

        m_consumerThread1.reset(new std::thread(
            std::bind(&ArrayLockFreeQueueTest::runConsumer, this, CONSUMER1)));
        m_consumerThread2.reset(new std::thread(
            std::bind(&ArrayLockFreeQueueTest::runConsumer, this, CONSUMER2)));
        m_consumerThread3.reset(new std::thread(
            std::bind(&ArrayLockFreeQueueTest::runConsumer, this, CONSUMER3)));
        
        timedPrint("main", "About to call join on the producers...");
        m_producerThread1->join();
        m_producerThread2->join();
        m_producerThread3->join();
        
        timedPrint("main", "Producer threads are done. About to sleep for 10 seconds...");
        std::this_thread::sleep_for(std::chrono::seconds(10));
        
        timedPrint("main", "About to push something in the queue to let the consumers finish");
        std::this_thread::sleep_for(std::chrono::seconds(1));
        m_queue.push(0);
        m_queue.push(0);
        m_queue.push(0);
        
        m_consumerThread1->join();
        m_consumerThread2->join();
        m_consumerThread3->join();

        assert(m_queue.pop(data) == false);

        timedPrint("main", "Done!");

        return 0;
    }
    
    const TestQueueType_t& GetReferenceToQueue(){return m_queue;}
    //TestQueueType_t GetCopyOfQueue(){return m_queue;}

private:
    TestQueueType_t m_queue;
    std::chrono::system_clock::time_point m_startTestTime;
    std::mutex m_printMutex;
    
    std::unique_ptr<std::thread> m_producerThread1;
    std::unique_ptr<std::thread> m_producerThread2;
    std::unique_ptr<std::thread> m_producerThread3;
    std::unique_ptr<std::thread> m_consumerThread1;
    std::unique_ptr<std::thread> m_consumerThread2;
    std::unique_ptr<std::thread> m_consumerThread3;

    void runProducer(const std::string &name)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        timedPrint(name.c_str(), "About to fill up the queue");
        for (int i = 0; i < QUEUE_SIZE/3; i++)
        {
            m_queue.push(i);
        }
        
        // wait for m_queue to be full
        while(m_queue.size() != QUEUE_SIZE);
        
        timedPrint(name.c_str(), "trying to push 2 more elements. Queue is full");
        for (int i = 0; i < 2; i++)
        {
            while (m_queue.push(i) == false) 
                ;
            timedPrint(name.c_str(), "Success!. Someone must have popped the queue");
        }
       
        timedPrint(name.c_str(), "Done!");
    }
    
    void runConsumer(const std::string &name)
    {
        int data;
        
        std::this_thread::sleep_for(std::chrono::seconds(3));
        
        timedPrint(name.c_str(), "About to empty out the queue");
        for (int i = 0; i < QUEUE_SIZE/3; i++)
        {
            timedPrint(name.c_str(), "Sleeping for a second before popping the queue");
            std::this_thread::sleep_for(std::chrono::seconds(1));
            while (m_queue.pop(data) == false)
                ;
            
            //assert(data == i); (multiple producers... strict order may not happen)
        }
        
        for (int i = 0; i < 2; i++)
        {
            timedPrint(name.c_str(), "About to pop another element");
            while (m_queue.pop(data) == false)
                ;
            timedPrint(name.c_str(), "done popping");
            
            //assert(data == i); (multiple producers... strict order may not happen)
        }
        
        timedPrint(name.c_str(), "done waiting on empty queue");
        
        timedPrint(name.c_str(), "Looping on an empty queue until someone pushes something");
        while (m_queue.pop(data) == false)
            ;
        timedPrint(name.c_str(), "Done!");
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
    int multipleProducerResult;
    ArrayLockFreeQueueTest multipleProducerTest;

    multipleProducerResult = multipleProducerTest.run();
    
	return multipleProducerResult;
}
