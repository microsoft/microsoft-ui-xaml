// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CWinDataStreamBuffer : public IPALDataStreamBuffer
{
public:
    static HRESULT Create(_Outptr_result_maybenull_ CWinDataStreamBuffer **ppWinBuffer);
    static HRESULT Create(_In_ XBYTE fCreateMemoryStream, _In_ XUINT32 cbMaxBytes, _Outptr_result_maybenull_ CWinDataStreamBuffer **ppWinBuffer);
    static HRESULT Create(_In_ XUINT32 cbBuffer,
                          _In_reads_opt_(cbBuffer) XUINT8* pBuffer,
                          _Outptr_result_maybenull_ CWinDataStreamBuffer **ppWinBuffer);
    static HRESULT CreateFromIPalMemory(_In_ IPALMemory *pPalMemory, _Outptr_  CWinDataStreamBuffer **ppWinBuffer);

    virtual _Check_return_ HRESULT Write(_In_reads_bytes_(cb) void const *pData,
                                        _In_ XUINT32 cb,
                                        _In_ XUINT32 cbOffset,
                                        _In_opt_ IStream *pStream = NULL) = 0;
    virtual _Check_return_ HRESULT Read( _Out_writes_bytes_(cb) void* pv,
                                        _In_ XUINT32 cbOffset,
                                        _In_ XUINT32 cb,
                                        _Out_opt_ XUINT32* pcbRead) = 0;
    virtual _Check_return_ HRESULT SetSize(XUINT32 cb) = 0;

    virtual XUINT32 Length() = 0;
    virtual XUINT32 Size() = 0;
    virtual XUINT32 GetOffset() = 0;
    virtual bool SupportFastSeek() { return m_bSupportsFastSeek; };
    void SetFastSeekSupport(bool bSupported) {m_bSupportsFastSeek = bSupported;}

    _Check_return_ HRESULT Write(_In_reads_bytes_(cb) const void *pv, XUINT32 cb, XINT32 offset) override;
    _Check_return_ HRESULT CreateStream(_Out_ IPALStream** ppStream) override;

    virtual void SetFinalDownloadSize( _In_ XUINT32 cbFinalSize ) = 0 ;

protected:
    bool m_bSupportsFastSeek;
};

class CWinMemoryStreamBuffer final : public CWinDataStreamBuffer
{
public:
    CWinMemoryStreamBuffer()
    {
        m_pBuffer = NULL;
        m_cRef = 0;
    }
    ~CWinMemoryStreamBuffer() override
    {
        ReleaseInterface(m_pBuffer);
    }
    static HRESULT Create( _In_ XUINT32 cbMaxBytes, _Outptr_result_maybenull_ CWinMemoryStreamBuffer **ppWinBuffer);

    XUINT32 AddRef() override;
    XUINT32 Release() override;

    _Check_return_ HRESULT Write(_In_reads_bytes_(cb) void const *pData,
                                        _In_ XUINT32 cb,
                                        _In_ XUINT32 cbOffset,
                                        _In_opt_ IStream *pStream = NULL) override
        {return m_pBuffer->Write(pData, cb, cbOffset);} ;
    _Check_return_ HRESULT Read( _Out_writes_bytes_(cb) void* pv,
                                        _In_ XUINT32 cbOffset,
                                        _In_ XUINT32 cb,
                                        _Out_opt_ XUINT32* pcbRead) override
        {return m_pBuffer->Read(pv, cb, pcbRead);};
    _Check_return_ HRESULT SetSize(XUINT32 cb) override;

    XUINT32 Length() override{ return m_pBuffer->Size(); };
    XUINT32 Size() override { return m_pBuffer->Size(); };
    XUINT32 GetOffset() override { return 0; };
    bool SupportFastSeek() override { return false; };
    void SetFastSeekSupport(bool bSupported) {};
    _Check_return_ HRESULT Write(_In_reads_bytes_(cb) const void *pv, XUINT32 cb, XINT32 offset) override
        {return m_pBuffer->Write(pv, cb, offset);};
    _Check_return_ HRESULT CreateStream(_Out_ IPALStream** ppStream) override
        {return m_pBuffer->CreateStream(ppStream);};

