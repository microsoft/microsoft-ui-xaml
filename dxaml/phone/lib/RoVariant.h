// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "precomp.h"

#ifndef FAILED
#error Depends on the <windows.h> FAILED macro and HRESULT type. \
Please include <windows.h> or define FAILED before importing \
this header.
#endif

//
// RoVariant
//
// Synopsis: drop in replacement for IPV, ComPtr<IPV>
//   provides accessor methods for reading an IPV instance.
//
// Using as out param:
//    HRESULT f1(_Out_ IInspectable** ppI);
//    HRESULT f2(_Out_ IPropertyValue** ppPV);
//    RoVariant rv;
//    hr = f1(&rv);
//    hr = f2(&rv);
// Get an integer:
//    int x;
//    hr = rv->GetInt32(&x);
// Get a normal object out (non-boxed -- will fail if
//  rv holds refernece to an IPV):
//    ComPtr<IInspectable> ifoo;
//    hr = rv->GetInspectable(&ifoo);
//
//

// PURPOSE
//  STL functions that are needed for proper rvalue ref handling, and
//  for alignment calculation
//
// NOTE
//  This code is essentially copied unchanged from the STL 10 code.
namespace XWinRT { namespace FakeStl2 {

        template<class T>
        struct remove_reference
        {    // remove reference
            typedef T type;
        };

        template<class T>
        struct remove_reference<T&>
        {    // remove reference
            typedef T type;
        };

        template<class T>
        struct remove_reference<T&&>
        {    // remove rvalue reference
            typedef T type;
        };

        // TEMPLATE FUNCTION move
        template<class T> inline
        typename XWinRT::FakeStl2::remove_reference<T>::type&&
        move(T&& arg)
        {    // forward _Arg as movable
            return ((typename XWinRT::FakeStl2::remove_reference<T>::type&&)arg);
        }

        // TEMPLATE FUNCTION swap (from <algorithm>)
        template<class T> inline
        void swap(T& lhs, T& rhs)
        {    // exchange values stored at lhs and rhs
            using XWinRT::FakeStl2::move;
            T tmp = move(lhs);
            lhs = move(rhs);
            rhs = move(tmp);
        }
}} // XWinRT::FakeStl2

// RoVariant
// Smart pointer as drop-in replacement for ComPtr<IInspectable> and
// ComPtr<IPropertyValue>. Provides IPV methods over IInspectable*,
// to access boxed instances.
//
// Where the IInspectable* is an in parameter, use 'RoVariant::Wrap()'.
//   This will forgo unnecessary AddRefs in most cases.
// Where ComPtr<IPropertyValue> was used, replace with 'RoVariant'.
// Where IPropertyValue* was a member field (with manual ref counting),
//   typically replace with 'RoVariant'
//
// Note: no null pointer checks are needed. IInspectable* of null
//   is the new representation of "empty".
//
// Eg, this is legal:
//
//     RoVariant spValue = __nullptr;
//     spValue->get_Type(&type); // 'type' will be set to PropertyType_Empty.
//
// Other examples:
//
//     HRESULT foo(IInspectable* pValue_)
//     {
//         auto pValue = RoVariant::Wrap(pValue);
//
//         // was using pValue_ directly
//         if       (SUCCEEDED(pValue->GetInt32(&i))) { ... }
//         elese if (SUCCEEDED(pValue->GetString(&hs))) { ... }
//     }
//
// And
//
//     RoVariant spValue;  // was ComPtr<IPropertyValue>
//     HRESULT set_Something(IInspectable* pValue)
//     {
//         spValue = pValue; return S_OK;
//     }
//     HRESULT DoStuff()
//     {
//         PropertyType type;
//         hr = spValue->get_Type(&type);
//         switch(type) { ... }
//     }
//
//
class RoVariant
{
private:
    // Note: these must be success HRESULTs
    // bits: 0: pointer set
    //       1: refcounted
    //       2: is property value
    enum States
    {
        StateIsNull     = S_OK,
        StateIsObjNoRef = S_OK + 1,
        StateIsObj      = S_OK + 3,
        StateIsPV       = S_OK + 7,
    };
    static bool StateHasRefcount(HRESULT hrState)
    {
        return ((hrState == StateIsObj) || (hrState == StateIsPV));
    }

    // Accessor class. Provides member functions on operator->
    class Accessor
    {
    private:
        IInspectable* _pI;
        HRESULT _hrState;

        ABI::Windows::Foundation::IPropertyValue* _PV() const
        {
            ASSERT(_hrState == StateIsPV);
            return static_cast<ABI::Windows::Foundation::IPropertyValue*>(_pI);
        }
        // Caller is assuming _hrState == StateIsPV. returns.
        // _hrState or TYPE_E_TYPEMISMATCH as appropriate
        HRESULT VerifyPV() const
        {
            if (_hrState == StateIsPV)
            {
                return S_OK;
            }
            else
            {
                return FAILED(_hrState) ? _hrState : TYPE_E_TYPEMISMATCH;
            }
        }

