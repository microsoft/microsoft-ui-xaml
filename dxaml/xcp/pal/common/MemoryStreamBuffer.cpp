// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

//------------------------------------------------------------------------
//
// CMemoryStreamBuffer::~CMemoryStreamBuffer
//
// dtor
//
//------------------------------------------------------------------------
CMemoryStreamBuffer::~CMemoryStreamBuffer()
{
    CMemoryStreamBufferBlock* pTemp = m_pBlockFirst;
    while (pTemp)
    {
        pTemp= pTemp->GetNext();
        delete m_pBlockFirst;
        m_pBlockFirst = pTemp;
    }
}

//------------------------------------------------------------------------
//
// CMemoryStreamBuffer::AddRef
//
//
//
//------------------------------------------------------------------------
XUINT32 CMemoryStreamBuffer::AddRef()
{
    return PAL_InterlockedIncrement(&m_cRef);
}

//------------------------------------------------------------------------
//
// CMemoryStreamBuffer::Release
//
//
//
//------------------------------------------------------------------------
XUINT32 CMemoryStreamBuffer::Release()
{
    XUINT32 cRef = PAL_InterlockedDecrement(&m_cRef);

    if (cRef == 0)
    {
        delete this;
    }

    return cRef;
}

//------------------------------------------------------------------------
//
// CMemoryStreamBuffer::Write
//
// Add new data to the end of the data stream
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CMemoryStreamBuffer::Write(_In_reads_bytes_(cb) void const *pData, XUINT32 cb, XINT32 cbOffset)
{
    std::unique_ptr<CMemoryStreamBufferBlock> pBlock;

    // cbOffset is ignored since this is actually an Append with http11 not
    // being used.

    IFCEXPECT_RETURN(pData);

    auto lock = m_Lock.lock();

    pBlock = std::make_unique<CMemoryStreamBufferBlock>();

    IFC_RETURN(pBlock->Write(cb, pData));

    if (m_pBlockLast)
    {
        m_pBlockLast->SetNext(pBlock.get());
    }
    else
    {
        ASSERT(!m_pBlockFirst);
        m_pBlockFirst = pBlock.get();
    }
    m_pBlockLast = pBlock.get();
    pBlock.release(); // memory is now owned by the list
    if (m_bUpdateSize)
    {
        if (m_uSize == (XUINT32)-1)
        {
            m_uSize = cb;
        }
        else
        {
            m_uSize += cb;
        }
    };
    return S_OK;
}

//------------------------------------------------------------------------
//
// CMemoryStreamBuffer::Read
//
// Read a block of data from the data stream
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CMemoryStreamBuffer::Read (_Out_writes_bytes_(cb) void* pv, XUINT32 cb,  _Out_opt_ XUINT32* pcbRead)
{
    XUINT32 cbRead = 0;
    XUINT32 cbReadTotal = 0;

    if (cb == 0)
    {
        return S_OK;
    }

    auto lock = m_Lock.lock();
    auto readUpdater = wil::scope_exit([&]
    {
        if (pcbRead)
        {
            *pcbRead = cbReadTotal;
        }
    });

    if (m_pBlockFirst == NULL)
    {
        IFC_RETURN(E_FAIL);
    }

    while ((cbReadTotal < cb) && m_pBlockFirst)
    {
        IFC_RETURN(m_pBlockFirst->Read((XUINT8*)pv + cbReadTotal, m_uCurrentBlockOffset, cb - cbReadTotal, &cbRead));
        cbReadTotal += cbRead;
        if (cbReadTotal < cb)
        {
            //release this block
            CMemoryStreamBufferBlock* pTemp = m_pBlockFirst;
            m_pBlockFirst = m_pBlockFirst->GetNext();
            delete pTemp;
            m_uCurrentBlockOffset = 0;
        }
        else
        {
            m_uCurrentBlockOffset += cbRead;
        }
    }

    //if we have released all the blocks set last block to NULL;
    if (!m_pBlockFirst)
    {
        m_pBlockLast = NULL;
    }
    ASSERT(cbReadTotal <= m_uSize);

    return S_OK;
}


//------------------------------------------------------------------------
//
// CMemoryStreamBuffer::CreateStream
//
// Creates CMemoryStream
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CMemoryStreamBuffer::CreateStream(_Out_ IPALStream** ppStream)
{
        HRESULT hr = S_OK;
        CMemoryStream* pStream = NULL;
        
        IFCPTR(ppStream);
    
        IFC(CMemoryStream::Create(this, &pStream));
    
        *ppStream = pStream;
        pStream = NULL;
    
    Cleanup:
        ReleaseInterface(pStream);
        RRETURN(hr);
}

