/// This file defines a particular delegate depending on the value of
/// N_DELEGATE_ARGS. If it is 1 it will define a delegate with 1 parameter,
/// if it is 2 a delegate with 2 parameters and so on.
/// It also defines MakeDelegate global functions to be used when using the delegate
/// pattern

#if !defined (N_DELEGATE_ARGS)
    #error number of arguments needed to compile the DelegateN.h file
#elif N_DELEGATE_ARGS==1
    #define DELEGATE_TEMPLATE_ARGS class Arg1
    #define DELEGATE_FUNCTION_ARGS Arg1 p1
#elif N_DELEGATE_ARGS==2
    #define DELEGATE_TEMPLATE_ARGS class Arg1, class Arg2
    #define DELEGATE_FUNCTION_ARGS Arg1 p1, Arg2 p2
#elif N_DELEGATE_ARGS==3
    #define DELEGATE_TEMPLATE_ARGS class Arg1, class Arg2, class Arg3
    #define DELEGATE_FUNCTION_ARGS Arg1 p1, Arg2 p2, Arg3 p3
#elif N_DELEGATE_ARGS==4
    #define DELEGATE_TEMPLATE_ARGS class Arg1, class Arg2, class Arg3, class Arg4
    #define DELEGATE_FUNCTION_ARGS Arg1 p1, Arg2 p2, Arg3 p3, Arg4 p4
#elif N_DELEGATE_ARGS==5
    #define DELEGATE_TEMPLATE_ARGS class Arg1, class Arg2, class Arg3, class Arg4, class Arg5
    #define DELEGATE_FUNCTION_ARGS Arg1 p1, Arg2 p2, Arg3 p3, Arg4 p4, Arg5 p5
#elif N_DELEGATE_ARGS==6
    #define DELEGATE_TEMPLATE_ARGS class Arg1, class Arg2, class Arg3, class Arg4, class Arg5, class Arg6
    #define DELEGATE_FUNCTION_ARGS Arg1 p1, Arg2 p2, Arg3 p3, Arg4 p4, Arg5 p5, Arg6 p6
#elif N_DELEGATE_ARGS==7
    #define DELEGATE_TEMPLATE_ARGS class Arg1, class Arg2, class Arg3, class Arg4, class Arg5, class Arg6, class Arg7
    #define DELEGATE_FUNCTION_ARGS Arg1 p1, Arg2 p2, Arg3 p3, Arg4 p4, Arg5 p5, Arg6 p6, Arg7 p7
#elif N_DELEGATE_ARGS==8
    #define DELEGATE_TEMPLATE_ARGS class Arg1, class Arg2, class Arg3, class Arg4, class Arg5, class Arg6, class Arg7, class Arg8
    #define DELEGATE_FUNCTION_ARGS Arg1 p1, Arg2 p2, Arg3 p3, Arg4 p4, Arg5 p5, Arg6 p6, Arg7 p7, Arg8 p8
#else
    #error the maximum number of arguments to compile the DelegateN.h file is 8
#endif


template<typename RETURN_TYPE, DELEGATE_TEMPLATE_ARGS>
class Delegate < RETURN_TYPE ( DELEGATE_FUNCTION_ARGS ) > :
    public fastdelegate::FastDelegate < RETURN_TYPE ( DELEGATE_FUNCTION_ARGS ) >
{
public:
    typedef fastdelegate::FastDelegate < RETURN_TYPE ( DELEGATE_FUNCTION_ARGS ) > Base_t;

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
    Delegate (Y *pObject, RETURN_TYPE (X::*xMethod) (DELEGATE_FUNCTION_ARGS)) :
        Base_t(pObject, xMethod)
    {}

    // The same as above but for const members
    template <class X, class Y>
    Delegate (Y *pObject, RETURN_TYPE (X::*xMethod) (DELEGATE_FUNCTION_ARGS) const) :
        Base_t(pObject, xMethod)
    {}

    // The same as above but for functions that are not part of any class
    Delegate (RETURN_TYPE (*function) (DELEGATE_FUNCTION_ARGS)) :
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
        // it uses twice the operator ! inside the
        return !(static_cast<const Base_t*>(this)->empty());
    }
};

///////////////////////////////////////
// Global functions to create delegates

// create a delegate to a non-const function
template <class X, class Y, DELEGATE_TEMPLATE_ARGS, typename RETURN_TYPE>
Delegate <RETURN_TYPE (DELEGATE_FUNCTION_ARGS)> MakeDelegate ( Y * pObject, RETURN_TYPE (X::*XMethod)(DELEGATE_FUNCTION_ARGS) )
{
    return Delegate <RETURN_TYPE (DELEGATE_FUNCTION_ARGS)>(pObject, XMethod);    // return Delegate on the stack
}

// create a delegate to a const function
template <class X, class Y, DELEGATE_TEMPLATE_ARGS, typename RETURN_TYPE>
Delegate <RETURN_TYPE (DELEGATE_FUNCTION_ARGS)> MakeDelegate ( const Y * pObject, RETURN_TYPE (X::*XConstMethod)(DELEGATE_FUNCTION_ARGS) const )
{
    return Delegate <RETURN_TYPE (DELEGATE_FUNCTION_ARGS)>(pObject, XConstMethod);    // return Delegate on the stack
}

// create a delegate to a static function (without object)
template <DELEGATE_TEMPLATE_ARGS, typename RETURN_TYPE>
Delegate <RETURN_TYPE (DELEGATE_FUNCTION_ARGS)> MakeDelegate ( RETURN_TYPE (*Function)(DELEGATE_FUNCTION_ARGS) )
{
    return Delegate <RETURN_TYPE (DELEGATE_FUNCTION_ARGS)>(Function);    // return Delegate on the stack
}


#undef DELEGATE_TEMPLATE_ARGS
#undef DELEGATE_FUNCTION_ARGS
