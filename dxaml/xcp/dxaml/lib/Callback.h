// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Defines a set of class and function templates that implement a C++
//      callback mechanism.
//
//      These callbacks capture a function pointer (possibly member function),
//      function arguments, and a target object (for a member function pointer).
//
//      The function can later be invoked without needing to pass arguments or
//      a target object, since these have been captured by the callback object.
//
//      Client code creates callbacks by calling one of the overloaded 
//      MakeCallback() function templates. Template arguments can typically
//      be deduced.
// 
//      MakeCallback() returns an ICallback pointer. The callback is later 
//      invoked by calling ICallback::Invoke().

#pragma once

namespace DirectUI
{

//-----------------------------------------------------------------------------
//
// All callback objects implement this interface.
//
// The callback object keeps all state needed to invoke the callback. Managing
// the lifetime of the callback object is critical, since it may be managing the
// lifetime of other objects it has captured.
//
// ICallback implements IUnknown purely for lifetime. This allows ICallback 
// pointers to be managed by COM smart pointer classes like ComPtr.
//     
//-----------------------------------------------------------------------------
class ICallback : public ctl::implements<IUnknown>
{
public:
    //-----------------------------------------------------------------------------
    //
    // Invoke this callback.
    //
    // This can fail for two reasons:
    //    - Coercing the captured arguments to the callback function's arguments
    //      can fail. For instance, the callback may have captured a parameter
    //      by weak reference. If the referent is no longer alive, resolving the 
    //      weak reference will fail, and this will cause Invoke() to fail.
    //
    //   - More commonly, Invoke() fails if a failure is returned by the wrapped
    //     function pointer.
    //     
    //-----------------------------------------------------------------------------
    virtual _Check_return_ HRESULT Invoke() = 0;

protected:
    ~ICallback() override { }
};

//-----------------------------------------------------------------------------
//
// Callbacks capture arguments and target objects of member function pointers.
// 
// Coercion allows the captured types to differ from the types expected by
// the function this callback represents. For instance, this allows capturing
// a weak reference by typing the callback function to take a strong reference.
//
// Coercion is implemented as a base template Coercion<From, To> and a set of
// partial or complete specializations. 
//
// An instance of Coercion represents the stateful result of a coercion. The
// Coercion instance must be valid as long as the result of the coercion is
// in use.
//
// The base template will coerce any pair of types where To is 
// copy-constructable from From.
//     
//-----------------------------------------------------------------------------
template <typename From, typename To>
struct Coercion
{
    To      result;
    HRESULT hr;

    Coercion(From& value) :
        result(value),
        hr(S_OK)
    {
    }

    bool ShouldNoOp() { return false; }
};

//-----------------------------------------------------------------------------
//
// Specialization for no-op coercion (where From and To are the same type).
// This specialization ensures that references are used and avoids extra copies.
//     
//-----------------------------------------------------------------------------
template <typename T>
struct Coercion<T, T>
{
    T&      result;
    HRESULT hr;

    Coercion(T& value) :
        result(value),
        hr(S_OK)
    {
    }

    bool ShouldNoOp() { return false; }
};

//-----------------------------------------------------------------------------
//
// Specialization to coerce ctl::ComPtr<T> into T*.
//
// This allows a callback to capture an argument using a ComPtr<T> (thereby 
// controlling the lifetime of the argument) but ultimately invoke a function
// that takes a T*.
//     
//-----------------------------------------------------------------------------
template <typename T>
struct Coercion<ctl::ComPtr<T>, T*>
{
    T*      result;
    HRESULT hr;

    Coercion(ctl::ComPtr<T>& value) :
        result(value.Get()),
        hr(S_OK)
    {
    }

    bool ShouldNoOp() { return false; }
};

//-----------------------------------------------------------------------------
//
// Similar to above, specialization to coerce Microsoft::WRL::ComPtr<T> into T*.
//     
//-----------------------------------------------------------------------------
template <typename T>
struct Coercion<Microsoft::WRL::ComPtr<T>, T*>
{
    T*      result;
    HRESULT hr;

    Coercion(Microsoft::WRL::ComPtr<T>& value) :
        result(value.Get()),
        hr(S_OK)
    {
    }