    void SetFinalDownloadSize( _In_ XUINT32 cbFinalSize ) override { };  // Does nothing for MemoryStreamBuffer.

protected:
    CMemoryStreamBuffer* m_pBuffer;
    XUINT32 m_cRef;
};

class CMappedMemory;

class CWinHttp11StreamBuffer final : public CWinDataStreamBuffer
{
private:
    struct StreamDataChunk
    {
    public:
        StreamDataChunk(XUINT32 cbOffset = 0, XUINT32 cbLength = 0, StreamDataChunk *pNext = NULL)
        {
            m_cbOffset = cbOffset;
            m_cbLength = cbLength;
            m_pNext = pNext;
        }
        ~StreamDataChunk()
        {
        }

        XUINT32 m_cbOffset;
        XUINT32 m_cbLength;
        StreamDataChunk *m_pNext;
    };

private:
    class StreamDataChunkList
    {
    public:
        StreamDataChunkList()
        {
            m_pHead = NULL;
            m_pCurrentWriteChunk = NULL;
            m_pCurrentReadChunk = NULL;
        }
        ~StreamDataChunkList()
        {
            StreamDataChunk *pTemp = NULL;
            StreamDataChunk *pCurrent = m_pHead;
            while(pCurrent)
            {
                pTemp = pCurrent->m_pNext;
                delete pCurrent;
                pCurrent = pTemp;
            }

            // m_pCurrentWriteChunk and m_pCurrentReadChunk should be removed
            // in above loop, so no need to explicitly delete them again here.
        }
        static _Check_return_ HRESULT Create(_Out_ StreamDataChunkList **ppList);
        _Check_return_ HRESULT Write(_In_ XUINT32 cbOffset, _In_ XUINT32 cbLength);
        XUINT32 DataAvailable(_In_ XUINT32 cbOffset, _In_ XUINT32 cbLength);

        StreamDataChunk *GetCurrentWriteChunk( )
        {
            return m_pCurrentWriteChunk;
        }

        StreamDataChunk *GetCurrentReadChunk( )
        {
            return m_pCurrentReadChunk;
        }

    private:
        StreamDataChunk *GetFloorChunk( _In_ XUINT32 cbOffset, _In_ StreamDataChunk *pCurrent);
        StreamDataChunk *m_pHead;
        StreamDataChunk *m_pCurrentWriteChunk;
        StreamDataChunk *m_pCurrentReadChunk;
    };

public:
    static HRESULT Create(_In_ XUINT32 cbMaxBytes, _Outptr_result_maybenull_ CWinHttp11StreamBuffer **ppBuffer);

    XUINT32 AddRef() override;
    XUINT32 Release() override;

    // IStream is only used for IE to save double writting the buffer.
    _Check_return_ HRESULT Write(_In_reads_bytes_(cbLength) void const *pData,
                                        _In_ XUINT32 cbLength,
                                        _In_ XUINT32 cbOffset,
                                        _In_opt_ IStream *pStream = NULL) override;
    _Check_return_ HRESULT Read(_Out_writes_bytes_(cbLength) void* pv,
                                       _In_ XUINT32 cbOffset,
                                       _In_ XUINT32 cbLength,
                                       _Out_opt_ XUINT32* pcbRead) override;
    _Check_return_ HRESULT SetSize(_In_ XUINT32 cb) override;

    XUINT32 Length() override;
    XUINT32 Size() override;
    XUINT32 GetOffset() override;

    void SetFinalDownloadSize( _In_ XUINT32 cbFinalSize ) override;

private:
    ~CWinHttp11StreamBuffer() override;
    CWinHttp11StreamBuffer();

