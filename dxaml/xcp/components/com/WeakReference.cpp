// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "WeakReferenceImpl.h"

IFACEMETHODIMP
ctl::Details::WeakReferenceImpl::Resolve(_In_ REFIID iid, _Outptr_opt_ IInspectable **objectReference)
{
    ComBase *pSourceRef = NULL;

    *objectReference = NULL;
    pSourceRef = m_pSourceRef;

    // to perform a successful resolve, attempt to perform a thread safe ref count increment on the 
    // source before performing a QueryInterface. 
    for (;;)
    {
        ULONG cSourceRef = GetSourceRefCount();
        if (m_pSourceRef == NULL || cSourceRef == 0)
        {
            return S_OK;
        }

        if (InterlockedCompareExchange(&m_cSourceRef, cSourceRef + 1, cSourceRef) == cSourceRef)
        {
            break;
        }
    }

    if (m_pSourceRef)
    {
        // use the stack variable pointing to source reference to prevent a case
        // where we AV because m_pSourceRef has been cleared out by GC
        IFC_RETURN(ctl::as_iunknown(pSourceRef)->QueryInterface(iid, (void **)objectReference));
    }

    InterlockedDecrement(&m_cSourceRef);

    return S_OK;
}