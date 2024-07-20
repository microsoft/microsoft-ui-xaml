// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// A bare set of PAL interfaces needed to work with classes fundemental
// to the XAML framework. These will make good candidates for dePALing
// in future refactorings. We're keeping the interfaces to enable scoped
// feature work without having to rewrite the world.
//
// Soon they will be represented by a minimal PAL lib that can be easily
// linked from unit test DLLs.

#pragma once

// Unfortuantely needed by IPALUri::GetCanonical =(
#include <xstring_ptr.h>

// PAL waitable object.  This is the base interface for threads, events,
// and semaphores.
struct IPALWaitable
{
    virtual _Check_return_ HRESULT Close() = 0;
    virtual XHANDLE GetHandle() = 0;
};

struct IPALMemory
{
    virtual XUINT32 AddRef() const = 0;
    virtual XUINT32 Release() const = 0;

    // Direct access to the entire memory block
    virtual _Check_return_ void   *GetAddress() const = 0;
    virtual XUINT32 GetSize()    const = 0;
};

struct IPALFile : public IPALWaitable
{
    virtual XUINT32 Release() = 0;
    virtual _Check_return_ HRESULT Read(_In_ XUINT32 cBuffer, _Out_writes_bytes_(cBuffer) void *pBuffer, _Out_opt_ XUINT32 *pcRead) = 0;
    virtual _Check_return_ HRESULT Write(_In_ XUINT32 cBuffer, _In_reads_bytes_(cBuffer) const void *pBuffer, _Out_opt_ XUINT32 *pcWritten) = 0;
    virtual _Check_return_ HRESULT SetFilePointer(_In_ XINT64 cbOffset, _In_ XINT32 nMoveMethod) = 0;
    virtual _Check_return_ HRESULT SetEndOfFile() = 0;
    virtual _Check_return_ HRESULT GetSize(_Out_ XUINT64 *pSize) = 0;
    virtual _Check_return_ HRESULT MapRange(_In_ XUINT64 nOffset, _In_ XUINT32 cRange, _Outptr_ IPALMemory **ppMemory) = 0;
    virtual _Check_return_ HRESULT GetLastModifiedTime(_Out_ XINT32* pLastModified) = 0;
};

enum class ComponentResourceLocation
{
    Application = 0,
    Nested = 1
};

// Given this sample URI the methods below will return these strings
// ftp://username:password@hostname%41.redmond.corp.microsoft.com:8080/path1/path2.ext?query#fragment
//
// GetCanonical    - ftp://username:password@hostnameA.redmond.corp.microsoft.com:8080/path1/path2.ext?query
// GetExtension    - ext
// GetFileName     - path2.ext
// GetHost         - hostnameA.redmond.corp.microsoft.com
// GetPassword     - password
// GetPath         - /path1/path2.ext
// GetScheme       - ftp
// GetUsername     - username
// GetPortNumber   - 8080
//
//  Note that is is not possible to get the original string passed in to create
//  the object.  This string is considered harmful and as such no requirement is
//  made here to force it to be saved.
//
//  Each time these methods are called they allocate a string buffer. It is the
//  responsibility of the caller to release this string.  This can be done by
//  calling the platform services MemoryFree method.  If the URI doesn't have
//  the requested field the method will return S_OK and a NULL string pointer.
struct IPALUri
{
protected:
    virtual ~IPALUri(){}  // 'delete' not allowed, use 'Release' instead.
public:
    virtual XUINT32 AddRef() = 0;
    virtual XUINT32 Release() = 0;
    virtual _Check_return_ HRESULT Clone(_Out_ IPALUri **ppUri) const = 0;
    virtual _Check_return_ HRESULT Combine(
        _In_ XUINT32 cUri,
        _In_reads_(cUri) const WCHAR *pUri,
        _Out_ IPALUri **ppUriCombine
        ) = 0;

    virtual _Check_return_ HRESULT CreateBaseURI(_Out_ IPALUri **ppBaseUri) = 0;
    virtual _Check_return_ HRESULT GetCanonical(_Inout_ XUINT32 * pBufferLength,
        _Out_writes_to_opt_(*pBufferLength, *pBufferLength) WCHAR * pszBuffer) const = 0;
    virtual _Check_return_ HRESULT GetCanonical(_Out_ xstring_ptr* pstrCanonical) const = 0;
    virtual _Check_return_ HRESULT GetExtension(_Inout_ XUINT32 * pBufferLength,
        _Out_writes_(*pBufferLength) WCHAR * pszBuffer) = 0;
    virtual _Check_return_ HRESULT GetFileName(_Inout_ XUINT32 * pBufferLength,
        _Out_writes_(*pBufferLength) WCHAR * pszBuffer) = 0;
    virtual _Check_return_ HRESULT GetHost(_Inout_ XUINT32 * pBufferLength,
        _Out_writes_opt_(*pBufferLength) WCHAR * pszBuffer) const = 0;
    virtual _Check_return_ HRESULT GetPassword(_Inout_ XUINT32 * pBufferLength,
        _Out_writes_(*pBufferLength) WCHAR * pszBuffer) = 0;
    virtual _Check_return_ HRESULT GetPath(_Inout_ XUINT32 * pBufferLength,
        _Out_writes_opt_(*pBufferLength) WCHAR * pszBuffer) const = 0;
    virtual _Check_return_ HRESULT GetScheme(_Inout_ XUINT32 * pBufferLength,
        _Out_writes_opt_(*pBufferLength) WCHAR * pszBuffer) const = 0;
    virtual _Check_return_ HRESULT GetUsername(_Inout_ XUINT32 * pBufferLength,
        _Out_writes_opt_(*pBufferLength) WCHAR * pszBuffer) = 0;
    virtual _Check_return_ HRESULT GetQueryString(_Inout_ XUINT32 * pBufferLength,
        _Out_writes_opt_(*pBufferLength) WCHAR * pszBuffer) = 0;
    virtual _Check_return_ HRESULT GetPortNumber(_Out_ XUINT32 *pPortNumber) = 0;

    virtual _Check_return_ HRESULT GetFilePath(
        _Inout_ XUINT32 * pBufferLength,
        _Out_writes_opt_(*pBufferLength) WCHAR * pszBuffer) const = 0;

    virtual _Check_return_ HRESULT TransformToMsResourceUri(_Outptr_ IPALUri **ppUri) const = 0;

    virtual void SetComponentResourceLocation(_In_ ComponentResourceLocation resourceLocation) = 0;
    virtual ComponentResourceLocation GetComponentResourceLocation() const = 0;
};


// DEPRECATED: Referenced counted object interface
struct IObject
{
    virtual XUINT32 AddRef() = 0;
    virtual XUINT32 Release() = 0;
};

//------------------------------------------------------------------------
//
//  Method:   XcpDebugSetLeakDetectionFlag
//
//  Synopsis:
//      Mark LeakDetection flag in individual object that was allocated
//      through XcpDebugAllocate.
//
//------------------------------------------------------------------------
extern void XcpDebugSetLeakDetectionFlag(_In_ void *pAddress, _In_ bool fDisableLeakDetection);
