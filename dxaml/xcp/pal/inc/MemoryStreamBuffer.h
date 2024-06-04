// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <XcpAllocation.h>

class CMemoryStreamBuffer final : public IPALDataStreamBuffer
{
public:
    _Check_return_ HRESULT Read( _Out_writes_bytes_(cb) void* pv,
                                        _In_ XUINT32 cb,
                                        _Out_opt_ XUINT32* pcbRead);
    XUINT32 Size() {
        XUINT32 size;
        {
            auto lock = m_Lock.lock();
            size = m_uSize;
        }
        return size;
   }


//IPALDataStreamBuffer members
    XUINT32 AddRef() override;
    XUINT32 Release() override;
    _Check_return_ HRESULT Write(_In_reads_bytes_(cb) const void *pv, XUINT32 cb, XINT32 offset) override;
    _Check_return_ HRESULT CreateStream(_Out_ IPALStream** ppStream) override;


    CMemoryStreamBuffer( _In_ XUINT32 cbMaxBytes)
    {
        m_uCurrentBlockOffset = 0;
        m_pBlockFirst = NULL;
        m_pBlockLast = NULL;
        m_cRef = 1;
        m_uSize = cbMaxBytes;
        m_bUpdateSize = (cbMaxBytes == (XUINT32) -1) ? 1 : 0;
    };

    ~CMemoryStreamBuffer() override;

    private:
    class CMemoryStreamBufferBlock
    {
        public:
            CMemoryStreamBufferBlock()
            {
                m_pBlock = NULL;
                m_uSize = 0;
                m_pNext = NULL;
            };

            ~CMemoryStreamBufferBlock()
            {
                delete [] m_pBlock;
                m_pBlock = NULL;
                m_uSize = 0;
                m_pNext = NULL;
            };

            _Check_return_ HRESULT Write(_In_ XUINT32 uSize, _In_reads_bytes_(uSize) void const *pData)
            {
                HRESULT hr = S_OK;
                m_pBlock = new XUINT8[uSize];
                m_uSize = uSize;
                m_pNext = NULL;
                memcpy(m_pBlock, (XUINT8*)pData,uSize);
                RRETURN(hr);//RRETURN_REMOVAL
            };

            _Check_return_ HRESULT Read( _Out_writes_bytes_(cb) void* pv,
                                                _In_ XUINT32 uOffset,
                                                _In_ XUINT32 cb,
                                                _Out_opt_ XUINT32* pcbRead)
            {
                HRESULT hr = S_OK;
                XUINT32 len = cb + uOffset;

                ASSERT(m_pBlock);

                if((uOffset > m_uSize) || (len < cb))
                {
                    IFC(E_INVALIDARG);
                };

                if(len > m_uSize)
                {
                    cb = m_uSize - uOffset;
                }

                memcpy(pv, m_pBlock+uOffset, cb);

                *pcbRead = cb;

                Cleanup:
                    RRETURN (hr);

            };

            void SetNext( _In_ CMemoryStreamBufferBlock* pNext)
            {
                m_pNext = pNext;
            }

            CMemoryStreamBufferBlock* GetNext() { return m_pNext;};
            XUINT32 GetSize() { return m_uSize;};


        private:
            XUINT8* m_pBlock;
            XUINT32 m_uSize;
            CMemoryStreamBufferBlock* m_pNext;

    };

private:
    XUINT32 m_uCurrentBlockOffset;
    CMemoryStreamBufferBlock* m_pBlockFirst;
    CMemoryStreamBufferBlock* m_pBlockLast;
    wil::critical_section m_Lock;
    XINT32 m_cRef;
    XUINT32 m_uSize;
    bool m_bUpdateSize;
};

