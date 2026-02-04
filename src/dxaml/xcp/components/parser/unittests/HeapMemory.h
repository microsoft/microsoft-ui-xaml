// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// This class provides the IPALMemory interface for a refcounted Heap memory

class CHeapMemory final : public IPALMemory
{
 private:
     CHeapMemory()
     {
            m_pData = NULL;
            m_cSize = 0;
            m_cRef = 1;
            m_bOwnMemory = TRUE;
     }

protected:
    virtual ~CHeapMemory();
 public:

    static _Check_return_ HRESULT Create(XUINT32 _In_ cSize, _Outptr_ IPALMemory **ppPalMemory);
    static _Check_return_ HRESULT Create(XUINT32 _In_ cSize, _In_ XUINT8 *pMemory, _Outptr_ IPALMemory **ppPalMemory, XUINT8 bOwnMemory = TRUE);

    virtual XUINT32     AddRef()  const;
    virtual XUINT32     Release() const;
    virtual void*       GetAddress() const;
    virtual XUINT32     GetSize() const;

private:
    mutable XUINT32 m_cRef;
    _Field_size_opt_(m_cSize) XUINT8 *m_pData;
    XUINT32 m_cSize;
    XUINT8 m_bOwnMemory;
};


