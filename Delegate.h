/// @file    Delegate.h
/// @brief   This file contains the definition of the Delegate class
///
/// This file defines the Delegate pattern.
/// http://en.wikipedia.org/wiki/Delegation_pattern
/// the delegation pattern is a technique where an object outwardly expresses
/// certain behaviour but in reality delegates responsibility for implementing
/// that behavior to an associated object in an Inversion of Responsibility
///
/// It defines delegates from 0 arguments up to 8. Example of use
///
/// // This defines a delegate that returns void and receives an UInt16 and a std::string
/// Delegate<void(UInt16, std::string)> delegateFunction;
/// // This call assigns the delegate to the method f defined in the class A. It will
/// // be called in the context of the object 'obj'
/// delegateFunction = MakeDelegate(&obj, &A::f);
/// // Actual call to the delegate
/// delegateFunction(0, std::string("Hello world"));


#ifndef DELEGATE_H_
#define DELEGATE_H_

#include "detail/FastDelegate.h"

// Define Delegate as a template to be able to use the same name for
// all the different Delegate templates (there is one template per number
// of parameters)
template <typename Signature>
class Delegate;

///@brief Delegate for functions with 0 arguments
template<typename RETURN_TYPE>
class Delegate < RETURN_TYPE ( ) > :
    public fastdelegate::FastDelegate < RETURN_TYPE ( ) >
{
public:
    typedef fastdelegate::FastDelegate < RETURN_TYPE ( ) > Base_t;

    // default constructor&destructor
    Delegate ()
    {}
    ~Delegate()
    {}

    // copy constructor
    Delegate (const Delegate& source) :
        Base_t(source)
    {}

    // RETURN_TYPE (X::*xMethod) (void) is the C++ way of describing a pointer to the function xMethod in the class X
    template <class X, class Y>
    Delegate (Y *pObject, RETURN_TYPE (X::*XMethod) ()) :
        Base_t(pObject, XMethod)
    {}

    // The same as above but for const members
    template <class X, class Y>
    Delegate (Y *pObject, RETURN_TYPE (X::*XConstMethod) () const) :
        Base_t(pObject, XConstMethod)
    {}

    // The same as above but for functions that are not part of any class
    Delegate (RETURN_TYPE (*function) ()) :
        Base_t(function)
    {}

    // copying delegates
    Delegate& operator= (const Base_t &source)
    {
        *static_cast<Base_t*>(this) = source;
        return *this;
    }

    // operator to check if the Delegate is valid. It would be the same as !(!Delegate)
    // so using !Delegate is faster
    operator bool () const
    {
        return !(static_cast<const Base_t*>(this)->empty());
    }
};

///////////////////////////////////////
// Global functions to create delegates

// create a delegate to a non-const function
template <class X, class Y, typename RETURN_TYPE>
Delegate <RETURN_TYPE ()> MakeDelegate ( Y * pObject, RETURN_TYPE (X::*XMethod)() )
{
    return Delegate <RETURN_TYPE ()>(pObject, XMethod);    // return Delegate on the stack
}

// create a delegate to a const function
template <class X, class Y, typename RETURN_TYPE>
Delegate <RETURN_TYPE ()> MakeDelegate ( const Y * pObject, RETURN_TYPE (X::*XConstMethod)() const )
{
    return Delegate <RETURN_TYPE ()>(pObject, XConstMethod);    // return Delegate on the stack
}

// create a delegate to a static function (without object)
template <typename RETURN_TYPE>
Delegate <RETURN_TYPE ()> MakeDelegate ( RETURN_TYPE (*Function)() )
{
    return Delegate <RETURN_TYPE ()>(Function);    // return Delegate on the stack
}

// N_DELEGATE_ARGS is used as sort of a parameter to include the file detail/DelegateN.h
// depending on that value DelegateN.h will define a Delegate with 1, 2, etc. parameters
// The maximum is 8 since it is the maximum number of parameters defined in FastDelegate.h
#undef  N_DELEGATE_ARGS
#define N_DELEGATE_ARGS 1
#include "detail/DelegateN.h"

#undef  N_DELEGATE_ARGS
#define N_DELEGATE_ARGS 2
#include "detail/DelegateN.h"

#undef  N_DELEGATE_ARGS
#define N_DELEGATE_ARGS 3
#include "detail/DelegateN.h"

#undef  N_DELEGATE_ARGS
#define N_DELEGATE_ARGS 4
#include "detail/DelegateN.h"

#undef  N_DELEGATE_ARGS
#define N_DELEGATE_ARGS 5
#include "detail/DelegateN.h"

#undef  N_DELEGATE_ARGS
#define N_DELEGATE_ARGS 6
#include "detail/DelegateN.h"

#undef  N_DELEGATE_ARGS
#define N_DELEGATE_ARGS 7
#include "detail/DelegateN.h"

#undef  N_DELEGATE_ARGS
#define N_DELEGATE_ARGS 8
#include "detail/DelegateN.h"

#endif /* DELEGATE_H_ */
