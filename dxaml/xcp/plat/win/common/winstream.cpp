// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

//------------------------------------------------------------------------
//
//  Method:   StreamDataChunkList::Create
//
//  Synopsis:
//      Create an instance of the StreamDataChunkList::Create object
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CWinHttp11StreamBuffer::StreamDataChunkList::Create(_Out_ StreamDataChunkList **ppList)
{
    HRESULT hr = S_OK;
    StreamDataChunkList *pNewList = NULL;
    StreamDataChunk     *pHead = NULL;

    IFCPTR(ppList);

    pNewList = new StreamDataChunkList();

    pHead = new StreamDataChunk(0,0,NULL);

    pNewList->m_pHead = pHead;
    pNewList->m_pCurrentWriteChunk = pHead;
    pNewList->m_pCurrentReadChunk = pHead;
    pHead = NULL;

    *ppList = pNewList;
    pNewList = NULL;

Cleanup:

    delete pHead;
    delete pNewList;

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   StreamDataChunkList::Write
//
//  Synopsis:
//      This will mark the data offsets as written to the current chunklist.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CWinHttp11StreamBuffer::StreamDataChunkList::Write(
                                                _In_ XUINT32 cbOffset,
                                                _In_ XUINT32 cbLength)
{
    HRESULT hr = S_OK;
    StreamDataChunk *pClose = NULL;
    StreamDataChunk *pTemp = NULL;
    StreamDataChunk *pCurrent = NULL;
    IFCEXPECT(cbLength > 0);

    // Determine the closest node lower than cbOffset.
    pClose = GetFloorChunk(cbOffset, m_pCurrentWriteChunk);
    pCurrent = pClose;

    // Check if the new data will just extend pClose
    if ((cbOffset >= pClose->m_cbOffset) && (cbOffset <= pClose->m_cbOffset + pClose->m_cbLength))
    {
        // Increase the size of pClose with the new length size.
        if (cbOffset + cbLength > pClose->m_cbOffset + pClose->m_cbLength)
        {
            pClose->m_cbLength = (cbOffset + cbLength) - pClose->m_cbOffset;
        }
        // Otherwise we already have this data downloaded, do nothing.
        else
        {
            goto Cleanup;
        }
    }
    // Otherwise create a new node for the data
    else
    {
        pTemp = new StreamDataChunk(cbOffset, cbLength, pClose->m_pNext);
        pClose->m_pNext = pTemp;
        // Update the current chunk
        pCurrent = pTemp;
    }

    // Loop combining nodes if needed.  This will happen if the length of pClose now extends it beyond
    // the start of the next chunk, requiring the chunks to be combined.
    while((pClose->m_pNext) && (pClose->m_cbOffset + pClose->m_cbLength >= pClose->m_pNext->m_cbOffset))
    {
        ASSERT( pCurrent == pClose );

        pTemp = pClose->m_pNext;
        // Add the next node's extra bytes if needed
        if (pClose->m_cbOffset + pClose->m_cbLength < pTemp->m_cbOffset + pTemp->m_cbLength)
        {
            pClose->m_cbLength = (pTemp->m_cbOffset + pTemp->m_cbLength) - pClose->m_cbOffset;
        }
        // Update list to collapse the next node.
        pClose->m_pNext = pTemp->m_pNext;

        if (pTemp == m_pCurrentReadChunk)
        {
            // The cached m_pCurrentReadChunk will be deleted, update it
            // here to store a new one.

            // The upper level's Http11StreamBuffer class has a lock for Write/Read/Query,
            // so it is guaranteed that m_pCurrentReadChunk is not in use when Buffer is
            // written and the data chunk node is merged here.

            m_pCurrentReadChunk = pClose;
        }

        delete pTemp;
        // Update the current chunk
        pCurrent = pClose;
    }

Cleanup:

    if (SUCCEEDED(hr))
    {
        // save pCurrent in m_pCurrentWriteChunk field.
        m_pCurrentWriteChunk = pCurrent;
    }

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   StreamDataChunkList::DataAvailable
//
//  Synopsis:
//      Returns number of bytes available. Hence, a return value of 0
//    indicates no data is available.
//
//------------------------------------------------------------------------
XUINT32
CWinHttp11StreamBuffer::StreamDataChunkList::DataAvailable(
                                                _In_ XUINT32 cbOffset,
                                                _In_ XUINT32 cbLength)
{
    StreamDataChunk *pTemp = GetFloorChunk(cbOffset, m_pCurrentReadChunk);
    ASSERT( pTemp != NULL );

    if (cbOffset < pTemp->m_cbOffset + pTemp->m_cbLength)
    {
        m_pCurrentReadChunk = pTemp;
        return std::min(pTemp->m_cbOffset + pTemp->m_cbLength - cbOffset, cbLength);
    }

    return 0;
}

//------------------------------------------------------------------------
//
//  Method:   StreamDataChunkList::GetFloorChunk
//
//  Synopsis:
//      Gets the chunk containing the provided offset.  If the offset
//    hasn't been written yet this will return the preceeding chunk.
//
//------------------------------------------------------------------------
CWinHttp11StreamBuffer::StreamDataChunk *
CWinHttp11StreamBuffer::StreamDataChunkList::GetFloorChunk(_In_ XUINT32 cbOffset, _In_ StreamDataChunk *pCurrent)
{
    StreamDataChunk *pTemp = pCurrent;

    if (pTemp == NULL || cbOffset < pTemp->m_cbOffset)
    {
        pTemp = m_pHead;
    }

    while (pTemp->m_pNext != 0 && cbOffset >= pTemp->m_pNext->m_cbOffset)
    {
        pTemp = pTemp->m_pNext;
    }

    RRETURN(pTemp);
}

//------------------------------------------------------------------------
//
//  Method:   CWinHttp11StreamBuffer::m_cbSysGran
//
//  Synopsis:
//      System granularity cached to save time.
//
//------------------------------------------------------------------------
XUINT32 CWinHttp11StreamBuffer::m_cbSysGran = 0;

//------------------------------------------------------------------------
//
// CWinHttp11StreamBuffer::CWinHttp11StreamBuffer
//
// ctor
//
//------------------------------------------------------------------------
CWinHttp11StreamBuffer::CWinHttp11StreamBuffer()
{
    m_cRef = 0;
    m_hMapFile = NULL;
    m_hTmpFile = NULL;
    m_pChunks = NULL;
    m_pReadView = NULL;
    m_pWriteView = NULL;
    m_cbDataSize = 0;
    m_cbPageSize = 0;
    m_cbViewSize = 0;
    SetFastSeekSupport(TRUE);
}

//------------------------------------------------------------------------
//
// CWinHttp11StreamBuffer::~CWinHttp11StreamBuffer
//
// dtor
//
//------------------------------------------------------------------------
CWinHttp11StreamBuffer::~CWinHttp11StreamBuffer()
{
    if (m_pChunks)
    {
        delete m_pChunks;
        m_pChunks = NULL;
    }

    ReleaseInterface(m_pReadView);
    ReleaseInterface(m_pWriteView);

    if (m_hMapFile)
    {
        CloseHandle(m_hMapFile);
    }

    if (m_hTmpFile)
    {
        CloseHandle(m_hTmpFile);
    }
}

//------------------------------------------------------------------------
//
// CWinHttp11StreamBuffer::Create
//
// Create a CWinHttp11StreamBuffer with a set maximum size
//
//------------------------------------------------------------------------
HRESULT
CWinHttp11StreamBuffer::Create(XUINT32 cbMaxBytes, _Outptr_result_maybenull_ CWinHttp11StreamBuffer **ppWinMediaStream)
{
    HRESULT hr = S_OK;
    CWinHttp11StreamBuffer* pBuffer = NULL;
    WCHAR *pTmpPath = NULL;
    WCHAR *pTmpFile = NULL;

    IFCPTR(ppWinMediaStream);
    *ppWinMediaStream = NULL;

    pBuffer = new CWinHttp11StreamBuffer();

    pBuffer->AddRef();

    // Determine the system size granularity.
    if (m_cbSysGran == 0)
    {
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        IFCEXPECT(sysInfo.dwAllocationGranularity > 0);
        m_cbSysGran = sysInfo.dwAllocationGranularity;
    }
    // Determine file view size, within the system file granularity size
    pBuffer->m_cbViewSize = (MEDIASTREAM_PAGE_MAP_SIZE > m_cbSysGran) ?
        MEDIASTREAM_PAGE_MAP_SIZE : m_cbSysGran;

    // Determine the buffer size
    pBuffer->m_cbDataSize = cbMaxBytes;

    // Allocate string buffers for temp file name.
    pTmpPath = new WCHAR[MAX_PATH];
    pTmpFile = new WCHAR[MAX_PATH];

    // Create a temp file for storage.
    IFCEXPECT(GetTempPath(MAX_PATH, pTmpPath) > 0);
    IFCEXPECT(GetTempFileName(pTmpPath, L"XCP", 0, pTmpFile) > 0);
    // Open the temp file and grab a handle.
    pBuffer->m_hTmpFile = CreateFile(pTmpFile,
                                     GENERIC_READ | GENERIC_WRITE,
                                     0,
                                     NULL,
                                     CREATE_ALWAYS,
                                     FILE_FLAG_DELETE_ON_CLOSE,
                                     NULL);
    if (pBuffer->m_hTmpFile == INVALID_HANDLE_VALUE)
    {
        IFC(E_OUTOFMEMORY);
    }

    IFC(StreamDataChunkList::Create(&(pBuffer->m_pChunks)));

    ASSERT(pBuffer->GetCurrentReadChunk( ));
    ASSERT(pBuffer->GetCurrentWriteChunk( ));

    *ppWinMediaStream = pBuffer;
    pBuffer = NULL;

Cleanup:
    ReleaseInterface(pBuffer);

    delete[] pTmpPath;
    delete[] pTmpFile;

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method: CreateMediaMappedMemory
//
//  Synopsis:
//      Create a CMappedMemory object for a media file view.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CWinHttp11StreamBuffer::CreateMediaMappedMemory(
    _In_ XUINT32 cbOffset,
    _In_ XUINT32 bReadOnly,
    _Inout_ CMappedMemory **ppMappedMemory)
{
    HRESULT hr = S_OK;
    CMappedMemory* pMemory = NULL;
    void *pMapping = NULL;

    IFCPTR(ppMappedMemory);

    // Determine if the current view needs to be updated.
    if ((*ppMappedMemory != NULL) &&
        ((cbOffset < (*ppMappedMemory)->GetOffset()) ||
         (cbOffset >= (*ppMappedMemory)->GetOffset() + (*ppMappedMemory)->GetSize())))
    {
        ReleaseInterface(*ppMappedMemory);
    }

    if (*ppMappedMemory == NULL)
    {
        XUINT32 dwViewMode = (bReadOnly) ? FILE_MAP_READ : FILE_MAP_ALL_ACCESS;

        // Adjust the desired offset for system granularity
        cbOffset = cbOffset - cbOffset % m_cbSysGran;

        // Determine size of mapping, if there isn't a full view remaining, view the remaining size available.
        XUINT32 cbSize = (m_cbViewSize + cbOffset > m_cbPageSize) ?
            m_cbPageSize - cbOffset : m_cbViewSize;

        pMapping = MapViewOfFile(m_hMapFile, dwViewMode, 0, cbOffset, cbSize);
        IFCOOM(pMapping);

        pMemory = new CMappedMemory(NULL, NULL, pMapping, cbSize, cbOffset);
        pMapping = NULL;

        *ppMappedMemory = pMemory;
        pMemory = NULL;
    }
Cleanup:
    if (pMapping)
    {
        UnmapViewOfFile(pMapping);
    }
    ReleaseInterface(pMemory);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
// CWinHttp11StreamBuffer::AddRef
//
//
//
//------------------------------------------------------------------------
XUINT32 CWinHttp11StreamBuffer::AddRef()
{
    return ::InterlockedIncrement((LONG*) &m_cRef);
}

//------------------------------------------------------------------------
//
// CWinHttp11StreamBuffer::Release
//
//
//
//------------------------------------------------------------------------
XUINT32 CWinHttp11StreamBuffer::Release()
{
    XUINT32 cRef = ::InterlockedDecrement((LONG*) &m_cRef);

    if (cRef == 0)
    {
        delete this;
    }

    return cRef;
}

//------------------------------------------------------------------------
//
// CWinHttp11StreamBuffer::UpdateMappingSize
//
// Update the size of the filemapping object to allow for growth
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CWinHttp11StreamBuffer::UpdateMappingSize(_In_ XUINT32 cbSize)
{
    auto lock = m_Lock.lock();

    // We cannot go beyond the initial size yet
    if (cbSize > m_cbDataSize)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    // If we don't need to grow then we're fine
    if (cbSize > m_cbPageSize)
    {
        // We need to clear the old mapping and views and recreate them
        ReleaseInterface(m_pReadView);
        ReleaseInterface(m_pWriteView);

        if (m_hMapFile)
        {
            CloseHandle(m_hMapFile);
            m_hMapFile = NULL;
        }

        // Calculate the new view size. It should be a multiple of the Mapping Size
        // up to the file size
        XUINT32 cbSizeNew = (cbSize + MEDIASTREAM_MAPPING_SIZE - 1) / MEDIASTREAM_MAPPING_SIZE;
        cbSizeNew *= MEDIASTREAM_MAPPING_SIZE;

        cbSizeNew = std::min(cbSizeNew, m_cbDataSize);

        // Recreate the file mapping with the new size
        m_hMapFile = CreateFileMapping(m_hTmpFile,
                                       NULL,
                                       PAGE_READWRITE,
                                       0,
                                       cbSizeNew,
                                       NULL);
        IFCOOM_RETURN(m_hMapFile);
        m_cbPageSize = cbSizeNew;
    }
    return S_OK;
}

//------------------------------------------------------------------------
//
// CWinHttp11StreamBuffer::Write
//
// Add new data to the end of the data stream
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CWinHttp11StreamBuffer::Write(_In_reads_bytes_(cbLength) void const *pData, XUINT32 cbLength, XUINT32 cbOffset, _In_ IStream *pStream)
{
    XUINT32 cbWrite = 0;
    XUINT32 cbCursor = cbOffset;

    auto lock = m_Lock.lock();

    // Either pData or pStream needs to be set to provide valid data.
    IFCEXPECT_RETURN(pData || pStream);

    if ((cbOffset + cbLength) < cbOffset || cbOffset > m_cbDataSize)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    // Firefox can give us a packet that's bigger than the total size.
    // just trim it.
    if ((cbOffset + cbLength) > m_cbDataSize)
    {
        cbLength = m_cbDataSize - cbOffset;
    }

    IFC_RETURN(UpdateMappingSize(cbOffset + cbLength));

    while (cbWrite < cbLength)
    {
        // Update the current view if needed.
        IFC_RETURN(CreateMediaMappedMemory(
                cbCursor,
                FALSE,
                &(m_pWriteView)));

        // Determine write offset within the current view.
        XUINT32 cbViewOffset = cbCursor - m_pWriteView->GetOffset();

        // Determine the amount to write to the current view, and copy the memory.
        XUINT32 cWriteAmount = std::min(cbLength - cbWrite, m_pWriteView->GetSize() - cbViewOffset);
        IFC_RETURN(CopyViewData(m_pWriteView, cbCursor, (XUINT8 *)pData + cbWrite, &cWriteAmount, FALSE, pStream));
        cbWrite += cWriteAmount;
        cbCursor += cWriteAmount;
    }

    // Update the chunklist with the newly written data.
    IFC_RETURN(m_pChunks->Write(cbOffset, cbWrite));
    return S_OK;
}

//------------------------------------------------------------------------
//
// CWinHttp11StreamBuffer::CopyViewData
//
// Perform the actual memory copy required.  bRead determines if we should
// be reading data or writing it.  If pStream is set we assume writing as
// the IStream is provided only when new data is available from IE.  It is
// used here for performance, to avoid an additional copy.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CWinHttp11StreamBuffer::CopyViewData(_In_ CMappedMemory *pView,
                                    _In_ XUINT32 cbOffset,
                                    _In_reads_bytes_(*pcb) XUINT8* pv,
                                    _Inout_ XUINT32 *pcb,
                                    _In_ XUINT32 bRead,
                                    _In_ IStream *pStream)
{
    IFCPTR_RETURN(pView);
    IFCPTR_RETURN(pcb);
    IFCEXPECT_RETURN(pv || pStream);
    IFCEXPECT_RETURN(*pcb > 0);

    // Check that enough room was mapped, do not validate the actual data.
    IFCEXPECT_RETURN((cbOffset >= pView->GetOffset()) && (cbOffset + *pcb <= pView->GetOffset() + pView->GetSize()));
    XUINT32 cViewOffset = cbOffset - pView->GetOffset();

    // If stream is set then call read on the IStream directly into the page memory.
    if (pStream)
    {
        IFC_RETURN(pStream->Read((UINT8*)pView->GetAddress() + cViewOffset, *pcb, (ULONG*)pcb));
    }
    else
    {
        XUINT8 *pTarget = nullptr;
        XUINT8 *pSource = nullptr;

        // Determine which way the copy is going.
        if (bRead)
        {
            pTarget = pv;
            pSource = (UINT8*)pView->GetAddress() + cViewOffset;
        }
        else
        {
            pTarget = (UINT8*)pView->GetAddress() + cViewOffset;
            pSource = pv;
        }

        memcpy(pTarget, pSource, *pcb);
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
// CWinHttp11StreamBuffer::Read
//
// Read a block of data from the data stream
//
//------------------------------------------------------------------------
_Check_return_
HRESULT
CWinHttp11StreamBuffer::Read(
    _Out_writes_bytes_(cbLength) void* pv,
    XUINT32 cbOffset,
    XUINT32 cbLength,
    _Out_opt_ XUINT32* pcbRead
    )
{
    XUINT32 cbRead = 0;
    XUINT32 cbAvailable = 0;

    auto lock = m_Lock.lock();
    auto updateReadCount = wil::scope_exit([&]
    {
        if (pcbRead)
        {
            *pcbRead = cbRead;
        }
    });

    if ((cbOffset >= m_cbDataSize) || ((cbOffset + cbLength) < cbOffset))
    {
        IFC_RETURN(E_INVALIDARG);
    }

    if (cbLength == 0)
    {
        cbRead = 0;
        return S_OK;
    }

    // Adjust length requested if necessary
    if (cbOffset + cbLength > m_cbDataSize)
    {
        cbLength = m_cbDataSize - cbOffset;
    }

    // Determine how many bytes are available
    cbAvailable = m_pChunks->DataAvailable(cbOffset, cbLength);

    // If we don't have enough data, inform the calling that the operation is still pending.
    // This will cause a new byte-range request.
    if ( cbAvailable == 0 || cbAvailable < cbLength )
    {
        IFC_RETURN(E_PENDING);
    }

    while(cbRead < cbAvailable)
    {
        // Update the current view if needed.
        IFC_RETURN(CreateMediaMappedMemory(
                cbOffset,
                TRUE,
                &(m_pReadView)));

        // Determine the offset within the view.
        XUINT32 cbViewOffset = cbOffset - m_pReadView->GetOffset();

        // Determine amount to read from current view, and copy the memory.
        XUINT32 cbReadAmount = std::min(cbAvailable - cbRead, m_pReadView->GetSize() - cbViewOffset);
        HRESULT hrCopyView = CopyViewData(m_pReadView, cbOffset, (XUINT8 *)pv + cbRead, &cbReadAmount, TRUE);
        ASSERT( hrCopyView != E_PENDING );
        IFC_RETURN(hrCopyView);

        // Update counts
        cbRead += cbReadAmount;
        cbOffset += cbReadAmount;
    }
    return S_OK;
}

//------------------------------------------------------------------------
//
// CWinHttp11StreamBuffer::SetSize
//
// Set the length of the data
//
//------------------------------------------------------------------------
HRESULT CWinHttp11StreamBuffer::SetSize (XUINT32 cb)
{
    // Currently changing the size is not supported.
    RRETURN(E_NOTIMPL);
}

//------------------------------------------------------------------------
//
// CWinHttp11StreamBuffer::Length
//
// Get the length of the data in the buffer
//
//------------------------------------------------------------------------
XUINT32 CWinHttp11StreamBuffer::Length()
{
    XUINT32 len = 0;
    StreamDataChunk *pCurrentChunk = NULL;

    auto lock = m_Lock.lock();
    pCurrentChunk = GetCurrentWriteChunk();
    len = (pCurrentChunk) ? pCurrentChunk->m_cbOffset + pCurrentChunk->m_cbLength : 0;

    return len;
}

//------------------------------------------------------------------------
//
// CWinHttp11StreamBuffer::Size
//
// Get the allocated length of the buffer
//
//------------------------------------------------------------------------
XUINT32 CWinHttp11StreamBuffer::Size()
{
    XUINT32 size = 0;
    auto lock = m_Lock.lock();
    size = m_cbDataSize;

    return size;
}

//------------------------------------------------------------------------
//
// CWinHttp11StreamBuffer::GetOffset
//
// Get offset of the current chunk
//
//------------------------------------------------------------------------
XUINT32 CWinHttp11StreamBuffer::GetOffset()
{
    XUINT32 cbOffset = 0;
    StreamDataChunk *pCurrentChunk = NULL;

    auto lock = m_Lock.lock();
    pCurrentChunk = GetCurrentWriteChunk( );
    cbOffset = pCurrentChunk ? pCurrentChunk->m_cbOffset : 0;

    return cbOffset;
}

//------------------------------------------------------------------------
//
// CWinHttp11StreamBuffer::SetFinalDownloadSize
//
// Update final size when it has completed the downloading.
//
//------------------------------------------------------------------------
void CWinHttp11StreamBuffer::SetFinalDownloadSize( _In_ XUINT32 cbFinalSize )
{
    auto lock = m_Lock.lock();

    //
    // We support this for WHS server, it is enabled for non-BRR scenario only.
    // And the final size should always be smaller than initial total size,
    // otherwise, this function is no-op.
    //
    if (!SupportFastSeek())
    {
        if (cbFinalSize <= m_cbDataSize)
        {
            m_cbDataSize = cbFinalSize;
        }
        else
        {
            ASSERT(FALSE);
        }
    }
}

//------------------------------------------------------------------------
//
// CWinMemoryStreamBuffer::Create
//
// Create a CWinMemoryStreamBuffer
//
//------------------------------------------------------------------------
HRESULT
CWinMemoryStreamBuffer::Create( _In_ XUINT32 cbMaxBytes, _Outptr_result_maybenull_ CWinMemoryStreamBuffer **ppWinBuffer)
{
    HRESULT hr = S_OK;
    CWinMemoryStreamBuffer* pBuffer = NULL;

    IFCPTR(ppWinBuffer);
    *ppWinBuffer = NULL;

    pBuffer = new CWinMemoryStreamBuffer();

    pBuffer->AddRef();

    pBuffer->m_pBuffer = new CMemoryStreamBuffer(cbMaxBytes);

    *ppWinBuffer = pBuffer;
    pBuffer = NULL;

Cleanup:
    ReleaseInterface(pBuffer);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
// CWinMemoryStreamBuffer::AddRef
//
//
//
//------------------------------------------------------------------------
XUINT32 CWinMemoryStreamBuffer::AddRef()
{
    return ::InterlockedIncrement((LONG*) &m_cRef);
}

//------------------------------------------------------------------------
//
// CWinMemoryStreamBuffer::Release
//
//
//
//------------------------------------------------------------------------
XUINT32 CWinMemoryStreamBuffer::Release()
{
    XUINT32 cRef = ::InterlockedDecrement((LONG*) &m_cRef);

    if (cRef == 0)
    {
        delete this;
    }

    return cRef;
}

//------------------------------------------------------------------------
//
// CWinMemoryStreamBuffer::SetSize
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CWinMemoryStreamBuffer::SetSize(XUINT32 cb)
{
    ASSERT(FALSE);
    RRETURN(E_NOTIMPL);
};

//------------------------------------------------------------------------
//
// CWinBasicStreamBuffer::CWinBasicStreamBuffer
//
// ctor
//
//------------------------------------------------------------------------
CWinBasicStreamBuffer::CWinBasicStreamBuffer()
{
    m_cRef = 0;
    m_cursor = 0;
    m_size = 0;
    m_pData = NULL;
    m_pPalMemory = NULL;
    SetFastSeekSupport(FALSE);
}

//------------------------------------------------------------------------
//
// CWinBasicStreamBuffer::~CWinBasicStreamBuffer
//
// dtor
//
//------------------------------------------------------------------------
CWinBasicStreamBuffer::~CWinBasicStreamBuffer()
{
    if (m_pPalMemory)
    {
        ReleaseInterface(m_pPalMemory);
    }
    else
    {
        delete [] m_pData;
    }

    m_pData = NULL;
}

//------------------------------------------------------------------------
//
// CWinBasicStreamBuffer::Create
//
// Create a CWinBasicStreamBuffer
//
//------------------------------------------------------------------------
HRESULT
CWinBasicStreamBuffer::Create(_Outptr_result_maybenull_ CWinBasicStreamBuffer **ppWinBuffer)
{
    return Create((XUINT32) 0, ppWinBuffer);
}


//------------------------------------------------------------------------
//
// CWinBasicStreamBuffer::Create
//
// Create a CWinBasicStreamBuffer
//
//------------------------------------------------------------------------
#pragma warning(push)
#pragma warning(disable: 6387)  /* suppress the noise */
HRESULT
CWinBasicStreamBuffer::CreateFromIPalMemory(_In_ IPALMemory *pPalMemory, _Outptr_ CWinBasicStreamBuffer **ppWinBuffer)
{
    HRESULT hr = S_OK;
    CWinBasicStreamBuffer* pBuffer = NULL;

    IFCPTR(ppWinBuffer);
    *ppWinBuffer = NULL;

    pBuffer = new CWinBasicStreamBuffer();

    pBuffer->AddRef();

    pBuffer->m_pPalMemory = pPalMemory;

    pBuffer->m_pData = (XUINT8 *) pPalMemory->GetAddress();
    pBuffer->m_size = pPalMemory->GetSize();
    pBuffer->m_cursor = pBuffer->m_size;

    AddRefInterface(pPalMemory);

    *ppWinBuffer = pBuffer;
    pBuffer = NULL;

Cleanup:
    ReleaseInterface(pBuffer);
    RRETURN(hr);
}
#pragma warning(pop)

//------------------------------------------------------------------------
//
// CWinBasicStreamBuffer::Create
//
// Create a CWinBasicStreamBuffer with a set maximum size
//
//------------------------------------------------------------------------
HRESULT
CWinBasicStreamBuffer::Create(XUINT32 cbMaxBytes, _Outptr_result_maybenull_ CWinBasicStreamBuffer **ppWinBuffer)
{
    HRESULT hr = S_OK;
    CWinBasicStreamBuffer* pBuffer = NULL;

    IFCPTR(ppWinBuffer);
    *ppWinBuffer = NULL;

    pBuffer = new CWinBasicStreamBuffer();

    pBuffer->AddRef();

    pBuffer->m_pData = new XUINT8[cbMaxBytes];

    pBuffer->m_size = cbMaxBytes;

    *ppWinBuffer = pBuffer;
    pBuffer = NULL;

Cleanup:
    ReleaseInterface(pBuffer);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
// CWinBasicStreamBuffer::Create
//
// Create a CWinBasicStreamBuffer with a block of memory
//
//------------------------------------------------------------------------
HRESULT
CWinBasicStreamBuffer::Create(_In_ XUINT32 cbBuffer, _In_reads_opt_(cbBuffer) XUINT8* pBuffer, _Outptr_result_maybenull_ CWinBasicStreamBuffer **ppWinBuffer)
{
    HRESULT hr = S_OK;
    CWinBasicStreamBuffer* pDataBuffer = NULL;

    IFCPTR(ppWinBuffer);
    IFCPTR(pBuffer);
    *ppWinBuffer = NULL;

    pDataBuffer = new CWinBasicStreamBuffer();

    pDataBuffer->AddRef();

    pDataBuffer->m_pData = pBuffer;

    pDataBuffer->m_size = cbBuffer;
    pDataBuffer->m_cursor = cbBuffer;

    *ppWinBuffer = pDataBuffer;
    pDataBuffer = NULL;

Cleanup:
    ReleaseInterface(pDataBuffer);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
// CWinBasicStreamBuffer::AddRef
//
//
//
//------------------------------------------------------------------------
XUINT32 CWinBasicStreamBuffer::AddRef()
{
    return ::InterlockedIncrement((LONG*) &m_cRef);
}

//------------------------------------------------------------------------
//
// CWinBasicStreamBuffer::Release
//
//
//
//------------------------------------------------------------------------
XUINT32 CWinBasicStreamBuffer::Release()
{
    XUINT32 cRef = ::InterlockedDecrement((LONG*) &m_cRef);

    if (cRef == 0)
    {
        delete this;
    }

    return cRef;
}

//------------------------------------------------------------------------
//
// CWinBasicStreamBuffer::Append
//
// Add new data to the end of the data stream
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CWinBasicStreamBuffer::Write(_In_reads_bytes_(cb) void const *pData, XUINT32 cb, XUINT32 cbOffset, _In_ IStream *pStream)
{
    XUINT32 tempSize = 0;
    XUINT32 minTempSize = 0;
    std::unique_ptr<XUINT8[]> pTempBuffer;

    // cbOffset is ignored since this is actually an Append with http11 not
    // being used.

    IFCEXPECT_RETURN(pData || pStream);

    if (m_pPalMemory)
    {
        IFC_RETURN(E_NOTIMPL);
    }

    auto lock = m_Lock.lock();

    IFC_RETURN(UInt32Add(m_cursor, cb, &minTempSize));
    if (minTempSize > m_size)
    {
        tempSize = m_size * 2;
        if (tempSize < minTempSize)
            tempSize = minTempSize;

        pTempBuffer = std::make_unique<XUINT8[]>(tempSize);
        memcpy(pTempBuffer.get(), (XUINT8*)m_pData, m_size);

        delete[] m_pData;

        m_pData = pTempBuffer.release();
        m_size = tempSize;
        pTempBuffer = nullptr;
    }

    if (m_pData)
    {
        if (pStream)
        {
            IFC_RETURN(pStream->Read(m_pData + m_cursor, cb, (ULONG*)&cb));
        }
        else
        {
            _Analysis_assume_(m_cursor + cb <= m_size);
            memcpy(m_pData + m_cursor, (XUINT8*)pData, cb);
        }
        m_cursor += cb;
    }
    else
    {
        IFC_RETURN(E_FAIL);
    }
    return S_OK;
}

//------------------------------------------------------------------------
//
// CWinBasicStreamBuffer::Read
//
// Read a block of data from the data stream
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CWinBasicStreamBuffer::Read(_Out_writes_bytes_(cb) void* pv, XUINT32 cbOffset, XUINT32 cb, _Out_opt_ XUINT32* pcbRead)
{
    XUINT32 len;
    XUINT32 cbRead = 0;

    auto lock = m_Lock.lock();
    auto readUpdater = wil::scope_exit([&]
    {
        if (pcbRead)
        {
            *pcbRead = cbRead;
        }
    });

    if (m_pData == NULL)
    {
        IFC_RETURN(E_FAIL);
    }

    if (cb == 0)
    {
        cbRead = 0;
        return S_OK;
    }

    if (cbOffset + cb < cbOffset)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    len = m_cursor;
    // Don't read past the point of valid data
    if (cbOffset + cb > len)
    {
        if (len == m_size)
        {
            cbRead = len - cbOffset;
        }
        else
        {
            IFC_RETURN(E_PENDING);
        }
    }
    else
    {
        cbRead = cb;
    }

    if (cbRead == 0)
    {
        return S_OK;
    }
    IFCEXPECT_ASSERT_RETURN(cbRead <= cb);

    memcpy(pv, m_pData + cbOffset, cbRead);

    return S_OK;
}

//------------------------------------------------------------------------
//
// CWinBasicStreamBuffer::SetSize
//
// Set the length of the data
//
//------------------------------------------------------------------------
HRESULT CWinBasicStreamBuffer::SetSize (XUINT32 cb)
{
    XUINT8* pTemp = NULL;

    if (m_pPalMemory)
    {
        IFC_RETURN(E_NOTIMPL);
    }

    auto lock = m_Lock.lock();

    if (m_pData == NULL)
    {
        IFC_RETURN(E_FAIL)
    }

    if (cb != m_size)
    {
        pTemp = new XUINT8[cb];

        memcpy(pTemp, m_pData, std::min(cb, m_size));
        m_size = cb;

        delete[] m_pData;
        m_pData = pTemp;
    }
    return S_OK;
}

//------------------------------------------------------------------------
//
// CWinBasicStreamBuffer::Length
//
// Get the length of the data in the buffer
//
//------------------------------------------------------------------------
XUINT32 CWinBasicStreamBuffer::Length()
{
    XUINT32 len = 0;
    auto lock = m_Lock.lock();
    len = m_cursor;

    return len;
}

//------------------------------------------------------------------------
//
// CWinBasicStreamBuffer::Size
//
// Get the allocated length of the buffer
//
//------------------------------------------------------------------------
XUINT32 CWinBasicStreamBuffer::Size()
{
    XUINT32 size = 0;
    auto lock = m_Lock.lock();
    size = m_size;

    return size;
}

//------------------------------------------------------------------------
//
// CWinBasicStreamBuffer::GetOffset
//
// Get the offset of the current chunk
//
//------------------------------------------------------------------------
XUINT32 CWinBasicStreamBuffer::GetOffset()
{
    // No chunking, so offset is alway 0
    return 0;
}

//------------------------------------------------------------------------
//
// CWinBasicStreamBuffer::SetFinalDownloadSize
//
// Update final size when it has completed the downloading.
//
//------------------------------------------------------------------------
void CWinBasicStreamBuffer::SetFinalDownloadSize( _In_ XUINT32 cbFinalSize )
{
    auto lock = m_Lock.lock();

    //
    // We support this for WHS server, the final size should always be smaller
    // than initial total size, otherwise, this function is no-op.
    //
    if (cbFinalSize <= m_size)
    {
        m_size = cbFinalSize;
    }
    else
    {
        ASSERT(FALSE);
    }
}

//------------------------------------------------------------------------
//
// CWinDataStreamBuffer::Create
//
// Create a CWinDataStreamBuffer
//
//------------------------------------------------------------------------
HRESULT
CWinDataStreamBuffer::Create(_Outptr_result_maybenull_ CWinDataStreamBuffer **ppWinBuffer)
{
    RRETURN(CWinBasicStreamBuffer::Create(reinterpret_cast<CWinBasicStreamBuffer**>(ppWinBuffer)));
}


//------------------------------------------------------------------------
//
// CWinDataStreamBuffer::Create
//
// Create a CWinDataStreamBuffer
//
//------------------------------------------------------------------------
#pragma warning(push)
#pragma warning(disable: 6387)  /* suppress the noise */
HRESULT
CWinDataStreamBuffer::CreateFromIPalMemory(_In_ IPALMemory *pPalMemory, _Outptr_ CWinDataStreamBuffer **ppWinBuffer)
{
    RRETURN(CWinBasicStreamBuffer::CreateFromIPalMemory(pPalMemory, reinterpret_cast<CWinBasicStreamBuffer**>(ppWinBuffer)));
}
#pragma warning(pop)

//------------------------------------------------------------------------
//
// CWinDataStreamBuffer::Create
//
// Create a CWinDataStreamBuffer with a set maximum size
//
//------------------------------------------------------------------------
HRESULT
CWinDataStreamBuffer::Create(_In_ XBYTE fCreateMemoryStream, XUINT32 cbMaxBytes, _Outptr_result_maybenull_ CWinDataStreamBuffer **ppWinBuffer)
{
    HRESULT hr = S_OK;

    if(fCreateMemoryStream && (cbMaxBytes >= NONBUFFEREDSTREAM_MIN_SIZE))
    {
        TRACE(TraceAlways, L"##### Creating CWinMemoryStreamBuffer . #####");
        IFC(CWinMemoryStreamBuffer::Create(cbMaxBytes, reinterpret_cast<CWinMemoryStreamBuffer**>(ppWinBuffer)));

    }
    else
    if (cbMaxBytes >= MEDIASTREAM_MIN_SIZE)
    {
        TRACE(TraceAlways, L"##### Creating CWinHttp11StreamBuffer Size=%d bytes. #####", cbMaxBytes );
        IFC(CWinHttp11StreamBuffer::Create(cbMaxBytes, reinterpret_cast<CWinHttp11StreamBuffer**>(ppWinBuffer)));
    }
    else
    {
        TRACE(TraceAlways, L"##### Creating CWinBasicStreamBuffer Size=%d bytes. #####", cbMaxBytes );
        IFC(CWinBasicStreamBuffer::Create(cbMaxBytes, reinterpret_cast<CWinBasicStreamBuffer**>(ppWinBuffer)));
    }

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
// CWinDataStreamBuffer::Create
//
// Create a CWinDataStreamBuffer with a block of memory
//
//------------------------------------------------------------------------
HRESULT
CWinDataStreamBuffer::Create(_In_ XUINT32 cbBuffer, _In_reads_opt_(cbBuffer) XUINT8* pBuffer, _Outptr_result_maybenull_ CWinDataStreamBuffer **ppWinBuffer)
{
    RRETURN(CWinBasicStreamBuffer::Create(cbBuffer, pBuffer, reinterpret_cast<CWinBasicStreamBuffer**>(ppWinBuffer)));
}

_Check_return_ HRESULT
CWinDataStreamBuffer::Write(_In_reads_bytes_(cb) const void *pv, XUINT32 cb, XINT32 offset)
{
    return Write(pv, cb, offset, NULL);
}

_Check_return_ HRESULT
CWinDataStreamBuffer::CreateStream(_Out_ IPALStream** ppStream)
{
    HRESULT hr = S_OK;
    CWinDataStream* pStream = NULL;

    IFCPTR(ppStream);

    IFC(CWinDataStream::Create(this, &pStream));

    *ppStream = pStream;
    pStream = NULL;

Cleanup:
    ReleaseInterface(pStream);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
// CWinDataStream::CWinDataStream
//
// ctor
//
//------------------------------------------------------------------------
CWinDataStream::CWinDataStream()
{
    m_cRef = 1;
    m_cursor = 0;
    m_pBuffer = NULL;
    m_capabilities = 0;
}

//------------------------------------------------------------------------
//
// CWinDataStream::~CWinDataStream
//
// dtor
//
//------------------------------------------------------------------------
CWinDataStream::~CWinDataStream()
{
    if (m_pBuffer)
    {
        m_pBuffer->Release();
    }
}

//------------------------------------------------------------------------
//
// CWinDataStream::Create
//
// Exposed factory method for creation of a new data stream
//
//------------------------------------------------------------------------
HRESULT CWinDataStream::Create(
    _In_opt_ CWinDataStreamBuffer *pDataBuffer,
    _Outptr_result_maybenull_ CWinDataStream **ppWinDataStream)
{
    HRESULT hr = S_OK;
    CWinDataStream* pStream = NULL;

    IFCPTR(ppWinDataStream);
    *ppWinDataStream = NULL;
    IFCPTR(pDataBuffer);

    pStream = new CWinDataStream();

    pStream->m_pBuffer = pDataBuffer;
    pStream->m_pBuffer->AddRef();

    *ppWinDataStream = pStream;

    //
    // By default, the stream is readable, seekable and support unknwon size.
    // UNKNOWN_SIZE CAP is not used by Windows code right now, but putting it here is
    // to make code parity as in Mac platform. and this CAP can be checked later
    // in media code if necessary.
    //
    pStream->m_capabilities = PALSTREAM_CAPS_READABLE | PALSTREAM_CAPS_SEEKABLE | PALSTREAM_CAPS_SUPPORT_UNKNOWN_SIZE;

    //
    // Based on pDataBuffer's capability to determine if it is SLOW_SEEK only.
    //

    if (!pDataBuffer->SupportFastSeek( ))
    {
        pStream->m_capabilities |= PALSTREAM_CAPS_SLOW_SEEK;
    }

    pStream = NULL;

Cleanup:
    ReleaseInterface(pStream);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
// CWinDataStream::AddRef
//
//
//
//------------------------------------------------------------------------
XUINT32 CWinDataStream::AddRef()
{
    return ::InterlockedIncrement((LONG*) &m_cRef);
}

//------------------------------------------------------------------------
//
// CWinDataStream::Release
//
//
//
//------------------------------------------------------------------------
XUINT32 CWinDataStream::Release()
{
    XUINT32 cRef = ::InterlockedDecrement((LONG*) &m_cRef);

    if (cRef == 0)
    {
        delete this;
    }

    return cRef;
}

//------------------------------------------------------------------------
//
// CWinDataStream::Clone
//
//
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CWinDataStream::Clone (_Outptr_result_maybenull_ IPALStream **ppNewStream)
{
    HRESULT hr = S_OK;
    CWinDataStream* pStream = NULL;

    IFCPTR(ppNewStream);

    IFC(CWinDataStream::Create(0, &pStream));

    pStream->m_pBuffer = m_pBuffer;
    if (pStream->m_pBuffer)
    {
        pStream->m_pBuffer->AddRef();
    }

    pStream->m_cursor = 0;
    pStream->m_capabilities = m_capabilities;

    *ppNewStream = pStream;
    pStream = NULL;

Cleanup:
    ReleaseInterface(pStream);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
// CWinDataStream::Read
//
// Read a block of data from the data stream
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CWinDataStream::Read (_Out_writes_bytes_(cb) void *pv, XUINT32 cb, _Out_opt_ XUINT32 *pcbRead)
{
    HRESULT hr = S_OK;
    XUINT32 cbRead = 0;

    if (m_pBuffer == NULL)
    {
        IFC(E_FAIL);
    }

    if (m_cursor + cb < m_cursor)
    {
        IFC(E_INVALIDARG);
    }

    IFC(m_pBuffer->Read(pv, m_cursor, cb, &cbRead));
    m_cursor = m_cursor + cbRead;

Cleanup:
    if (pcbRead)
    {
        *pcbRead = cbRead;
    }

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
// CWinDataStream::Seek
//
// Move the stream's read cursor
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CWinDataStream::Seek (XINT64 qwMove, PALSeekOrigin eSeekOrigin, _Out_opt_ XUINT64 *pqwNewPosition)
{
    HRESULT hr = S_OK;

    if (m_pBuffer == NULL)
    {
        IFC(E_FAIL);
    }

    if (SeekOriginStart != eSeekOrigin)
    {
        IFC(E_NOTIMPL);
    }

    // We can't seek past the end of the stream
    if (qwMove > m_pBuffer->Size())
    {
        IFC(E_FAIL);
    }

    // Always seek to the requested position if within bounds
    m_cursor = static_cast<XINT32>(qwMove);

Cleanup:
    if (pqwNewPosition)
    {
        *pqwNewPosition = m_cursor;
    }

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
// CWinDataStream::SetSize
//
// Set the length of the data
//
//------------------------------------------------------------------------
HRESULT CWinDataStream::SetSize (XUINT64 qwNewSize)
{
    HRESULT hr = S_OK;

    if (m_pBuffer == NULL)
    {
        IFC(E_FAIL)
    }

    IFC(m_pBuffer->SetSize(static_cast<XUINT32>(qwNewSize)));
Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
// CWinDataStream::GetSize
//
// Get the size of the data stream.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CWinDataStream::GetSize (_Out_opt_ XUINT64 *pqwSize)
{
    HRESULT hr = S_OK;

    IFCPTR(pqwSize);

    if (m_pBuffer == NULL)
    {
        IFC(E_FAIL);
    }

    *pqwSize = m_pBuffer->Length();

Cleanup:

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
// CWinDataStream::GetOffset
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CWinDataStream::GetOffset(_Out_ XUINT64 *pqOffset)
{
    HRESULT hr = S_OK;

    IFCPTR(pqOffset);

    if (m_pBuffer == NULL)
    {
        IFC(E_FAIL);
    }

    *pqOffset = m_pBuffer->GetOffset();

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   GetPosition
//
//  Synopsis:
//          Get the position of the current read cursor
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CWinDataStream::GetPosition(_Out_ XUINT64 *pqPosition)
{
    HRESULT hr = S_OK;

    IFCPTR(pqPosition)
    *pqPosition = m_cursor;

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
// CWinDataStream::Write
//
// This function is not implemented for CWinDataStream.  Instead, new data
// should be appended to the associated CWinDataStreamBuffer.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CWinDataStream::Write(_In_reads_bytes_(cb) const void *pv, XUINT32 cb, XINT32 offset, _Out_opt_ XUINT32 *pcbWritten)
{
    HRESULT hr = S_OK;

    ASSERT(false, L"CWinDataStream does not implement Write.\n");
    IFC(E_NOTIMPL);

Cleanup:
    RRETURN(hr);
}

