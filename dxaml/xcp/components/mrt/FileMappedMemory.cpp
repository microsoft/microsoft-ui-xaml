// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "FileMappedMemory.h"

FileMappedMemory::FileMappedMemory()
    : m_cRef(1)
    , m_hFile(0)
    , m_hMapped(0)
    , m_pData(nullptr)
    , m_cSize(0)
{
}

FileMappedMemory::~FileMappedMemory()
{
    if (m_pData)
    {
        UnmapViewOfFile(m_pData);
    }
    if (m_hMapped)
    {
        CloseHandle(m_hMapped);
    }
    if (m_hFile)
    {
        CloseHandle(m_hFile);
    }
}

// Creates a FileMappedMemory object with a memory mapped file buffer.
_Check_return_ HRESULT FileMappedMemory::Create(_In_ const xstring_ptr& strFilePath, _Outptr_ IPALMemory** ppMemory)
{
    HRESULT hr = S_OK;
    xref_ptr<FileMappedMemory> pMemory;
    unsigned char* pMapped = nullptr;
    HANDLE hMapped = 0;
    UINT64 fileSize = 0;
    unsigned int cMapped = 0;
    HANDLE hFile = nullptr;

    IFCPTR(ppMemory);

    // Open file for read. Allow shared delete as it will still be mapped until
    // the last handle is closed.
    hFile = CreateFile(
        strFilePath.GetBuffer(),
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);

    if (hFile == nullptr)
    {
        IFC(E_FAIL);
    }

    if (hFile == INVALID_HANDLE_VALUE)
    {
        DWORD errorCode = GetLastError();
        if (errorCode == ERROR_ACCESS_DENIED)
        {
            DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_strXbfExtension, L"xbf");

            if (strFilePath.EndsWith(c_strXbfExtension, xstrCompareCaseInsensitive))
            {
                // fail fast if this is a file lock issue
                IFCFAILFAST(HRESULT_FROM_WIN32(errorCode));
            }
        }

        IFC(HRESULT_FROM_WIN32(errorCode));
    }

    if (!GetFileSizeEx(hFile, (LARGE_INTEGER*)&fileSize))
    {
        IFC(E_FAIL);
    }

    if (fileSize > 0xffffffff)
    {
        IFC(E_OUTOFMEMORY);
    }

    // Attempt to create the specified mapping
    hMapped = CreateFileMapping(hFile, nullptr, PAGE_READONLY, 0, 0, nullptr);
    IFCOOM(hMapped);

    cMapped = DWORD(fileSize & 0xffffffff);
    pMapped = static_cast<unsigned char*>(MapViewOfFile(hMapped, FILE_MAP_READ, 0, 0, cMapped));
    IFCOOM(pMapped);

    // Now create a mapped memory object to allow the caller to access the memory
    pMemory = make_xref<FileMappedMemory>();

    pMemory->m_hFile = hFile;
    pMemory->m_hMapped = hMapped;
    pMemory->m_pData = pMapped;
    pMemory->m_cSize = cMapped;

    hFile = 0;
    hMapped = 0;
    pMapped = nullptr;

    *ppMemory = pMemory.detach();

Cleanup:
    if (pMapped != nullptr)
    {
        UnmapViewOfFile(pMapped);
    }
    if (hMapped != nullptr)
    {
        CloseHandle(hMapped);
    }
    if (hFile != nullptr)
    {
        CloseHandle(hFile);
    }

    RRETURN(hr);
}

// IPALMemory::AddRef
unsigned int FileMappedMemory::AddRef() const
{
    unsigned int cRef = (unsigned int)::InterlockedIncrement((LONG*)&m_cRef);

    ASSERT(cRef);

    if (!cRef)
    {
        // we have hit an overflow...exit the process
        XAML_FAIL_FAST();
    }

    return cRef;
}

// IPALMemory::Release
unsigned int FileMappedMemory::Release() const
{
    unsigned int cRef = (unsigned int)::InterlockedDecrement((LONG*)&m_cRef);

    if (0 == cRef)
    {
        delete this;
    }

    return cRef;
}