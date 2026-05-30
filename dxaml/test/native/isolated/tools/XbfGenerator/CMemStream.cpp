// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "CMemStream.h"

#define min(a,b)            (((a) < (b)) ? (a) : (b))

// construction and destruction
// construction and destruction
CMemStream::CMemStream() :
m_position(0),
m_cRef(1),
m_size(0)
{
}

CMemStream::~CMemStream() { }


///// IUnknown methods
STDMETHODIMP CMemStream::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IStream))
    {
        *ppvObj = static_cast<IStream *> (this);
        AddRef();
        return NOERROR;
    }
    *ppvObj = NULL;
    return E_NOINTERFACE;
}
STDMETHODIMP_(ULONG) CMemStream::AddRef()
{
    return ++m_cRef;
}
STDMETHODIMP_(ULONG) CMemStream::Release()
{
    if (--m_cRef == 0L)
    {
        delete this;
        return 0;
    }
    return m_cRef;
}

/* IStream methods */
STDMETHODIMP CMemStream::Read(void* pv, ULONG cb, ULONG* pcbRead)
{
    if (m_buffer.empty() || !pv)
    {
        return E_FAIL;
    }
    std::copy_n(m_buffer.begin(), min(cb, m_size), static_cast<UCHAR*>(pv));

    if (pcbRead != NULL)
    {
        *pcbRead = min(cb, static_cast<ULONG>(m_size));
    }
    return S_OK;

}
STDMETHODIMP CMemStream::Write(const void* pv, ULONG cb, ULONG* pcbWritten)
{

    if (!pv)
    {
        return E_FAIL;
    }

    if ((m_position + cb) > static_cast<ULONG>(m_size))
    {
        m_buffer.resize(m_position + cb);
    }

    std::copy_n(static_cast<const UCHAR*>(pv), cb, &m_buffer[m_position]);
    m_size = m_buffer.size();

    if (pcbWritten != NULL)
    {
        *pcbWritten = cb;
    }
    return S_OK;
}

STDMETHODIMP CMemStream::Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER* plibNewPosition)
{
    switch (dwOrigin)
    {
    case SEEK_END:
        m_position = static_cast<ULONG>(m_size); // current size always reflects last byte
        break;
    case SEEK_SET:
        m_position = 0;
        break;
    }
    m_position += dlibMove.LowPart;    //dlibMove.lowpart is the offset

    if (m_position == (DWORD)-1)
    {
        return E_FAIL;
    }

    if (plibNewPosition != NULL)
    {
        plibNewPosition->LowPart = m_position;
        plibNewPosition->HighPart = dlibMove.HighPart;
    }
    return S_OK;
}
STDMETHODIMP CMemStream::SetSize(ULARGE_INTEGER /*libNewSize*/)
{
    return E_NOTIMPL;
}
STDMETHODIMP CMemStream::CopyTo(IStream* /*pstm */, ULARGE_INTEGER /*cb*/,
    ULARGE_INTEGER* /*pcbRead*/,
    ULARGE_INTEGER* /*pcbWritten*/)
{
    return E_NOTIMPL;
}
STDMETHODIMP CMemStream::Commit(DWORD /*grfCommitFlags*/)
{
    return E_NOTIMPL;
}
STDMETHODIMP CMemStream::Revert()
{
    return E_NOTIMPL;
}
STDMETHODIMP CMemStream::LockRegion(ULARGE_INTEGER /*libOffset*/, ULARGE_INTEGER /*cb*/,
    DWORD /*dwLockType*/)
{
    return E_NOTIMPL;
}
STDMETHODIMP CMemStream::UnlockRegion(ULARGE_INTEGER /*libOffset*/, ULARGE_INTEGER /*cb*/,
    DWORD /*dwLockType*/)
{
    return E_NOTIMPL;
}
STDMETHODIMP CMemStream::Stat(STATSTG* /*pstatstg*/, DWORD /*grfStatFlag*/)
{
    return E_NOTIMPL;
}
STDMETHODIMP CMemStream::Clone(IStream** /*ppstm*/)
{
    return E_NOTIMPL;
}