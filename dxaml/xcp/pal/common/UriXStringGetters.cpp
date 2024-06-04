// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Implements helper getters for IPALUri fields that return xstring_ptrs instead of raw buffers.

#include "precomp.h"
#include "XStringBuilder.h"

namespace UriXStringGetters_Internal
{
    template<class MemberGetterFunctionType>
    _Check_return_ HRESULT GetUriPart(
        _In_ const IPALUri *pUri,
        MemberGetterFunctionType pMemberGetter,
        _Out_ xstring_ptr* pstrValue
        )
    {
        HRESULT hr = S_OK;
        WCHAR *pszValue = NULL;
        XUINT32 cValue = 0;
        XStringBuilder valueBuilder;

        IFC((pUri->*pMemberGetter)(&cValue, NULL));

        ++cValue; // need one more character than reported for the null terminator

        IFC(valueBuilder.InitializeAndGetFixedBuffer(cValue, &pszValue));

        IFC((pUri->*pMemberGetter)(&cValue, pszValue));

        valueBuilder.SetFixedBufferCount(cValue);
        IFC(valueBuilder.DetachString(pstrValue));

    Cleanup:
        RRETURN(hr);
    }
}

namespace UriXStringGetters
{
    _Check_return_ HRESULT GetScheme(
        _In_ const IPALUri *pUri,
        _Out_ xstring_ptr* pstrScheme
        )
    {
        RRETURN(UriXStringGetters_Internal::GetUriPart(pUri, &IPALUri::GetScheme, pstrScheme));
    }

    _Check_return_ HRESULT GetHost(
        _In_ const IPALUri *pUri,
        _Out_ xstring_ptr* pstrHost
        )
    {
        RRETURN(UriXStringGetters_Internal::GetUriPart(pUri, &IPALUri::GetHost, pstrHost));
    }
    
    _Check_return_ HRESULT GetPath(
        _In_ const IPALUri *pUri,
        _Out_ xstring_ptr* pstrPath
        )
    {
        RRETURN(UriXStringGetters_Internal::GetUriPart(pUri, &IPALUri::GetPath, pstrPath));
    }

    _Check_return_ HRESULT GetFilePath(
        _In_ const IPALUri *pUri,
        _Out_ xstring_ptr* pstrPath
        )
    {
        RRETURN(UriXStringGetters_Internal::GetUriPart(pUri, &IPALUri::GetFilePath, pstrPath));
    }
}
