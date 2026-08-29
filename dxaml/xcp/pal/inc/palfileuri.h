// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//  Abstract:
//      Contains the basic memory support for the core platform 
//      abstraction layer

#ifndef __PAL__FILE_URI__
#define __PAL__FILE_URI__

#include <minpal.h>

//--------------------------------------------------------
//
// PAL File/URI Data Interfaces
//
//--------------------------------------------------------

//  File Option Flags
//
//  Options that can be specified when opening or creating files
//
//------------------------------------------------------------------------

// do not use the reserved values - they are placeholders to help catch
// bugs from an API change - see bug 30408 for details
#define foptReserved1 0x00000000
#define foptReserved2 0x00000001
#define foptRead      0x00000002
#define foptWrite     0x00000004
#define foptExclusive 0x00000010

//
//  This should be used exclusively (do not combine them)
//
#define foptCreateAlways       0x00000020
#define foptCreateNew          0x00000040
#define foptOpenAlways         0x00000080
#define foptOpenExisting       0x00000100
#define foptTruncateExisting   0x00000200


// Time structures
struct XSYSTEMTIME
{
    XUINT16 wYear;
    XUINT16 wMonth;
    XUINT16 wDayOfWeek;
    XUINT16 wDay;
    XUINT16 wHour;
    XUINT16 wMinute;
    XUINT16 wSecond;
    XUINT16 wMilliseconds;
};


//------------------------------------------------------------------------
//
//  Enumeration:  PALSeekOrigin
//
//  Synopsis:
//      Enumeration specifying the origin of a seek call
//
//        Currently we only supports SeekOriginStart.
//
//------------------------------------------------------------------------
enum PALSeekOrigin
{
    SeekOriginStart = 0,
    SeekOriginCurrent = 1,
    SeekOriginEnd = 2
};

//------------------------------------------------------------------------
//
//  Interface:  IPALStream
//
//  Synopsis:
//      Interface to interact with a Stream object.
//
//        Currently we only support reading from a stream for Media.
//
//------------------------------------------------------------------------
struct IPALStream
{
protected:
    virtual ~IPALStream() {};

public:
    virtual XUINT32 AddRef() = 0;
    virtual XUINT32 Release() = 0;
    virtual HRESULT Clone(_Outptr_result_maybenull_ IPALStream **ppNewStream) = 0;
    virtual HRESULT Read(_Out_writes_bytes_to_(cb, *pcbRead) void *pv, _In_ XUINT32 cb, _Deref_out_range_(0,cb) XUINT32 *pcbRead) = 0;
    virtual HRESULT Seek (XINT64 qwMove, PALSeekOrigin eSeekOrigin, _Out_opt_ XUINT64 *pqwNewPosition) = 0;
    virtual HRESULT SetSize (XUINT64 qwNewSize) = 0;
    virtual HRESULT GetSize (_Out_ XUINT64 *pqwSize) = 0;
    virtual HRESULT GetOffset(_Out_ XUINT64 *pqOffset) = 0;
    virtual HRESULT GetPosition(_Out_ XUINT64 *pqPosition) = 0;
    virtual HRESULT Write(_In_reads_bytes_(cb) const void *pv, XUINT32 cb, XINT32 offset, _Out_opt_ XUINT32 *pcbWritten) = 0;
    virtual XUINT32 CanSeek() = 0;
    virtual XUINT32 GetCapabilities( ) = 0;
};

//------------------------------------------------------------------------
//
//  Interface:  IPALDataStreamBuffer
//
//  Synopsis:
//      Interface to interact with a data buffer that can back multiple
//      IPALStream objects
//
//----------------------------------------------------------------------
struct IPALDataStreamBuffer
{
protected:
    virtual ~IPALDataStreamBuffer() {};

public:
    virtual XUINT32 AddRef() = 0;
    virtual XUINT32 Release() = 0;

    virtual HRESULT Write(_In_reads_bytes_(cb) const void *pv, XUINT32 cb, XINT32 offset) = 0;
    virtual HRESULT CreateStream(_Out_ IPALStream** ppStream) = 0;
};

//
// Definition of different IPALStream capabilities:
// 
// Code can call GetCapabilities() on IPALStream to get the combination
// of these capabilities.
//
#define PALSTREAM_CAPS_READABLE       0x00000001  // The PALStream can be read.
#define PALSTREAM_CAPS_WRITABLE       0x00000002  // The PALStream is writeable.
#define PALSTREAM_CAPS_SEEKABLE       0x00000004  // The PALStream is seekable.
#define PALSTREAM_CAPS_SLOW_SEEK      0x00000008  // The PALStream supports slow seek only, usually it is 
                                                  // because the downloader for the stream doesn't support 
                                                  // Byte Range Request when stream is from remote server,
                                                  // so the downloading might take longer time when seeking 
                                                  // to a new location.

#define PALSTREAM_CAPS_SUPPORT_UNKNOWN_SIZE  0x00000010  
                                                  // File size is not accurate or unknown in the beginning,  
                                                  // when download finishes, final size is ready,  PALStream 
                                                  // can use this final size in consequent operations.


//------------------------------------------------------------------------
//
//  Interface:  IPALURIServices
//
//  Synopsis:
//      Provides an abstraction for system URI support.
//
//------------------------------------------------------------------------
struct IPALURIServices
{
   virtual _Check_return_ HRESULT UriCreate(
                                  _In_ XUINT32 cString,
                                  _In_reads_(cString) const WCHAR *pString,
                                  _Out_ IPALUri **ppUri) = 0;
};
                                                  
#endif //#ifndef __PAL__FILE_URI__
