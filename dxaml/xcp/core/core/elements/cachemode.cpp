// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#define STR_BITMAP_CACHE L"BitmapCache"

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates an instance of the base cachemode class.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CCacheMode::Create(
    _Outptr_ CDependencyObject **ppObject,
    _In_ CREATEPARAMETERS *pCreate
    )
{
    // If we're passed a string create forward the call to the appropriate
    // subclass create.

    if (pCreate->m_value.GetType() == valueString)
    {
        // Check for "BitmapCache"
        XUINT32 cString = 0;
        const WCHAR* pString = pCreate->m_value.AsEncodedString().GetBufferAndCount(&cString);
        if (SZ_COUNT(STR_BITMAP_CACHE) == cString
            && (0 == xstrncmpi(STR_BITMAP_CACHE, pString, cString)))
        {
            IFC_RETURN(CBitmapCache::Create(ppObject, pCreate));
            return S_OK;
        }
        
    }

    // If we can't make an object, return an error
    return E_FAIL;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates an instance of the BitmapCache class.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CBitmapCache::Create(
    _Outptr_ CDependencyObject **ppObject,
    _In_ CREATEPARAMETERS *pCreate
)
{
    HRESULT hr = E_FAIL;
    CBitmapCache *pObject = NULL;

    pObject = new CBitmapCache(pCreate->m_pCore);
    IFC(ValidateAndInit(pObject, ppObject));

// On success we've transferred ownership

    pObject = NULL;

Cleanup:
    delete pObject;
    RRETURN(hr);
}