    _Check_return_ HRESULT CreateMediaMappedMemory(
                                    _In_ XUINT32 nOffset,
                                    _In_ XUINT32 bReadOnly,
                                    _Out_ CMappedMemory **ppMappedMemory);
    _Check_return_ HRESULT CopyViewData(
                                    _In_ CMappedMemory *pView,
                                    _In_ XUINT32 cbOffset,
                                    _In_reads_bytes_(*pcb) XUINT8* pv,
                                    _Inout_ XUINT32 *pcb,
                                    _In_ XUINT32 bRead,
                                    _In_ IStream *pStream = NULL);

    _Check_return_ HRESULT UpdateMappingSize(_In_ XUINT32 cbSize);


   XCP_FORCEINLINE StreamDataChunk *GetCurrentWriteChunk( )
   {
       return (m_pChunks ? m_pChunks->GetCurrentWriteChunk( ) : NULL);
   }

   XCP_FORCEINLINE StreamDataChunk *GetCurrentReadChunk( )
   {
       return (m_pChunks ? m_pChunks->GetCurrentReadChunk( ) : NULL);
   }

private:
    XHANDLE m_hMapFile; // The file mapping
    XHANDLE m_hTmpFile; // The source temp file
    StreamDataChunkList *m_pChunks;
    CMappedMemory *m_pReadView;
    CMappedMemory *m_pWriteView;

    // System Granularity is static to avoid querying the file system
    // too often.
    static XUINT32 m_cbSysGran;
    XUINT32 m_cbDataSize;
    XUINT32 m_cbPageSize;
    XUINT32 m_cbViewSize;
    XUINT32 m_cRef;
    wil::critical_section m_Lock;
};

class CWinBasicStreamBuffer final : public CWinDataStreamBuffer
{
public:
    static HRESULT Create(_Outptr_result_maybenull_ CWinBasicStreamBuffer **ppWinBuffer);
    static HRESULT Create(_In_ XUINT32 cbMaxBytes, _Outptr_result_maybenull_ CWinBasicStreamBuffer **ppWinBuffer);
    static HRESULT Create(_In_ XUINT32 cbBuffer,
                          _In_reads_opt_(cbBuffer) XUINT8* pBuffer,
                          _Outptr_result_maybenull_ CWinBasicStreamBuffer **ppWinBuffer);
    static HRESULT CreateFromIPalMemory(_In_ IPALMemory *pPalMemory, _Outptr_  CWinBasicStreamBuffer **ppWinBuffer);

    XUINT32 AddRef() override;
    XUINT32 Release() override;

    _Check_return_ HRESULT Write(_In_reads_bytes_(cb) void const *pData,
                                        _In_ XUINT32 cb,
                                        _In_ XUINT32 cbOffset,
                                        _In_ IStream *pStream = NULL) override;
    _Check_return_ HRESULT Read(_Out_writes_bytes_(cb) void* pv, _In_ XUINT32 cbOffset, _In_ XUINT32 cb,  _Out_opt_ XUINT32* pcbRead) override;
    _Check_return_ HRESULT SetSize(_In_ XUINT32 cb) override;

    XUINT32 Length() override;
    XUINT32 Size() override;
    XUINT32 GetOffset() override;

    void SetFinalDownloadSize( _In_ XUINT32 cbFinalSize ) override;

private:
    ~CWinBasicStreamBuffer() override;
    CWinBasicStreamBuffer();

private:
    _Field_size_(m_size) XUINT8* m_pData;
    XUINT32 m_size;
    XUINT32 m_cRef;
    XUINT32 m_cursor;
    wil::critical_section m_Lock;
    IPALMemory *m_pPalMemory;

};


class CWinDataStream final : public IPALStream
{
public:
    static HRESULT Create(_In_opt_ CWinDataStreamBuffer* pDataBuffer, _Outptr_result_maybenull_ CWinDataStream **ppWinStream);

    XUINT32 AddRef() override;
    XUINT32 Release() override;
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
    ~CWinDataStream() override;
    CWinDataStream();

public:
    // Making it public is a hack to make it easy to reuse buffer when doing byte range request
    CWinDataStreamBuffer* m_pBuffer;
private:
    XUINT32 m_cRef;
    XUINT32 m_cursor;
    XUINT32 m_capabilities;

};

