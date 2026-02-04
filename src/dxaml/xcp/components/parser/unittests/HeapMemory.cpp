// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "HeapMemory.h"

/////////////////////////////////////////////////////////////////////
///  Implementation for class CHeapMemory
////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------
//
//  Method:   Create
//
//  Synopsis:
//
//------------------------------------------------------------------------

_Check_return_ HRESULT CHeapMemory::Create(XUINT32 _In_ cSize, _Outptr_ IPALMemory **ppPalMemory)
{
    HRESULT hr = S_OK;
    CHeapMemory *pMemory = NULL;
        
    IFCPTR(ppPalMemory);
        
    pMemory = new CHeapMemory();

    if (cSize <= 0)
    {
        IFC(E_FAIL);
    }
    
    pMemory->m_pData = new XUINT8[cSize];
    pMemory->m_cSize = cSize;
    *ppPalMemory = pMemory;
    pMemory = NULL;
    
Cleanup:
    
    ReleaseInterface(pMemory);
        
    RRETURN(hr);        

}

CHeapMemory::~CHeapMemory()
{
    if ( m_pData && m_bOwnMemory)
    {
        delete [] m_pData;
    }
    m_pData = NULL;
    m_cSize = 0;
}

//------------------------------------------------------------------------
//
//  Method:   Create
//
//  Synopsis:
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CHeapMemory::Create(XUINT32 _In_ cSize, _In_ XUINT8 *pMemory, _Outptr_ IPALMemory **ppPalMemory, XUINT8 bOwnMemory)
{
    HRESULT hr = S_OK;
    CHeapMemory *pHeapMemory = NULL;

    IFCPTR(ppPalMemory);
    IFCPTR(pMemory);
    pHeapMemory = new CHeapMemory();
    
  
    pHeapMemory->m_pData = pMemory;
    pHeapMemory->m_cSize = cSize;
    pHeapMemory->m_bOwnMemory = bOwnMemory;
    
    *ppPalMemory = pHeapMemory;
    pHeapMemory = NULL;
    
Cleanup:
    
    ReleaseInterface(pHeapMemory);
        
    RRETURN(hr);        

}

//------------------------------------------------------------------------
//
//  Method:   AddRef
//
//  Synopsis:
//
//------------------------------------------------------------------------
XUINT32 CHeapMemory::AddRef() const
{
    XUINT32 cRef = (XUINT32)::InterlockedIncrement((LONG*) &m_cRef);
    
    ASSERT(cRef);

    if (!cRef)
    {
        // we have hit an overflow...exit the process
        XAML_FAIL_FAST();
    }
    return cRef;
}

//------------------------------------------------------------------------
//
//  Method:   Release
//
//  Synopsis:
//
//------------------------------------------------------------------------
XUINT32     CHeapMemory::Release() const
{
    XUINT32 cRef = (XUINT32)::InterlockedDecrement((LONG*)&m_cRef);

    if (0 == cRef)
    {
        delete this;
    }

    return cRef;
}

//------------------------------------------------------------------------
//
//  Method:   GetAddress
//
//  Synopsis:
//
//------------------------------------------------------------------------
void*  CHeapMemory::GetAddress() const
{
    return m_pData;
}

//------------------------------------------------------------------------
//
//  Method:   GetSize
//
//  Synopsis:
//
//------------------------------------------------------------------------
XUINT32     CHeapMemory::GetSize() const
{
    return m_cSize;
}

