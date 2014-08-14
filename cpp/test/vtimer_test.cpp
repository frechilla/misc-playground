#include <iostream>
#include "vtimer.h"

// compilation: 
// $ g++ -g -O0 -Wall -std=c++11 -D_REENTRANT vtimer_test.cpp -o vtimer_test

class VTimerTest
{
public:
    VTimerTest()
    {}
    
    ~VTimerTest()
    {}
    
    void Callback(uint32_t a_currentTime)
    {
        std::cout << "Callback called at " << 
            static_cast<int>(a_currentTime) << std::endl;
    }
    
    int run(); 
    
private:
};

void GlobalCallback(uint32_t a_currentTime)
{
    std::cout << "Global Callback called at " << 
        static_cast<int>(a_currentTime) << std::endl;
}

int VTimerTest::run()
{    
    VTimer<uint32_t> virtual_timer(
        std::bind(&VTimerTest::Callback, this, std::placeholders::_1), 
        10); // period
    
    // init at 1. Callback should get called on 11 or bigger
    virtual_timer.Update(1); 
    virtual_timer.Update(3);
    // time should not go backwards, but the virtual time does not care as long as it 
    // is bigger than 0
    virtual_timer.Update(2); 
    virtual_timer.Update(3);
    // about to get called
    virtual_timer.Update(10);
    // calback should get called. Next expiration time 21!
    virtual_timer.Update(11);
    // calback should get called. Next expiration time 31!
    virtual_timer.Update(21);
    // calback should get called. Next expiration time 44!
    virtual_timer.Update(34);
    // EXpiration time is calculated from the latest time received (34)
    virtual_timer.Update(41);
    // No Callback call
    virtual_timer.Update(43);
    // Callback!!!
    virtual_timer.Update(44);
    // No callback until 54...
    virtual_timer.Update(44);
    // No callback until 54...
    virtual_timer.Update(44);
    // No callback until 54...
    virtual_timer.Update(53);
    
    // this timer should call the callback EVERY single call to update
    // except when it is called with time set to 0, the first time it's called
    // or when the timer goes backwards
    VTimer<uint32_t> virtual_timer2(
        std::bind(&GlobalCallback, std::placeholders::_1), 
        0); // period
    // no call
    virtual_timer2.Update(0); 
    // no call
    virtual_timer2.Update(0); 
    // call!
    virtual_timer2.Update(1); 
    // no call!
    virtual_timer2.Update(0); 
    // call!
    virtual_timer2.Update(1); 
    // call!
    virtual_timer2.Update(2); 
    // call!
    virtual_timer2.Update(2); 
    // no call!
    virtual_timer2.Update(1); 
    // call!
    virtual_timer2.Update(2); 
    
    return 0;
}

int main()
{
    VTimerTest theTest;
    int theVTimerTestResult;
    
    theVTimerTestResult = theTest.run();
 
    return theVTimerTestResult;
}