    public:
        Accessor(IInspectable* pI,
                          HRESULT hrState)
        : _pI(pI)
        , _hrState(hrState)
        {
        }
        const Accessor* operator->() const { return this; }

        //
        // various accessor methods (all const)
        //
        HRESULT get_Type ( _Out_ enum ABI::Windows::Foundation::PropertyType* type ) const
        {
            IFC_RETURN( _hrState );
            switch (_hrState)
            {
            case StateIsNull:
                *type = ABI::Windows::Foundation::PropertyType_Empty;
                break;
            case StateIsObj:
            case StateIsObjNoRef:
                *type = ABI::Windows::Foundation::PropertyType_Inspectable;
                break;
            case StateIsPV:
                IFC_RETURN( _PV()->get_Type(type) );
                break;
            default:
                ASSERT( false );
                #ifndef DBG
                __assume(false);
                #endif
                break;
            }
            return S_OK;
        }

        // To add more types, add a GetFoo definition here and a DEF_SCALAR_ACCESSOR below.
        HRESULT GetBoolean ( _Out_ boolean* value ) const;
    };

    // Notes: returned by RoVariant::operator&, used for COM
    //   out-params. Any value written to the location returned
    //   by operator IPV**() or operator II**() will be written
    //   back into the original RoVariant instance.
    class OutRef {
    private:
        RoVariant*      _pOwner;
        IInspectable*   _pI;
    public:
        explicit OutRef  (RoVariant* pOwner)
        : _pOwner(pOwner), _pI(__nullptr)
        {
        }
        operator ABI::Windows::Foundation::IPropertyValue**()
        {
            return reinterpret_cast<ABI::Windows::Foundation::IPropertyValue**>(&_pI);
        }
        operator IInspectable**()
        {
            return &_pI;
        }
        ~OutRef ()
        {
            // assign back to *pOnwer
            _pOwner->Attach(_pI);
        }
    };

private:
    IInspectable* _pI;
    HRESULT _hrState; // StateIsNull --> _pI is null
                      // StateIsObj  --> _pI is valid, not an IPV instance
                      // StateIsIPV  --> _pI is an IPV instance
                      // other --> return failure on accessor calls. eg, remoting error.

public:
    RoVariant(decltype(__nullptr)=__nullptr)
    {
        _pI = __nullptr;
        _hrState = StateIsNull;
    }
    // when 'attach' is true, pI has already been AddRef'd;
    //   e.g. when receiving an out param.
    RoVariant(IInspectable* pI, bool attach = false)
    : _pI(nullptr), _hrState(StateIsNull)
    {
        RoVariant tmp(pI, true, attach);
        this->Swap(tmp);
    }

private:
    RoVariant(IInspectable* pI, bool fAddRefInspectable, bool attach)
    {
        if (!pI)
        {
            _pI = __nullptr;
            _hrState = StateIsNull;
        }
        else
        {
            ABI::Windows::Foundation::IPropertyValue* pPV;
            HRESULT hr = pI->QueryInterface(&pPV);
            if (SUCCEEDED(hr))
            {
                _pI = pPV;
                if (attach)
                {
                    pI->Release();
                }
                _hrState = StateIsPV;
            }
            else if (hr == E_NOINTERFACE)
            {
                _pI = pI;
                if (fAddRefInspectable && !attach)
                {
                    _pI->AddRef();
                }
                _hrState = static_cast<HRESULT>(fAddRefInspectable ? StateIsObj
                                                                   : StateIsObjNoRef);
            }
            else
            {
                _pI = __nullptr;
                _hrState = hr;
                if (attach)
                {
                    pI->Release();
                }
            }
        }
    }
public:
    RoVariant(ABI::Windows::Foundation::IPropertyValue* pPV, bool attach = false)
    {
        if (!pPV)
        {
            _pI = __nullptr;
            _hrState = StateIsNull;
        }
        else
        {
            _pI = pPV;
            if (!attach)
            {
                _pI->AddRef();
            }
            _hrState = StateIsPV;
        }
    }
    RoVariant(const RoVariant& other)
    {
        _pI = other._pI;
        _hrState = other._hrState;
        if (_pI && StateHasRefcount(_hrState))
        {
            _pI->AddRef();
        }
    }
    RoVariant(RoVariant&& other) noexcept
    {
        _pI = other._pI;
        _hrState = other._hrState;
        other._pI = __nullptr;
        other._hrState = StateIsNull;
    }
    template <class T>
    RoVariant(const Microsoft::WRL::ComPtr<T>& sp)
    : _pI(nullptr), _hrState(StateIsNull)
    {
        RoVariant tmp(sp.Get());
        this->Swap(tmp);
    }
    ~RoVariant()
    {
        if (_pI && StateHasRefcount(_hrState))
        {
            _pI->Release();
        }
    }

    // double as move and copy assignment.
    RoVariant& operator=(RoVariant other)
    {
        this->Swap(other);
        return *this;
    }
    void Swap(RoVariant& other)
    {
        using XWinRT::FakeStl2::swap;
        swap(_pI, other._pI);
        swap(_hrState, other._hrState);
    }

