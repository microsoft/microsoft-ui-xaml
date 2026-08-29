// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//------------------------------------------------------------------------
//
//  Abstract:
//
//      Contains the interface to the media services object.
//
//------------------------------------------------------------------------

#ifndef CORE_MEDIA_H
#define CORE_MEDIA_H

struct IPALDownloadResponseCallback;
struct IPALAbortableOperation;
struct IXcpBrowserHost;
struct IXcpHostSite;

class CWindowRenderTarget;

struct IMediaQueueItem
{
    virtual IMediaQueueItem* GetNext() = 0;
    virtual IMediaQueueItem* GetPrevious() = 0;
    virtual IObject* GetState() = 0;
};

struct IMediaQueueTest
{
    virtual bool Test(IMediaQueueItem*& pHead, IMediaQueueItem*& pTail) = 0;
};

struct IMediaQueue : IObject
{
    virtual HRESULT AddMediaEvent(_In_ IObject* pState, _In_opt_ IMediaQueueTest* pTest) = 0;
    virtual HRESULT ProcessQueue() = 0;
    virtual HRESULT Shutdown() = 0;
};

struct IMediaQueueClient
{
    virtual HRESULT HandleMediaEvent(_In_opt_ IObject *pState, _In_ XINT32 bIsShuttingDown) = 0;
};

//------------------------------------------------------------------------
//
//  Interface:  IMediaServices
//
//  Synopsis:
//      Interface to an instance of a media services object.
//
//------------------------------------------------------------------------

struct IMediaServices
{
    // Lifetime Management
    virtual XUINT32 Release() = 0;
    virtual XUINT32 AddRef() = 0;

    // Downloading
    virtual HRESULT _Check_return_ CheckUri(
        _In_ const xstring_ptr& strRelativeUri,
        _In_ XUINT32 eUnsecureDownloadAction) = 0;

    // Browser Access
    virtual _Ret_notnull_ IXcpBrowserHost *GetBrowserHost() = 0;
    virtual IXcpHostSite *GetHostSite() = 0;

    // Get the current thread id of the core - the UI thread
    virtual DWORD GetThreadID() = 0;

    virtual _Check_return_ HRESULT CreateMediaQueue(_In_ IMediaQueueClient *pClient, _Outptr_ IMediaQueue **ppQueue) = 0;

    // Returns the window render target attached to this core services object.
    virtual _Ret_maybenull_ CWindowRenderTarget *NWGetWindowRenderTarget() = 0;
};

#endif

