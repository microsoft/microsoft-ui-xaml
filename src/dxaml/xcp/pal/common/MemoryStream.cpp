// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

//------------------------------------------------------------------------
//
//  Method:   Create
//
//  Synopsis:
//      Create an instance of the Stream object
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CMemoryStream::Create(_In_ CMemoryStreamBuffer* pData, _Outptr_ CMemoryStream **ppMemStream)
{
    HRESULT hr = S_OK;
    CMemoryStream *pMemStream = NULL;

    IFCPTR(ppMemStream);
    *ppMemStream = NULL;

    pMemStream = new CMemoryStream(pData);

    *ppMemStream = pMemStream;
    pMemStream = NULL;

Cleanup:
    ReleaseInterface(pMemStream);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   Constructor
//
//  Synopsis:
//      Private constructor. Create needs to be call to handle proper
//      initialization failures.
//
//------------------------------------------------------------------------
CMemoryStream::CMemoryStream(_In_ CMemoryStreamBuffer* pData)
: m_cRef(1)
{
    m_pData = pData;
    m_capabilities = PALSTREAM_CAPS_READABLE;
    pData->AddRef();
    m_qPosition = 0;
}

//------------------------------------------------------------------------
//
//  Method:   Destructor
//
//------------------------------------------------------------------------
CMemoryStream::~CMemoryStream()
{
    if(m_pData)
    {
        ReleaseInterface(m_pData);
    }
    m_pData = NULL;
}

//------------------------------------------------------------------------
//
//  Method:   AddRef
//
//  Synopsis:
//      Increment the reference count on the object
//
//------------------------------------------------------------------------
_Check_return_ XUINT32 CMemoryStream::AddRef()
{
    return PAL_InterlockedIncrement(&m_cRef);
}

//------------------------------------------------------------------------
//
//  Method:   Release
//
//  Synopsis:
//      Release one reference of the object. Delete the object when the
//      last reference is gone.
//
//------------------------------------------------------------------------
_Check_return_ XUINT32 CMemoryStream::Release()
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
//  Method:   Clone
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CMemoryStream::Clone(_Outptr_result_maybenull_ IPALStream **ppNewStream)
{ 
    ASSERT(FALSE); 
    RRETURN(E_NOTIMPL);
};

//------------------------------------------------------------------------
//
//  Method:   Seek
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CMemoryStream::Seek(XINT64 qwMove, PALSeekOrigin eSeekOrigin,  _Out_opt_ XUINT64* pqwNewPosition)
{ 
    ASSERT(FALSE); 
    RRETURN(E_NOTIMPL);
};

//------------------------------------------------------------------------
//
//  Method:   SetSize
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CMemoryStream::SetSize(XUINT64 qwNewSize)
{ 
    ASSERT(FALSE); 
    RRETURN(E_NOTIMPL);
};

//------------------------------------------------------------------------
//
//  Method:   GetOffset
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CMemoryStream::GetOffset(_Out_ XUINT64 *pqOffset)
{ 
    ASSERT(FALSE); 
    RRETURN(E_NOTIMPL);
};

//------------------------------------------------------------------------
//
//  Method:   GetPosition
//
//  Synopsis:
//          Get the position of the current read cursor
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CMemoryStream::GetPosition(_Out_ XUINT64 *pqPosition)
{
    *pqPosition = m_qPosition;
    RRETURN(S_OK);
}

//------------------------------------------------------------------------
//
//  Method:   Release
//
//  Synopsis:
//      Release one reference of the object. Delete the object when the
//      last reference is gone.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CMemoryStream::Write(_In_reads_bytes_(cb) const void *pv, XUINT32 cb, XINT32 offset, _Out_opt_ XUINT32 *pcbWritten)
{ 
   HRESULT hr = S_OK;
   if(m_pData) 
    {
        IFC(m_pData->Write(pv, cb, offset));
        if (pcbWritten)
        {
            *pcbWritten = cb;
        }
    }
    else
    {
        hr = E_UNEXPECTED;
    }
   
   Cleanup:
   RRETURN (hr);
};

//------------------------------------------------------------------------
//
//  Method:   Read
//
//  Synopsis:
//      Read n bytes from the current stream position
//
//------------------------------------------------------------------------

_Check_return_ HRESULT CMemoryStream::Read(_Out_writes_bytes_(cb) void* pv, XUINT32 cb, _Out_opt_ XUINT32* pcbRead)
{
    HRESULT hr = S_OK;
    ASSERT(m_pData);

    IFC(m_pData->Read(pv, cb, pcbRead));

    if (pcbRead)
    {
        m_qPosition += *pcbRead;
    }

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   GetSize
//
//  Synopsis:
//      Get the current size of the stream
//
//------------------------------------------------------------------------

_Check_return_ HRESULT CMemoryStream::GetSize(_Out_opt_ XUINT64* pqwSize)
{
    HRESULT hr = S_OK;

    IFCPTR(pqwSize);

    if (m_pData == NULL)
    {
        IFC(E_FAIL);
    }

    *pqwSize = m_pData->Size();
Cleanup:
    RRETURN(hr);
}

