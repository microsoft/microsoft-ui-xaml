// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <precomp.h>
#include <TDH.h>

ULONG g_uMockPropertySize = 0;
BYTE* g_pMockPropertyValue = nullptr;

////////////////////////////////////////////////////////////////////////////////
// MockTDHSetProperty is used by a test client to set the values that TdhGetProperty
// and TdhGetPropertySize return.
//
void MockTDHSetProperty(
    _In_ PWSTR /*pPropertyName*/,
    _In_ PBYTE pPropertyValue,
    _In_ ULONG uPropertyValueSize
    )
{
    g_uMockPropertySize = uPropertyValueSize;
    g_pMockPropertyValue = pPropertyValue;
}

////////////////////////////////////////////////////////////////////////////////
// TdhGetPropertySize is a mock implementation of the function with the same 
// name found in tdh.lib.
//
ULONG __stdcall TdhGetPropertySize(
    _In_   PEVENT_RECORD /*pEvent*/,
    _In_   ULONG TdhContextCount,
    _In_reads_opt_(TdhContextCount) PTDH_CONTEXT /*pTdhContext*/,
    _In_   ULONG PropertyDataCount,
    _In_reads_(PropertyDataCount) PPROPERTY_DATA_DESCRIPTOR /*pPropertyData*/,
    _Out_  ULONG *pPropertySize
    )
{
    *pPropertySize = g_uMockPropertySize;
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// TdhGetProperty is a mock implementation of the function with the same 
// name found in tdh.lib.
//
ULONG __stdcall TdhGetProperty(
    _In_   PEVENT_RECORD /*pEvent*/ ,
    _In_   ULONG TdhContextCount ,
    _In_reads_opt_(TdhContextCount) PTDH_CONTEXT /*pTdhContext*/ ,
    _In_   ULONG PropertyDataCount ,
    _In_reads_(PropertyDataCount) PPROPERTY_DATA_DESCRIPTOR /*pPropertyData*/ ,
    _In_   ULONG BufferSize,
    _Out_writes_bytes_(BufferSize) PBYTE pBuffer
    )
{
    if (g_uMockPropertySize > BufferSize)
    {
        return ERROR_INSUFFICIENT_BUFFER;
    }

    CopyMemory(pBuffer, g_pMockPropertyValue, g_uMockPropertySize);
    return 0;
}