    // For drop-in compatibility with IInspectable*
    // Note: only useful as basic pointer comparison.
    // Does NOT ensure COM Identity comparison
    operator IInspectable*() const
    {
        return Get();
    }

    // only for use by RoVariant
    IInspectable* Get() const
    {
        return _pI;
    }

    // sets instance to null, returning former value. Does not Release pointer.
    // Pointer may not be addref'd if _hrState == StateIsObj and
    // fAddRefInspectable == false. Only for use with RoVariant.
    IInspectable* Detach()
    {
        IInspectable* tmp = _pI;
        _pI = __nullptr;
        _hrState = StateIsNull;
        return tmp;
    }

    void Attach(IInspectable* pI)
    {
        RoVariant tmp(pI, true);
        this->Swap(tmp);
    }
    void Attach(ABI::Windows::Foundation::IPropertyValue* pPV)
    {
        RoVariant tmp(pPV, true);
        this->Swap(tmp);
    }

    Accessor operator*() const
    {
        return Accessor(_pI, _hrState);
    }
    Accessor operator->() const
    {
        return Accessor(_pI, _hrState);
    }

    // for COM out-params
    OutRef operator&()
    {
        return OutRef(this);
    }

    // not used.
    // present to simplify diagnosing ComPtr related misuse.
    struct USE_INSTEAD_ReleaseAndGetAddressOf{};
    USE_INSTEAD_ReleaseAndGetAddressOf GetAddressOf();

    // for COM out-params, when you explicitly need to release _before_ invoking
    OutRef ReleaseAndGetAddressOf()
    {
        *this = nullptr;
        return OutRef(this);
    }

    bool operator!() const
    {
        return !(Get());
    }

    // Use this free function if you only need to make a
    // one-off call, or wrap an input parameter that you
    // don't otherwise wish to addref.  E.g.:
    //
    //     hr = RoVariant::Wrap(pI)->GetInt32(&i);
    //
    // OR
    //
    //     auto spValue = RoVariant::Wrap(pValue_);
    //     hr = spValue->GetInt32(&i);
    //     if (FAILED(hr)) {
    //         spValue->GetString(&hs);
    //     }... etc.
    //
    // Avoids addref of IInspectable for non-boxed instances.
    static RoVariant Wrap(IInspectable* pI)
    {
        return RoVariant(pI, false, false);
    }
    static RoVariant Wrap(ABI::Windows::Foundation::IPropertyValue* pPV)
    {
        return RoVariant(pPV);
    }

    HRESULT CopyTo(IInspectable** ppI) const
    {
        IFC_RETURN(_hrState);
        (*ppI = _pI);
        if (_pI)
        {
            _pI->AddRef();
        }
        return S_OK;
    }
    HRESULT CopyTo(ABI::Windows::Foundation::IPropertyValue** ppPV) const
    {
        IFC_RETURN(_hrState);
        if (!_pI)
        {
            *ppPV = nullptr;
            return S_OK;
        }
        else if (_hrState == StateIsPV)
        {
            (*ppPV = static_cast<ABI::Windows::Foundation::IPropertyValue*>(_pI));
            _pI->AddRef();
            return S_OK;
        }
        else
        {
            return E_NOINTERFACE;
        }
    }
};

// operator == needed to resolve ambiguity introduced by conversion to IInspectable*
inline bool operator==(const RoVariant& lhs, const RoVariant& rhs)
{
    return (lhs.Get() == rhs.Get());
}
inline bool operator!=(const RoVariant& lhs, const RoVariant& rhs)
{
    return !(lhs == rhs);
}

inline bool operator==(const RoVariant& lhs, const std::nullptr_t&)
{
    return (lhs.Get() == nullptr);
}
inline bool operator!=(const RoVariant& lhs, const std::nullptr_t&)
{
    return !(lhs == nullptr);
}
inline bool operator==(const std::nullptr_t&, const RoVariant& rhs)
{
    return (nullptr == rhs.Get());
}
inline bool operator!=(const std::nullptr_t&, const RoVariant& rhs)
{
    return !(nullptr == rhs);
}

namespace Windows { namespace Foundation { namespace Detail
{
    template <class T>
    void ZeroInit(T& value)
    {
        value = 0;
    }
}}}

#define DEF_SCALAR_ACCESSOR( winrttype, cpptype ) \
inline HRESULT RoVariant::Accessor::Get ## winrttype ( _Out_ cpptype* value ) const \
{\
    if (!value)\
    {\
        return E_POINTER; \
    }\
    ::Windows::Foundation::Detail::ZeroInit(*value); \
    IFC_RETURN( VerifyPV() ); \
    return _PV()->Get ## winrttype(value);\
}

// To add more types, add a DEF_SCALAR_ACCESSOR(foo, bar) here and a GetFoo definition above.
DEF_SCALAR_ACCESSOR(Boolean, boolean)

#undef DEF_SCALAR_ACCESSOR

