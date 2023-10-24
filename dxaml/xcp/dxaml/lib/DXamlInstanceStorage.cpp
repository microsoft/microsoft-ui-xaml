// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "DXamlInstanceStorage.h"

extern DWORD g_dwTlsIndex;

namespace DXamlInstanceStorage
{
    const DWORD TLS_UNINITIALIZED = -1;

    DWORD g_dwTlsIndex = TLS_UNINITIALIZED;

    _Check_return_ HRESULT Initialize()
    {
        HRESULT hr = S_OK;

        if (TLS_UNINITIALIZED != g_dwTlsIndex)
        {
            goto Cleanup;
        }

        g_dwTlsIndex = TlsAlloc();
        
        if (TLS_OUT_OF_INDEXES == g_dwTlsIndex)
        {
            g_dwTlsIndex = TLS_UNINITIALIZED;
            IFC(E_FAIL);
        }

    Cleanup:
        RRETURN(hr);
    }

    _Check_return_ HRESULT Deinitialize()
    {
        HRESULT hr = S_OK;

        if (TLS_UNINITIALIZED == g_dwTlsIndex)
        {
            goto Cleanup;
        }

        if (0 == TlsFree(g_dwTlsIndex))
        {
            IFC(E_FAIL);
        }
        g_dwTlsIndex = TLS_UNINITIALIZED;
    
    Cleanup:
        RRETURN(hr);
    }

    _Check_return_ HRESULT GetValue(_Outptr_result_maybenull_ Handle* phValue)
    {
        HRESULT hr = S_OK;

        IFCEXPECT(TLS_UNINITIALIZED != g_dwTlsIndex);

        *phValue = TlsGetValue(g_dwTlsIndex);
        if (NULL == *phValue && ERROR_SUCCESS != GetLastError())
        {
            IFC(E_FAIL);
        }
    
    Cleanup:
        RRETURN(hr);
    }

    _Check_return_ HRESULT SetValue(_In_opt_ Handle hValue)
    {
        HRESULT hr = S_OK;

        IFCEXPECT(TLS_UNINITIALIZED != g_dwTlsIndex);

        if (0 == TlsSetValue(g_dwTlsIndex, hValue))
        {
            IFC(E_FAIL);
        }
    
    Cleanup:
        RRETURN(hr);
    }
};

