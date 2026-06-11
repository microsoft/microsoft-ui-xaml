// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// Contains the basic memory support for the core platform
// abstraction layer

#pragma once

#include <minpal.h>

struct IPALStream;

//------------------------------------------------------------------------
//
//  Interface:  IPALMemoryServices
//
//  Synopsis:
//      Provides an abstraction for system memory handling support.
//
//------------------------------------------------------------------------

struct IPALMemoryServices
{
// Memory services

    virtual _Check_return_ HRESULT CreatePALMemoryFromBuffer(_In_ XUINT32 cbBuffer, _In_reads_(cbBuffer) XUINT8* pBuffer, _In_ bool fOwnsBuffer, _Out_ IPALMemory** ppPALMemory) = 0;
    virtual _Check_return_ HRESULT CreateIPALStreamFromIPALMemory(_In_ IPALMemory *pPALMemory, _Out_ IPALStream **ppPALStream) = 0;
};

