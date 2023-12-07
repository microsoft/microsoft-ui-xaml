// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Contains the types and methods provided by the host abstraction layer

#pragma once

class ThreadedJobQueue;

struct IDownloader
{
protected :
        ~IDownloader(){} ; //'delete' not allowed, use 'Release' instead.

public :
        virtual XUINT32 AddRef() = 0;
        virtual XUINT32 Release() = 0;

        // This member is used for cases where there are legitimate exceptions
        // to site-of-origin download restriction.  If you make a call to this
        // member, comment the call as to why the exception is valid and secure.
        virtual _Check_return_ HRESULT CreateUnsecureDownloadRequest(
                               _In_ IPALDownloadRequest* pRequest,
                               _Outptr_opt_ IPALAbortableOperation **ppIAbortableDownload,
                               _In_opt_ IPALUri *pPreferredBaseUri = NULL) = 0 ;


        virtual _Check_return_ HRESULT ContinueAllDownloads(_Out_ XINT32* pfContinue ) = 0 ;

        virtual void ResetDownloader() = 0;

        virtual _Check_return_ HRESULT  StopAllDownloads() = 0 ;

        
        virtual HRESULT _Check_return_ CheckUri(
                                _In_ const xstring_ptr& strRelativeUri,
                                _In_ XUINT32 eUnsecureDownloadAction,
                                _Out_opt_ XINT32 *pfShouldSuppressCookies = NULL,
                                _Out_opt_ XINT32 *pfRemoveSecurityLock = NULL) = 0;

        virtual HRESULT _Check_return_ CheckUri(
                                _In_ IPALUri *pUri,
                                _In_ XUINT32 eUnsecureDownloadAction,
                                _Out_opt_ XINT32 *pfShouldSuppressCookies = NULL,
                                _Out_opt_ XINT32 *pfRemoveSecurityLock = NULL) = 0;

        virtual _Check_return_ HRESULT CreateUnsecureDownloadRequest(
                                _In_ IPALUri* pAbsoluteUri,
                                _In_ IPALDownloadRequest* pRequestInfo,
                                _Outptr_opt_ IPALAbortableOperation ** ppIAbortableDownload,
                                bool bCheckForPolicyExpiration = true) = 0;

        virtual ThreadedJobQueue& GetJobQueue() = 0;
};

struct IDownloaderSite
{
public:
        // No Add/Ref release - as typically the implementor
        // owns an IDownloader.

        virtual HRESULT _Check_return_ CreateDownloadRequest(
                               _In_ IDownloader* pIDownloader,
                               _In_ IPALUri* pAbsoluteUri,
                               _In_ IPALDownloadRequest* pRequestInfo,
                                _In_ XINT32                                 bCrossDomain, 
                                _In_ XINT32                                 bRemoveSecurityLock,
                                _Outptr_opt_ IPALAbortableOperation**    ppIAbortableDownload) = 0;


        virtual HRESULT _Check_return_ GetDownloaderSiteBaseUri(_Out_ IPALUri** ppBaseUri) = 0;
        virtual HRESULT _Check_return_ GetDownloaderSiteSecurityUri(_Out_ IPALUri** ppSecurityUri) = 0;

        virtual bool IsNetworkingUnrestricted() = 0;

};