    bool ShouldNoOp() { return false; }
};

//-----------------------------------------------------------------------------
//
// WeakRefTypeTraits allows resolving a weak reference into a strong reference 
// whose type does not have an associated IID.
//
// When a weak reference is resolved an IID must be supplied. For example, a weak 
// reference may be resolved into a strong reference to an IFrameworkElement.
//
// However, if a strong reference to FrameworkElement is desired, the weak reference
// must first be resolved to an IFrameworkElement, then the IFrameworkElement*
// must be cast to a FrameworkElement.
//
// WeakRefTypeTraits is used to supply the additional bit of type information.
// In effect, it says, to resolve a weak reference into a strong reference
// to TargetType, you must first resolve as WeakRefTypeTraits<TargetType>::ResolveType,
// then cast ResolveType to TargetType.
//     
//-----------------------------------------------------------------------------
template <typename TargetType>
struct WeakRefTypeTraits
{
    typedef TargetType ResolveType;
};

//-----------------------------------------------------------------------------
//
// Coerces a WeakRefPtr into a strong ref - ComPtr<T>.
//     
//-----------------------------------------------------------------------------
template <typename T>
struct Coercion<ctl::WeakRefPtr, ctl::ComPtr<T>>
{
    ctl::ComPtr<T> result;
    HRESULT        hr;

    Coercion(ctl::WeakRefPtr& value)
    {
        ctl::ComPtr<WeakRefTypeTraits<T>::ResolveType> spResolved;
    
        hr = value.As(&spResolved);
        if (SUCCEEDED(hr))
        {
            result = static_cast<T*>(spResolved.Get());
        }
    }

    bool ShouldNoOp() { return false; }
};

//-----------------------------------------------------------------------------
//
// Coerces a WeakRefPtr into a strong ref - T*. The strong reference is kept
// alive by the instance of Coercion, so the T* must be used within the
// scope of the Coercion object's lifetime.
//     
//-----------------------------------------------------------------------------
template <typename T>
struct Coercion<ctl::WeakRefPtr, T*>
{
    Coercion<ctl::WeakRefPtr, ctl::ComPtr<T>> intermediate;

    T*             result;
    HRESULT        hr;

    Coercion(ctl::WeakRefPtr& value) :
        intermediate(value),
        result(intermediate.result.Get()),
        hr(intermediate.hr)
    {
    }

    bool ShouldNoOp() { return result == nullptr; }
};

//-----------------------------------------------------------------------------
//
// Callback object for a non-member function with no arguments.
//     
//-----------------------------------------------------------------------------
struct FunctionCallback0 : public ICallback
{
    HRESULT (*m_pfn)();

    _Check_return_ HRESULT Invoke() override
    {
        return m_pfn();
    }
};

//-----------------------------------------------------------------------------
//
// Creates a callback for a non-member function with no arguments.
//     
//-----------------------------------------------------------------------------
inline ctl::ComPtr<ICallback> MakeCallback(HRESULT (*pfn)())
{
    typedef FunctionCallback0 CallbackObject;

    ctl::ComPtr<CallbackObject> spCallback;
    spCallback.Attach(new CallbackObject());
    if (spCallback)
    {
        spCallback->m_pfn = pfn;
    }
    return spCallback;
}

//-----------------------------------------------------------------------------
//
// Callback object for a non-member function with 1 argument.
//     
//-----------------------------------------------------------------------------
template <typename TArg1, typename TArg1Capture>
struct FunctionCallback1 : public ICallback
{
    HRESULT (*m_pfn)(TArg1);
    TArg1Capture m_arg1Captured;

    _Check_return_ HRESULT Invoke() override
    {
        typedef Coercion<TArg1Capture, TArg1> Arg1Coercion;
        Arg1Coercion arg1Coercion = Arg1Coercion(m_arg1Captured);
        if (FAILED(arg1Coercion.hr))
        {
            return arg1Coercion.hr;
        }
        
        HRESULT hr = m_pfn(arg1Coercion.result);

        return hr;
    }
};

//-----------------------------------------------------------------------------
//
// Creates a callback for a non-member function with 1 argument.
//     
//-----------------------------------------------------------------------------
template <typename TArg1, typename TArg1Capture>
ctl::ComPtr<ICallback> MakeCallback(HRESULT (*pfn)(TArg1), TArg1Capture arg1Captured)
{
    typedef FunctionCallback1<TArg1, TArg1Capture> CallbackObject;

    ctl::ComPtr<CallbackObject> spCallback;
    spCallback.Attach(new CallbackObject());
    if (spCallback)
    {
        spCallback->m_pfn          = pfn;
        spCallback->m_arg1Captured = arg1Captured;
    }
    return spCallback;
}

//-----------------------------------------------------------------------------
//
// Callback object for a non-member function with 2 arguments.
//     
//-----------------------------------------------------------------------------
template <typename TArg1, typename TArg2, typename TArg1Capture, typename TArg2Capture>
struct FunctionCallback2 : public ICallback
{
    HRESULT (*m_pfn)(TArg1, TArg2);
    TArg1Capture m_arg1Captured;
    TArg2Capture m_arg2Captured;

