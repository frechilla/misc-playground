#include <iostream>
#include <assert.h>
#include "singleton.h"

// compilation: 
// $ g++ -g -O0 -Wall -std=c++11 -D_REENTRANT singleton_test.cpp -o singleton_test

class MySingleton :
    public Singleton<MySingleton>
{
public:
    int a;
    int b;
    
private:
    friend class Singleton<MySingleton>;
   
    MySingleton(): a(1), b(2)
    {}    
    virtual ~MySingleton() {}
};

class SingletonTest
{
public:
    // TODO check thread safety of initialisation
    int run(); 
    
private:
};

int SingletonTest::run()
{
    std::cout << "A: " << MySingleton::Instance().a << std::endl;
    std::cout << "B: " << MySingleton::GetPtr()->b << std::endl;
    
    (MySingleton::Instance().a)++;
    (MySingleton::Instance().b)++;
    
    std::cout << "A: " << ++(MySingleton::GetPtr()->a) << std::endl;
    std::cout << "B: " << ++(MySingleton::Instance().b) << std::endl;

    (MySingleton::Instance().a)--;
    (MySingleton::Instance().b)--;
    
    std::cout << "A: " << MySingleton::Instance().a << std::endl;
    std::cout << "B: " << MySingleton::Instance().b << std::endl;
    
    MySingleton::Instance().a = 15;
    MySingleton::Instance().b = 25;
    
    std::cout << "A: " << MySingleton::GetPtr()->a << std::endl;
    std::cout << "B: " << MySingleton::GetPtr()->b << std::endl;
    
    assert(MySingleton::GetPtr() == &(MySingleton::Instance()));
    
    return 0;
}

int main()
{
    SingletonTest theTest;
    int theSingletonTestResult;
    
    theSingletonTestResult = theTest.run();
 
    return theSingletonTestResult;
}