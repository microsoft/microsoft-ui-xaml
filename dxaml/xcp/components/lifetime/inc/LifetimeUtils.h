// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "AutoPeg.h"
#include "LifetimeExterns.h"
#include "WeakReferenceSourceNoThreadId.h"
#include "ReferenceTrackerExtension.h"

namespace ctl
{
    // Resolve a weak reference
    template <class T>
    _Check_return_ HRESULT resolve_weakref(_In_ IWeakReference *pWeakRef, _Out_ T*& pStrongRef)
    {
        IInspectable* pResult = NULL;

        if (pWeakRef)
        {
            IFC_RETURN(pWeakRef->Resolve(__uuidof(T), &pResult));
            pStrongRef = static_cast<T*>(pResult);
        }

        return S_OK;
    }

    template <class T>
    _Check_return_ HRESULT resolve_weakref(_In_ IWeakReference *pWeakRef, _Out_ T** pStrongRef)
    {
        return resolve_weakref(pWeakRef, *pStrongRef);
    }

    _Check_return_ HRESULT as_weakref(_Out_ IWeakReference *&pWeakREf, _In_ IInspectable *pInput);

    enum ExpectedRefType
    {
        ExpectedRef_Tree,
        ExpectedRef_CLR,
        ExpectedRef_Exception
    };

    void  addref_expected(_In_opt_ xaml_hosting::IReferenceTrackerInternal* pInterface, _In_ ExpectedRefType refType);
    void  release_expected(_In_opt_ xaml_hosting::IReferenceTrackerInternal* pInterface);
    void  addref_expected(_In_opt_ IInspectable* pInterface, _In_ ExpectedRefType refType);
    void  release_expected(_In_opt_ IInspectable* pInterface);
    void  addref_expected(_In_opt_ WeakReferenceSourceNoThreadId* pObj, _In_ ExpectedRefType refType);
    void  release_expected(_In_opt_ WeakReferenceSourceNoThreadId* pObj);
    
    // Direct AddRef on a tracker object or its extension, even if it's aggregated.
    __inline void addref_interface_inner(_In_opt_ WeakReferenceSourceNoThreadId* pTrackerObj)
    {
        if (pTrackerObj)
        {
            pTrackerObj->AddRefDirect();
        }
    }

    // Direct Release on a tracker object or its extension, even if it's aggregated.
    __inline void release_interface_inner(_In_opt_ WeakReferenceSourceNoThreadId* pTrackerObj)
    {
        if (pTrackerObj)
        {
            pTrackerObj->ReleaseDirect();
        }
    }    
}