    _Check_return_ HRESULT Invoke() override
    {
        typedef Coercion<TArg1Capture, TArg1> Arg1Coercion;
        Arg1Coercion arg1Coercion = Arg1Coercion(m_arg1Captured);
        if (FAILED(arg1Coercion.hr))
        {
            return arg1Coercion.hr;
        }

        typedef Coercion<TArg2Capture, TArg2> Arg2Coercion;
        Arg2Coercion arg2Coercion = Arg2Coercion(m_arg2Captured);
        if (FAILED(arg2Coercion.hr))
        {
            return arg2Coercion.hr;
        }

        HRESULT hr = m_pfn(arg1Coercion.result, arg2Coercion.result);

        return hr;
    }
};

//-----------------------------------------------------------------------------
//
// Creates a callback for a non-member function with 2 arguments.
//     
//-----------------------------------------------------------------------------
template <typename TArg1, typename TArg2, typename TArg1Capture, typename TArg2Capture>
ctl::ComPtr<ICallback> MakeCallback(HRESULT (*pfn)(TArg1, TArg2), TArg1Capture arg1Captured, TArg2Capture arg2Captured)
{
    typedef FunctionCallback2<TArg1, TArg2, TArg1Capture, TArg2Capture> CallbackObject;

    ctl::ComPtr<CallbackObject> spCallback;
    spCallback.Attach(new CallbackObject());
    if (spCallback)
    {
        spCallback->m_pfn     = pfn;
        spCallback->m_arg1Captured = arg1Captured;
        spCallback->m_arg2Captured = arg2Captured;
    }
    return spCallback;
}

//-----------------------------------------------------------------------------
//
// Callback object for a member function with no arguments.
//     
//-----------------------------------------------------------------------------
template <typename TTargetObject, typename TTargetObjectCapture>
struct MemberFunctionCallback0 : public ICallback
{
    HRESULT (TTargetObject::*m_pfn)();
    TTargetObjectCapture m_targetObjectCaptured;

    _Check_return_ HRESULT Invoke() override
    {
        typedef Coercion<TTargetObjectCapture, TTargetObject*> TargetObjectCoercion;
        TargetObjectCoercion targetObjectCoercion = TargetObjectCoercion(m_targetObjectCaptured);
        if (FAILED(targetObjectCoercion.hr))
        {
            return targetObjectCoercion.hr;
        }

        if (targetObjectCoercion.ShouldNoOp())
        {
            return S_FALSE;
        }

        HRESULT hr = (targetObjectCoercion.result->*m_pfn)();

        return hr;
    }
};

//-----------------------------------------------------------------------------
//
// Creates a callback for a member function with no arguments.
//     
//-----------------------------------------------------------------------------
template <typename TTargetObject, typename TTargetObjectCapture>
ctl::ComPtr<ICallback> MakeCallback(TTargetObjectCapture targetObjectCaptured, HRESULT (TTargetObject::*pfn)())
{
    typedef MemberFunctionCallback0<TTargetObject, TTargetObjectCapture> CallbackObject;

    ctl::ComPtr<CallbackObject> spCallback;
    spCallback.Attach(new CallbackObject());
    if (spCallback)
    {
        spCallback->m_pfn                  = pfn;
        spCallback->m_targetObjectCaptured = targetObjectCaptured;
    }
    return spCallback;
}

//-----------------------------------------------------------------------------
//
// Callback object for a member function with one argument.
//     
//-----------------------------------------------------------------------------
template <typename TTargetObject, typename TArg1, typename TTargetObjectCapture, typename TArg1Capture>
struct MemberFunctionCallback1 : public ICallback
{
    HRESULT (TTargetObject::*m_pfn)(TArg1);
    TTargetObjectCapture m_targetObjectCaptured;
    TArg1Capture         m_arg1Captured;

