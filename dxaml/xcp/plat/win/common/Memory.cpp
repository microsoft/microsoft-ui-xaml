// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

//------------------------------------------------------------------------
//
//  Method:   ctor
//
//  Synopsis:
//      Constructor for CMappedMemory object
//
//------------------------------------------------------------------------

CMappedMemory::CMappedMemory(
    _In_ IPALFile *pOwner,
    _In_ HANDLE hMapped,
    _In_ void *pMapped,
    _In_ XUINT32 cMapped,
    _In_ XUINT32 cOffset
)
{
    m_pOwner = pOwner;
    m_hMapped = hMapped;
    m_pMapped = pMapped;
    m_cMapped = cMapped;
    m_cOffset = cOffset;
}

//------------------------------------------------------------------------
//
//  Method:   dtor
//
//  Synopsis:
//      Destructor for CMappedMemory object
//
//------------------------------------------------------------------------

CMappedMemory::~CMappedMemory()
{
    UnmapViewOfFile(m_pMapped);
    CloseHandle(m_hMapped);
    ReleaseInterface(m_pOwner);
}

//------------------------------------------------------------------------
//
//  Method:   ctor
//
//  Synopsis:
//      Constructor for CBufferMemory object
//
//------------------------------------------------------------------------

CBufferMemory::CBufferMemory(
    _In_reads_(cBuffer) XUINT8 *pBuffer,
    _In_ XUINT32 cBuffer,
    _In_ bool fOwnsBuffer
)
{
    m_pBuffer = pBuffer;
    m_cBuffer = cBuffer;
    m_fOwnsBuffer = fOwnsBuffer;
}

//------------------------------------------------------------------------
//
//  Method:   dtor
//
//  Synopsis:
//      Destructor for CBufferMemory object
//
//------------------------------------------------------------------------

CBufferMemory::~CBufferMemory()
{
    if (m_fOwnsBuffer && m_pBuffer)
        delete [] m_pBuffer;
}

