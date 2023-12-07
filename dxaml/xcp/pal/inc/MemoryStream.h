// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//  Abstract:
//      Implementation of IPALStream around an in memory data buffer

class CMemoryStreamBuffer;
class CMemoryStream final : public IPALStream
{
public:
    static  _Check_return_ HRESULT Create(_In_ CMemoryStreamBuffer* pData, _Outptr_ CMemoryStream **ppMemStream);

    _Check_return_ XUINT32 AddRef() override;
    _Check_return_ XUINT32 Release() override;
    _Check_return_ HRESULT Clone(_Outptr_result_maybenull_ IPALStream **ppNewStream) override;
    _Check_return_ HRESULT Read(_Out_writes_bytes_(cb) void* pv, XUINT32 cb,  _Out_opt_ XUINT32* pcbRead) override;
    _Check_return_ HRESULT Seek(XINT64 qwMove, PALSeekOrigin eSeekOrigin,  _Out_opt_ XUINT64* pqwNewPosition) override;
    _Check_return_ HRESULT SetSize(XUINT64 qwNewSize) override;
    _Check_return_ HRESULT GetSize(_Out_opt_ XUINT64* pqwSize) override;
    _Check_return_ HRESULT GetOffset(_Out_ XUINT64 *pqOffset) override;
    _Check_return_ HRESULT GetPosition(_Out_ XUINT64 *pqPosition) override;
    _Check_return_ HRESULT Write(_In_reads_bytes_(cb) const void *pv, XUINT32 cb, XINT32 offset, _Out_opt_ XUINT32 *pcbWritten) override;
    XUINT32 CanSeek() override { return (m_capabilities & PALSTREAM_CAPS_SEEKABLE) != 0 ? TRUE : FALSE; };
    XUINT32 GetCapabilities( ) override { return m_capabilities; };

private:
    ~CMemoryStream() override;
    CMemoryStream(_In_ CMemoryStreamBuffer* pData);

private:
    XINT32 m_cRef;
    XUINT32 m_capabilities;
    CMemoryStreamBuffer* m_pData;
    XUINT64 m_qPosition;
};



