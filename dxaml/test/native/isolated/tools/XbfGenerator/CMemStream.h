// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "Microsoft.UI.Xaml.coretypes.h"

/**
    CMemStream for testing purposes only. Is not thread safe, use only in single threaded application.
*/
class CMemStream : public IStream
{
#define SEEK_END 2
#define SEEK_SET 0
public:
    ///// object state
    std::vector<unsigned char>  m_buffer;         // object reference count
    unsigned long               m_position;
    unsigned int                m_cRef;
    size_t               m_size;
    // file handle

    // construction and destruction
    CMemStream();

    ~CMemStream();


    ///// IUnknown methods
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID FAR* ppvObj);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    /* IStream methods */
    STDMETHODIMP Read(void* pv, ULONG cb, ULONG* pcbRead);
    STDMETHODIMP Write(const void* pv, ULONG cb, ULONG* pcbWritten);
    STDMETHODIMP Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER* plibNewPosition);
    STDMETHODIMP SetSize(ULARGE_INTEGER /*libNewSize*/);
    STDMETHODIMP CopyTo(IStream* /*pstm */, ULARGE_INTEGER /*cb*/,
        ULARGE_INTEGER* /*pcbRead*/,
        ULARGE_INTEGER* /*pcbWritten*/);
    STDMETHODIMP Commit(DWORD /*grfCommitFlags*/);
    STDMETHODIMP Revert();
    STDMETHODIMP LockRegion(ULARGE_INTEGER /*libOffset*/, ULARGE_INTEGER /*cb*/,
        DWORD /*dwLockType*/);
    STDMETHODIMP UnlockRegion(ULARGE_INTEGER /*libOffset*/, ULARGE_INTEGER /*cb*/,
        DWORD /*dwLockType*/);
    STDMETHODIMP Stat(STATSTG* /*pstatstg*/, DWORD /*grfStatFlag*/);
    STDMETHODIMP Clone(IStream** /*ppstm*/);
};