    _Check_return_ HRESULT Invoke() override
    {
        typedef Coercion<TTargetObjectCapture, TTargetObject*> TargetObjectCoercion;
        TargetObjectCoercion targetObjectCoercion = TargetObjectCoercion(m_targetObjectCaptured);
        if (FAILED(targetObjectCoercion.hr))
        {
            return targetObjectCoercion.hr;
        }

        typedef Coercion<TArg1Capture, TArg1> Arg1Coercion;
        Arg1Coercion arg1Coercion = Arg1Coercion(m_arg1Captured);
        if (FAILED(arg1Coercion.hr))
        {
            return arg1Coercion.hr;
        }

        if (targetObjectCoercion.ShouldNoOp())
        {
            return S_FALSE;
        }

        HRESULT hr = (targetObjectCoercion.result->*m_pfn)(arg1Coercion.result);

        return hr;
    }
};

//-----------------------------------------------------------------------------
//
// Creates a callback for a member function with one argument.
//     
//-----------------------------------------------------------------------------
template <typename TTargetObject, typename TArg1, typename TTargetObjectCapture, typename TArg1Capture>
ctl::ComPtr<ICallback> MakeCallback(TTargetObjectCapture targetObjectCaptured, HRESULT (TTargetObject::*pfn)(TArg1), TArg1Capture arg1Captured)
{
    typedef MemberFunctionCallback1<TTargetObject, TArg1, TTargetObjectCapture, TArg1Capture> CallbackObject;

    ctl::ComPtr<CallbackObject> spCallback;
    spCallback.Attach(new CallbackObject());
    if (spCallback)
    {
        spCallback->m_pfn                  = pfn;
        spCallback->m_targetObjectCaptured = targetObjectCaptured;
        spCallback->m_arg1Captured         = arg1Captured;
    }
    return spCallback;
}

//-----------------------------------------------------------------------------
//
// Callback object for a member function with two arguments.
//     
//-----------------------------------------------------------------------------
template <typename TTargetObject, typename TArg1, typename TArg2, typename TTargetObjectCapture, typename TArg1Capture, typename TArg2Capture>
struct MemberFunctionCallback2 : public ICallback
{
    HRESULT (TTargetObject::*m_pfn)(TArg1, TArg2);
    TTargetObjectCapture m_targetObjectCaptured;
    TArg1Capture         m_arg1Captured;
    TArg2Capture         m_arg2Captured;

    _Check_return_ HRESULT Invoke() override
    {
        typedef Coercion<TTargetObjectCapture, TTargetObject*> TargetObjectCoercion;
        TargetObjectCoercion targetObjectCoercion = TargetObjectCoercion(m_targetObjectCaptured);
        if (FAILED(targetObjectCoercion.hr))
        {
            return targetObjectCoercion.hr;
        }

        typedef Coercion<TArg1Capture, TArg1> Arg1Coercion;
        Arg1Coercion arg1Coercion = Arg1Coercion(m_arg1Captured);
        if (FAILED(arg1Coercion.hr))
        {
            return arg1Coercion.hr;
        }

        typedef Coercion<TArg2Capture, TArg2> Arg2Coercion;
        Arg2Coercion arg2Coercion = Arg2Coercion(m_arg2Captured);
        if (FAILED(arg2Coercion.hr))
        {
            return arg2Coercion.hr;
        }

        if (targetObjectCoercion.ShouldNoOp())
        {
            return S_FALSE;
        }

        HRESULT hr = (targetObjectCoercion.result->*m_pfn)(arg1Coercion.result, arg2Coercion.result);

        return hr;
    }
};

//-----------------------------------------------------------------------------
//
// Creates a callback for a member function with two arguments.
//     
//-----------------------------------------------------------------------------
template <typename TTargetObject, typename TArg1, typename TArg2, typename TTargetObjectCapture, typename TArg1Capture, typename TArg2Capture>
ctl::ComPtr<ICallback> MakeCallback(TTargetObjectCapture targetObjectCaptured, HRESULT (TTargetObject::*pfn)(TArg1, TArg2), TArg1Capture arg1Captured, TArg2Capture arg2Captured)
{
    typedef MemberFunctionCallback2<TTargetObject, TArg1, TArg2, TTargetObjectCapture, TArg1Capture, TArg2Capture> CallbackObject;

    ctl::ComPtr<CallbackObject> spCallback;
    spCallback.Attach(new CallbackObject());
    if (spCallback)
    {
        spCallback->m_pfn                  = pfn;
        spCallback->m_targetObjectCaptured = targetObjectCaptured;
        spCallback->m_arg1Captured         = arg1Captured;
        spCallback->m_arg2Captured         = arg2Captured;
    }
    return spCallback;
}


}
