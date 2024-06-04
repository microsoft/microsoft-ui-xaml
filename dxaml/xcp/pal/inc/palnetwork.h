// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//  Abstract:
//      Contains the basic Network support for the core platform
//      abstraction layer

#ifndef __PAL__NETWORK__
#define __PAL__NETWORK__

struct IPALResource;
struct IPALStream;

//--------------------------------------------------------
//
// PAL Networking Data Interfaces
//
//--------------------------------------------------------

#ifndef UNSECURE_DOWNLOADACTION
#define UNSECURE_DOWNLOADACTION

//------------------------------------------------------------------------
//
//  Enumeration: UnsecureDownloadAction
//
//  Synopsis:
//      list of Exceptions to download actions that would otherwise
//      be limited to site-of-origin.
//
//------------------------------------------------------------------------
#define udaNone                           0x00000000   // no exceptions to site-of-origin restrictions
#define udaAllowCrossDomain               0x00000001   // allow same-scheme (same protocol) Uri's that are on a different domains.
#define udaAllowRedirects                 0x00000002   // allow cross domain AND redirects
#define udaAllowMediaPermissions          0x00000004   // allow access to resources at localhost with same scheme (http/https)
#define udaAllowCrossScheme               0x00000008   // Allows cross scheme to http/s targets only. Cross scheme to non http/s targets is not permitted.
#define udaAllowOfflineIfFile             0x00000010   // Allow an offline:// uri if the base uri is using file://
#define udaAllowAll                       0x0000FFFF   // allow download from any location, including local machine
#define udaBlockFileAccess                0x00010000   // do not allow file scheme, no matter base scheme
#define udaShareDownloadStream            0x00040000   // have multiple requests for the same url share a stream (used for images)
#define udaEmbeddedResourcesOnly          0x00080000   // download restricted to files embedded in a resource
#define udaResynchronizeStream            0x04000000   // reloads HTTP resources if the resource has been modified since the last time it was downloaded. (search MSDN for BINDF_RESYCNHRONIZE/INTERNET_FLAG_RESYNCHRONIZE to get more information)

#endif // ifndef UNSECURE_DOWNLOADACTION


//------------------------------------------------------------------------
//
//  Interface:  IPALDownloadResponse
//
//  Synopsis:
//      Contains the results of a download request.
//
//        Currently supports getting the entire results of a download to a buffer.
//
//         Will likely support obtaining a stream for MF scenarios.
//
//------------------------------------------------------------------------

struct IPALDownloadResponse
{
protected :
    ~IPALDownloadResponse(){};

public :
    virtual XUINT32 AddRef() = 0;
    virtual XUINT32 Release() = 0;
// Buffer based access.
    virtual HRESULT _Check_return_ Lock( _Outptr_ IPALMemory** ppIMemory ) = 0 ;
    virtual HRESULT _Check_return_ Unlock() = 0 ;
    virtual HRESULT _Check_return_ GetStream( _Outptr_ IPALStream** ppStream) = 0;

    // Gets the resource associated with this download, if any.
    virtual HRESULT _Check_return_ GetResource(_Outptr_result_maybenull_ IPALResource** ppResource) = 0;
};

//------------------------------------------------------------------------
//
//  Interface:  IPALAbortableOperation
//
//  Synopsis:
//      Allows a download request to be aborted in mid progress
//
//------------------------------------------------------------------------
class IAsyncDownloadRequestManager;
struct IPALAbortableOperation : public IObject
{
protected:
    virtual ~IPALAbortableOperation(){};

public:

    virtual void Abort() = 0;
    virtual bool SupportsAbortAsync() const { return false; }
    virtual void AbortAsync(_In_ IAsyncDownloadRequestManager* manager) { ASSERT(FALSE); }
    virtual HRESULT _Check_return_ Disconnect() { ASSERT(FALSE); return E_NOTIMPL; }
};

//------------------------------------------------------------------------
//
//  Interface:  IPALDownloadResponseCallback
//
//  Synopsis:
//      Callback to obtain the results of a download.
//
//        Currently supports getting the entire results of a download to a buffer.
//
//         Will likely support progress notifications for MF scenarios
//
//------------------------------------------------------------------------

struct IPALDownloadResponseCallback
{
protected :
    ~IPALDownloadResponseCallback(){};

public :
    virtual uint32_t AddRef() = 0;
    virtual uint32_t Release() = 0;

    virtual HRESULT GotResponse(
        _In_ xref_ptr<IPALDownloadResponse> response,
        HRESULT hrStatus
        )=0;

    virtual HRESULT GotData(
        uint64_t size,
        uint64_t totalSize
        )=0;
};

struct IPALStreamingCallback
{
protected :
    ~IPALStreamingCallback() {};

public :
    virtual XUINT32 AddRef() = 0;
    virtual XUINT32 Release() = 0;
// Callback to tell us that the object cannot fulfill the current request
    virtual HRESULT Buffering( _In_ XUINT64 nDesiredStreamPosition ) = 0;

    // Supply service to IPALStreamEx so that it can ask for new BRR seek.
    virtual HRESULT OnByteRangeRequest( _In_ XUINT64 nByteRangeStart, _In_ XUINT64  nByteRangeStop ) = 0;
};

//------------------------------------------------------------------------
//
//  Interface:  IPALDownloadRequest
//
//  Synopsis:
//      Object representing a web request that manages its parameters
//      such as the headers, http method, and original request url
//
//------------------------------------------------------------------------
struct IPALDownloadRequest
{
protected:
    virtual ~IPALDownloadRequest() {};
public:
    virtual XUINT32 AddRef() = 0;
    virtual XUINT32 Release() = 0;

    virtual HRESULT GetUrl(_Out_ xstring_ptr* pstrUrl) = 0;
    virtual HRESULT GetAbsoluteUri(_Outptr_result_maybenull_ IPALUri** ppAbsoluteUri) = 0;
    virtual HRESULT GetCallback(_Outptr_ IPALDownloadResponseCallback** ppCallback) = 0;
    virtual HRESULT GetOptionFlags(_Out_ XUINT32* pDownloadOptions) = 0;
    virtual HRESULT GetUseDefaultCredentials(_Out_ bool* pbUseDefaultCredentials) = 0;
    virtual HRESULT GetUsername(_Out_ xstring_ptr* pstrUsername) = 0;
    virtual HRESULT GetPassword(_Out_ xstring_ptr* pstrPassword) = 0;
    virtual HRESULT GetAuthDomain(_Out_ xstring_ptr* pstrAuthDomain) = 0;
};

//------------------------------------------------------------------------
//
//  Interface:  IPALExecuteOnUIThread
//
//  Synopsis:
//      This interface is used with PostMessage in order for a background
//      thread to execute a function on the UI thread.
//
//------------------------------------------------------------------------
struct IPALExecuteOnUIThread
{
protected:
    ~IPALExecuteOnUIThread(){};
public:
    virtual XUINT32 AddRef() = 0;
    virtual XUINT32 Release() = 0;

    virtual _Check_return_ HRESULT Execute() = 0;
};

#endif //#ifndef __PAL__NETWORK__