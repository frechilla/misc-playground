// ============================================================================
/// @file  consumer_thread_test.cpp
/// @brief file to test the consumer thread class
///
/// @copyright
/// @history
/// Ref        Who                When        What
///            Faustino Frechilla 04-May-2009 Original development
/// @endhistory
///
// ============================================================================

#include <iostream>
#include "consumer_thread.h"
#include "safe_queue.h"
using namespace std;

class ConsumerThreadTest
{
public:
    void Init()
    {
        cout << "Init thread " << endl;
    }

    void Consume(int a_data)
    {
        cout << "Consumed " << a_data << endl;
    }

    void Init2()
    {
        cout << "Init thread (2)" << endl;
    }

    void Consume2(int a_data)
    {
        cout << "Consumed (2): " << a_data << endl;
    }

    void run();
};

int main()
{
	ConsumerThreadTest consumerThreadTest;
	consumerThreadTest.run();

	return 0;
}

void ConsumerThreadTest::run()
{
    // a consumer thread with the default queue size
    ConsumerThread<int> thread1(
            MakeDelegate(this, &ConsumerThreadTest::Consume),
            MakeDelegate(this, &ConsumerThreadTest::Init));

    // a consumer thread with queue size = 1
    ConsumerThread<int> thread2(
            1,
            MakeDelegate(this, &ConsumerThreadTest::Consume2),
            MakeDelegate(this, &ConsumerThreadTest::Init2));

    sleep(1);

    for (int i = 0 ; i < 500 ; i++)
    {
        std::cout << "producing: " << i << std::endl;
        thread1.Produce(i);
        thread2.Produce(i);
    }

    sleep(1);

    thread1.Produce(1000);

    thread1.Join();
    thread2.Join();

    std::cout << "exiting ConsumerThreadTest::LaunchTest" << std::endl;
}
