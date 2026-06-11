// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <xref_ptr.h>
#include <CValue.h>
#include <type_traits>
#include <CDependencyObject.h>
#include "DependencyObjectTraits.h"

//---------------------------------------------------------------------------------------------------------------------------------
//  Verify a CDependencyObject's type before casting.
//
//  The do_pointer_cast<> form is meant to look like the standard dynamic_cast<> and returns NULL if the types are not compatible.
//
//  The DoPointerCast<> form follows the Silverlight standard of returning a failure code if the types do not match.
//      example:
//          _In_ CDependencyObject *pObject,
//          ...
//          CBorder* pBorder;
//          IFC(DoPointerCast(pBorder, pObject));
//
//  Overloads are provided for casting from a CValue, which additional ensures that the source is a 'valueObject'.
//---------------------------------------------------------------------------------------------------------------------------------

template<class T> inline
constexpr T* do_pointer_cast(_In_ T* pSameType)
{
    return pSameType;
}

template<class T>
constexpr T* do_pointer_cast(_In_opt_ CDependencyObject* pDO)
{
    static_assert(!std::is_same<T, CDependencyObject>::value, "No need to use cast.");
    return pDO && pDO->OfTypeByIndex<DependencyObjectTraits<T>::Index>() ? static_cast<T*>(pDO) : nullptr;
}

template<class T>
constexpr const T* do_pointer_cast(_In_opt_ const CDependencyObject* pDO)
{
    return do_pointer_cast<T>(const_cast<CDependencyObject*>(pDO));
}

template<class T>
constexpr T* do_pointer_cast(_In_ CValue& value)
{
    return do_pointer_cast<T>(value.As<valueObject>());
}

template<class T>
constexpr const T* do_pointer_cast(const CValue& value)
{
    return do_pointer_cast<T>(const_cast<CValue&>(value));
}

template<class FromType, class ToType>
constexpr xref_ptr<ToType> do_pointer_cast(const xref_ptr<FromType>& arg)
{
    return xref_ptr<ToType>(do_pointer_cast<ToType>(arg.get()));
}

template<class T>
_Check_return_ HRESULT DoPointerCast(_Out_ T*& pOut, _In_opt_ CDependencyObject* pIn)
{
    if (!pIn || do_pointer_cast<T>(pIn))
    {
        pOut = static_cast<T*>(pIn);
        return S_OK;
    }

    pOut = nullptr;
    return CORE_E_INVALIDTYPE;
}

template<class T>
_Check_return_ HRESULT DoPointerCast(_Out_ const T*& pOut, _In_opt_ CDependencyObject* pIn)
{
    return DoPointerCast(const_cast<T*&>(pOut), pIn);
}

template<class T>
_Check_return_ HRESULT DoPointerCast(_Out_ const T*& pOut, _In_opt_ const CDependencyObject* pIn)
{
    return DoPointerCast(const_cast<T*&>(pOut), const_cast<CDependencyObject*>(pIn));
}

template<class T>
_Check_return_ HRESULT DoPointerCast(_Out_ T*& pOut, _In_ CValue& value)
{
    return DoPointerCast(pOut, value.As<valueObject>());
}

template<class T>
_Check_return_ HRESULT DoPointerCast(_Out_ const T*& pOut, _In_ CValue& value)
{
    return DoPointerCast(const_cast<T*&>(pOut), value);
}

template<class T>
_Check_return_ HRESULT DoPointerCast(_Out_ const T*& pOut, const CValue& value)
{
    return DoPointerCast(const_cast<T*&>(pOut), const_cast<CValue&>(value));
}

template<class T>
_Check_return_ HRESULT DoPointerCast(_Out_ xref_ptr<T>& pOut, _In_opt_ CDependencyObject* pIn)
{
    T* pTemp = nullptr;
    HRESULT hr = DoPointerCast(pTemp, pIn);
    pOut = pTemp;
    return hr;
}

template<class T>
constexpr T* checked_cast(_In_ T* pSameType)
{
    ASSERT(!pSameType || pSameType->template OfTypeByIndex<DependencyObjectTraits<T>::Index>());
    return pSameType;
}

template<class T>
constexpr const T* checked_cast(_In_ const T* pSameType)
{
    ASSERT(!pSameType || pSameType->template OfTypeByIndex<DependencyObjectTraits<T>::Index>());
    return pSameType;
}

template<class T>
constexpr T* checked_cast(_In_ CDependencyObject* pDO)
{
    static_assert(!std::is_same<T, CDependencyObject>::value, "No need to use cast.");
    ASSERT(!pDO || pDO->OfTypeByIndex<DependencyObjectTraits<T>::Index>());
    return static_cast<T*>(pDO);
}

template<class T>
constexpr const T* checked_cast(const CDependencyObject* pDO)
{
    static_assert(!std::is_same<T, CDependencyObject>::value, "No need to use cast.");
    ASSERT(!pDO || pDO->OfTypeByIndex<DependencyObjectTraits<T>::Index>());
    return static_cast<const T*>(pDO);
}

template<class T>
constexpr T* checked_cast(_In_ CValue& arg)
{
    return checked_cast<T>(arg.As<valueObject>());
}

template<class T>
constexpr const T* checked_cast(const CValue& arg)
{
    return checked_cast<T>(arg.As<valueObject>());
}

template<class ToType, class FromType>
xref_ptr<ToType> checked_sp_cast(const xref_ptr<FromType>& arg)
{
    return xref_ptr<ToType>(checked_cast<ToType>(arg.get()));
}