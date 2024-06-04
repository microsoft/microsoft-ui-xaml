// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

//------------------------------------------------------------------------
//
//  Method:   ctor
//
//  Synopsis:
//      Constructor for CWinFile object
//
//------------------------------------------------------------------------

CWinFile::CWinFile()
{
    m_cRef = 1;
    m_hSystem = HANDLE(0);
    m_cSize = UINT64(0);
}

//------------------------------------------------------------------------
//
//  Method:   dtor
//
//  Synopsis:
//      Destructor for PALFile object
//
//------------------------------------------------------------------------

CWinFile::~CWinFile()
{
    IGNOREHR(Close());
}

//------------------------------------------------------------------------
//
//  Method:   Release
//
//  Synopsis:
//      Lowers the reference count.  Will delete the object if the reference
//  count if it releases zero.
//
//------------------------------------------------------------------------

XUINT32
CWinFile::Release(
)
{
    XUINT32 cRef = --m_cRef;

    if (!cRef)
        delete this;

    return cRef;
}

//------------------------------------------------------------------------
//
//  Method:   Close
//
//  Synopsis:
//      IPALWaitable Close implementation so that CloseHandle works
//      on IPALFiles.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT 
CWinFile::Close()
{
    if (m_hSystem)
        CloseHandle(m_hSystem);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   GetHandle
//
//  Synopsis:
//      No real handle for the file so we don't want to Wait on it.
//
//------------------------------------------------------------------------

XHANDLE 
CWinFile::GetHandle()
{
    return static_cast<XHANDLE>(m_hSystem);
}

//------------------------------------------------------------------------
//
//  Method:   Read
//
//  Synopsis:
//      Copies bytes from current file position to the specified buffer.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
CWinFile::Read(
    _In_ XUINT32 cBuffer,
    _Out_writes_bytes_(cBuffer) void *pBuffer,
    _Out_opt_ XUINT32 *pcRead
)
{
    XUINT32 cRead;

    if (pcRead == NULL)
    {
        pcRead = &cRead;
    }

    if (!ReadFile(m_hSystem, pBuffer, (DWORD) cBuffer, (LPDWORD)pcRead, NULL) || ((*pcRead) != (DWORD) cBuffer))
        return E_FAIL;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   Write
//
//  Synopsis:
//      Writes files to the handle
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
CWinFile::Write(
    _In_ XUINT32 cBuffer,
    _In_reads_bytes_(cBuffer) const void *pBuffer,
    _Out_opt_ XUINT32 *pcWritten
)
{
    XUINT32 cWritten;

    if (pcWritten == NULL)
    {
        pcWritten = &cWritten;
    }

    if (!WriteFile(m_hSystem, pBuffer, (DWORD) cBuffer, (LPDWORD)pcWritten, NULL) || ((*pcWritten) != (DWORD) cBuffer))
        return E_FAIL;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   SetEndOfFile
//
//  Synopsis:
//      Seek to the end of the file
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CWinFile::SetEndOfFile()
{
    HRESULT hr = S_OK;

    if (!::SetEndOfFile(m_hSystem))
    {
        hr = E_FAIL;
    }

    return hr;
}


//------------------------------------------------------------------------
//
//  Method:   MapRange
//
//  Synopsis:
//      Attempts to map a range of bytes into memory and returns a pointer to
//  the begining of the mapped range.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
CWinFile::MapRange(
    _In_ XUINT64 nOffset,
    _In_ XUINT32 cRange,
    _Outptr_ IPALMemory **ppMemory
)
{
    HRESULT hr = S_OK;
    CMappedMemory *pMemory = NULL;
    HANDLE  hMapped = NULL;
    void   *pMapped = NULL;

// Attempt to create the specified mapping

    hMapped = CreateFileMapping(m_hSystem, NULL, PAGE_READONLY, 0, 0, NULL);
    IFCOOM_RETURN(hMapped);

    pMapped = MapViewOfFile(hMapped, FILE_MAP_READ, DWORD(nOffset >> 32), DWORD(nOffset), DWORD(cRange));
    IFCOOM_RETURN(pMapped);

// Now create a mapped memory object to allow the caller to access the memory

    pMemory = new CMappedMemory(static_cast<IPALFile *>(this), hMapped, pMapped, cRange);

// We now have another reference on the file object.

    m_cRef++;

// The mapped memory object now owns the pointer and handle for the mapping.

    pMapped = NULL;
    hMapped = 0;

// Return the object and success to the caller.

   *ppMemory = static_cast<IPALMemory *>(pMemory);

    if (pMapped != NULL)
    {
        UnmapViewOfFile(pMapped);
    }
    if (hMapped != NULL)
    {
        CloseHandle(hMapped);
    }

    RRETURN(hr);//RRETURN_REMOVAL
}

//------------------------------------------------------------------------
//
//  Method:   GetLastModifiedTime
//
//  Synopsis:
//      Get the last time the file was modified (as UNIX time)
//
//------------------------------------------------------------------------
_Check_return_ HRESULT 
CWinFile::GetLastModifiedTime(_Out_ XINT32* pLastModified)
{
    HRESULT hr = S_OK;
    FILETIME filetimeLastModified = {0};
    XINT64 lastModified64 = 0;
        
    ULARGE_INTEGER uli1970 = {0};
    ULARGE_INTEGER uliLastModified = {0};
    SYSTEMTIME systime1970 = {0};
    FILETIME filetime1970 = {0};

    IFCPTR(pLastModified);

    IFCW32(GetFileTime(m_hSystem, NULL, NULL, &filetimeLastModified));

    // Convert filetime to unixtime
    // Unix time is based off of Thu, 1 Jan 1970 00:00:00
    systime1970.wYear = 1970;
    systime1970.wMonth = 1;
    systime1970.wDay = 1;
    systime1970.wDayOfWeek = 4;

    IFCW32(SystemTimeToFileTime(&systime1970, &filetime1970));

    uli1970.LowPart = filetime1970.dwLowDateTime;
    uli1970.HighPart = filetime1970.dwHighDateTime;

    uliLastModified.LowPart = filetimeLastModified.dwLowDateTime;
    uliLastModified.HighPart = filetimeLastModified.dwHighDateTime;

    lastModified64 = (uliLastModified.QuadPart - uli1970.QuadPart) / 10000000LL;

    ASSERT(lastModified64 < XINT32_MAX);
    if (lastModified64 >= XINT32_MAX)
    {
        // Value is too big to fit into 32-bit unix time
        IFC(E_UNEXPECTED);
    }

    *pLastModified = static_cast<XINT32>(lastModified64);

Cleanup:
    RRETURN(hr);
}
//------------------------------------------------------------------------
//
//  Method:   GetFileHandle
//
//  Synopsis:
//     Get the File Handle
//
//------------------------------------------------------------------------

XHANDLE 
CWinFile::GetFileHandle()
{
    return static_cast<XHANDLE>(m_hSystem);
}

//------------------------------------------------------------------------
//
//  Method:   SetFilePointer
//
//  Synopsis:
//      Seek to a particular location in the file
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CWinFile::SetFilePointer(_In_ XINT64 cbOffset, _In_ XINT32 nMoveMethod)
{
    HRESULT hr = S_OK;
    LARGE_INTEGER llOffset;

    llOffset.QuadPart = cbOffset;

    if (!SetFilePointerEx(m_hSystem, llOffset, NULL, static_cast<DWORD>(nMoveMethod)))
    {
        hr = E_FAIL;
    }

    return hr;
}

//------------------------------------------------------------------------
//
//  Method:   GetSize
//
//  Synopsis:
//      Returns the size in bytes of the file.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
CWinFile::GetSize(
    _Out_ XUINT64 *pcSize
)
{
    if (!m_cSize)
    {
        if (!GetFileSizeEx(m_hSystem, (LARGE_INTEGER *) &m_cSize))
            return E_FAIL;
    }

   *pcSize = m_cSize;

    return S_OK;
}
