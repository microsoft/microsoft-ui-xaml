// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.hpp"
#include <ThreadedJobQueue.h>
#include <wil\resource.h>
#include "corep.h"

//-------------------------------------------------------------------------
//
//  Function:   slowcmp
//
//  Synopsis:
//      Compares two strings ignoring case differences.  So for example the
//  strings "Match" and "matCH" will, in fact, match.  It only works if the
//  characters are in the range 0x0041 to 0x007a.
//
//      This function can also walk off the end of the shortest string if you
//  don't ensure cChar <= MIN(len(pTrg), len(pSrc))
//
//  Implementation details:
//      This function assumes the strings are 16 bit aligned.  This will fault
//  on certain architectures if that is not the case.
//
//-------------------------------------------------------------------------

XINT32
__cdecl
slowcmp(
    const _In_reads_(cChar) WCHAR *pTrg,
    const _In_reads_(cChar) WCHAR *pSrc,
    _In_ XUINT32 cChar
)
{
    while (cChar)
    {
        if (((*pTrg >= 0x0041) && (*pTrg <= 0x005a)) ||
            ((*pTrg >= 0x0061) && (*pTrg <= 0x007a)))
        {
            if ((*pTrg & 0x00df) != (*pSrc & 0x00df))
                break;
        }
        else
        {
            if (*pTrg != *pSrc)
                break;
        }
        pTrg++;
        pSrc++;
        cChar--;
    }

    return cChar ? XINT32(*pTrg - *pSrc) : 0;
}




//------------------------------------------------------------------------
//
//  Method:   Create
//
//  Synopsis:
//      Initializes object for use after creation.
//
//------------------------------------------------------------------------

_Check_return_
HRESULT
CDownloader::Create(   _Outptr_ CDownloader** ppDownloader,
                        _In_ IDownloaderSite * pDownloaderSite,
                        _In_ CCoreServices* pCore)
{
    HRESULT hr = S_OK ;
    CDownloader *pDownloader = NULL;

    IFCPTR(ppDownloader);
    IFCPTR(pDownloaderSite);
    IFCPTR(pCore);

    pDownloader = new CDownloader();

    pDownloader->m_pDownloaderSite = pDownloaderSite;

    //Since downloader will be kept by the host and host will keep the core
    // alive, we don't need a strong reference to core here.
    // in future if any scenario requires the Downloader independent of
    // host, we should revisit this
    pDownloader->m_pCore = pCore;
    pDownloader->m_mainThreadId = GetCurrentThreadId();

    *ppDownloader = pDownloader;
    pDownloader = NULL;
Cleanup :
    ReleaseInterface(pDownloader);
    RRETURN( hr ) ;
}

CDownloader::CDownloader()
{
    XCP_WEAK(&m_pCore);
}

CDownloader::~CDownloader()
{
    ASSERT(m_mainThreadId == GetCurrentThreadId());

    m_pDownloaderSite = NULL;
}

//------------------------------------------------------------------------
//
//  Method:   CreateUnsecureDownloadRequest
//
//  Synopsis:
//      Initiates a download request - asynchronously. Returns results via callback.
// This member is used for cases where there are legitimate exceptions
// to site-of-origin download restriction.  If you make a call to this
// member, comment the call as to why the exception is valid and secure.
//------------------------------------------------------------------------

_Check_return_ HRESULT
CDownloader::CreateUnsecureDownloadRequest(
                            _In_ IPALDownloadRequest* pRequest,
                            _Outptr_opt_ IPALAbortableOperation** ppIAbortableDownload,
                            _In_opt_ IPALUri* pPreferredBaseUri)
{
    HRESULT hr = S_OK ;
    xstring_ptr strCurrentUri;
    XINT32 bEmbeddedResourceOnly = FALSE;
    IPALMemory* pAppResourceMemory = NULL;
    IPALUri* pBaseUri = NULL;
    IPALUri* pDownloadUri = NULL;
    XUINT32 eUnsecureDownloadAction = udaNone;
    IPALDownloadResponseCallback* pCallback = NULL;
    IPALResourceManager *pResourceManager = NULL;
    bool isLocalResource = false;
    IPALResource* pResource = NULL;

    auto lock = m_Lock.lock();

    IFCPTR(pRequest);

    if (m_fStopDownloads)
    {
        IFC(E_FAIL);
    }

    IFC(pRequest->GetAbsoluteUri(&pDownloadUri));
    IFC(pRequest->GetOptionFlags(&eUnsecureDownloadAction));
    IFC(pRequest->GetCallback(&pCallback));

    if (eUnsecureDownloadAction & udaEmbeddedResourcesOnly)
    {
        bEmbeddedResourceOnly = TRUE;
    }

    IFC(m_pCore->GetResourceManager(&pResourceManager));

    if (pDownloadUri == NULL)
    {
        IFC(pRequest->GetUrl(&strCurrentUri));

        pBaseUri = pPreferredBaseUri;
        if (!pBaseUri)
        {
            // Get it from the control
            IFC(m_pDownloaderSite->GetDownloaderSiteBaseUri(&pBaseUri));
        }
    
        IFC(pResourceManager->CombineResourceUri(pBaseUri, strCurrentUri, &pDownloadUri));
    }

    IFC(pResourceManager->IsLocalResourceUri(pDownloadUri, &isLocalResource));
    
    // If the fragment is an absolute URI itself, the result might not be a local URI
    if (isLocalResource)
    {
        IFC(pResourceManager->TryGetLocalResource(pDownloadUri, &pResource));
        if (!pResource)
        {
            IFC(E_FAIL);
        }
        IFC(pResource->Load(&pAppResourceMemory));

        auto response = make_xref<CNULLDownloadResponse>(pResource, pAppResourceMemory);
        IGNOREHR(pCallback->GotResponse(std::move(response), hr));
        goto Cleanup;
    }

    if (bEmbeddedResourceOnly)
    {
        // bEmbeddedResourceOnly means we must load the resource out of the package. If we didn't satisfy
        // the download request with a local resource load, we must fail now.
        IFC(E_FAIL);
    }

    IFC(CreateUnsecureDownloadRequest(pDownloadUri, 
                                      pRequest,
                                      ppIAbortableDownload));

Cleanup:
    ReleaseInterface(pAppResourceMemory);
    ReleaseInterface(pDownloadUri);
    ReleaseInterface(pCallback);
    ReleaseInterface(pResourceManager);
    ReleaseInterface(pResource);

    RRETURN(hr);
}



//------------------------------------------------------------------------
//
//  Method:   ContinueAllDownloads
//
//  Synopsis: Ok to continue all downloads ?
//              S_OK if we should continue
//              S_FALSE if we should not continue.
//
// We will continue all downloads for now. Until we hook up the stop button to cancel everything.
//
//------------------------------------------------------------------------
HRESULT
_Check_return_ CDownloader::ContinueAllDownloads( _Out_ XINT32 *pfContinue)
{
    HRESULT hr = S_OK ;
    IFCPTR( pfContinue ) ;

    *pfContinue = ! m_fStopDownloads ;

Cleanup:
    RRETURN( hr ) ;
}

// Resets the state of the downloader primarily for test leak detection purposes.  Generally
// The destructor will clean everything up, but this provide a means of cleaning up global on-demand
// resources early.
void
CDownloader::ResetDownloader()
{
    m_threadedJobQueue.reset();
}

//------------------------------------------------------------------------
//
//  Method:   StopAllDownloads
//
//  Synopsis: Set flag indicating all downloads are to be terminated
//
//------------------------------------------------------------------------

HRESULT
CDownloader::StopAllDownloads()
{
    // TODO: We aren't handling this correctly as yet.
    // Need to change this to handle all pending downloads.
    // Needs to be done as part of work to wire up stop-button.
    m_fStopDownloads = TRUE;

    return S_OK ;
}

//------------------------------------------------------------------------
//
//  Method:   CheckUri
//
//  Synopsis: Check the validity of the passed relative Uri
//
//------------------------------------------------------------------------

_Check_return_
HRESULT
CDownloader::CheckUri(
    _In_ const xstring_ptr& strRelativeUri,
    _In_ XUINT32 eUnsecureDownloadAction,
    _Out_opt_ XINT32 *pfCrossDomain,
    _Out_opt_ XINT32 *pfRemoveSecurityLock)
{

    HRESULT hr = S_OK ;
    IPALUri * pAbsoluteUri = NULL;
    IPALUri* pBaseUri = NULL;

    auto lock = m_Lock.lock();

    //
    // GET BASE URL FROM CONTROL (pBaseUri)
    //

    IFC(m_pDownloaderSite->GetDownloaderSiteBaseUri(&pBaseUri));
    
    IFCPTR(pBaseUri);

    IFC(pBaseUri->Combine(strRelativeUri.GetCount(), strRelativeUri.GetBuffer(), &pAbsoluteUri));

    IFC( CheckUri(
            pAbsoluteUri,
            eUnsecureDownloadAction,
            pfCrossDomain,
            pfRemoveSecurityLock));

Cleanup:
    ReleaseInterface(pAbsoluteUri);
    RRETURN( hr ) ;
}

//------------------------------------------------------------------------
//
//  Method:   CheckUri
//
//  Synopsis:
//      Is pUriToCheck in a matching domain or sub-domain of our security Uri ?
//
//      RETURN S_OK if operation is allowed.
//      RETURN E_ACCESSDENIED if operation is not allowed.
//
//
//------------------------------------------------------------------------
_Check_return_
HRESULT
CDownloader::CheckUri(
                _In_ IPALUri * pUriToCheck,
                _In_ XUINT32  eUnsecureDownloadAction,
                _Out_opt_ XINT32 *pfCrossDomain,
                _Out_opt_ XINT32 *pfRemoveSecurityLock)
{
    HRESULT hr = E_ACCESSDENIED;

    WCHAR szTrustedHostName[XINTERNET_MAX_HOST_NAME_LENGTH + 1];
    WCHAR szTrustedScheme[XINTERNET_MAX_SCHEME_LENGTH + 1];
    WCHAR szCheckHostName[XINTERNET_MAX_HOST_NAME_LENGTH + 1];
    WCHAR szCheckScheme[XINTERNET_MAX_SCHEME_LENGTH + 1];
    XUINT32 cTrusted = 0;
    XUINT32 cCheck = 0;
    bool bIsNetworkingUnrestricted = false;

    XUINT32 nTrustedPort = 0;
    XUINT32 nCheckPort = 0;
    DownloadScheme eTrustedScheme = dsUNKNOWN;
    DownloadScheme eCheckScheme = dsUNKNOWN;

    IPALUri* pSecurityUri = NULL;

    XINT32 fDifferentDomains = FALSE;
    XINT32 fDifferentSchemes = FALSE;
    XINT32 fAllowLocalhostAccess = FALSE;

    auto lock = m_Lock.lock();

    if (pfCrossDomain)
    {
        *pfCrossDomain = TRUE;
    }
    if (pfRemoveSecurityLock)
    {
        *pfRemoveSecurityLock = TRUE;
    }

    IFCEXPECT(m_pDownloaderSite);

    //
    // GET SECURITY URI FROM CONTROL (pSecurityUri)
    //

    IFC(m_pDownloaderSite->GetDownloaderSiteSecurityUri(&pSecurityUri));
    ASSERT(pSecurityUri);

    cTrusted = ARRAY_SIZE(szTrustedScheme);
    IFC(pSecurityUri->GetScheme(&cTrusted, szTrustedScheme));

    if (cTrusted == 4)
    {
        if (slowcmp(szTrustedScheme, L"http", 4) == 0)
        {
            eTrustedScheme = dsHTTP;
        }
        else if (slowcmp(szTrustedScheme, L"file", 4) == 0)
        {
            eTrustedScheme = dsFILE;
        }
    }
    else if (cTrusted == 5 && slowcmp(szTrustedScheme, L"https", 5) == 0)
    {
        eTrustedScheme = dsHTTPS;
    }
    else if (cTrusted == 8 && slowcmp(szTrustedScheme, L"x-gadget", 8) == 0)
    {
        eTrustedScheme = dsXGADGET;
    }
    else if (cTrusted == 7 && slowcmp(szTrustedScheme, L"offline", 7) == 0)
    {
        eTrustedScheme = dsOFFLINE;
    }

    cCheck = ARRAY_SIZE(szCheckScheme);
    IFC(pUriToCheck->GetScheme(&cCheck, szCheckScheme));

    if (cCheck == 4)
    {
        if (slowcmp(szCheckScheme, L"http", 4) == 0)
        {
            eCheckScheme = dsHTTP;
        }
        else if (slowcmp(szCheckScheme, L"file", 4) == 0)
        {
            eCheckScheme = dsFILE;
        }
    }
    else if (cCheck == 5 && slowcmp(szCheckScheme, L"https", 5) == 0)
    {
        eCheckScheme = dsHTTPS;
    }
    else if (cCheck == 8 && slowcmp(szCheckScheme, L"x-gadget", 8) == 0)
    {
        eCheckScheme = dsXGADGET;
    }
    else if (cCheck == 7 && slowcmp(szCheckScheme, L"offline", 7) == 0)
    {
        eCheckScheme = dsOFFLINE;
    }

    // Check the schemes and set flag if they don't match
    if (eTrustedScheme != eCheckScheme)
    {
        fDifferentSchemes = TRUE;
    }

    // If schemes are different and the trusted scheme is https,
    // remove security lock.
    if (pfRemoveSecurityLock)
    {
        *pfRemoveSecurityLock = fDifferentSchemes && (eTrustedScheme == dsHTTPS);
    }

    cTrusted = ARRAY_SIZE(szTrustedHostName);
    IFC(pSecurityUri->GetHost(&cTrusted, szTrustedHostName));
    IFC(pSecurityUri->GetPortNumber(&nTrustedPort));

    cCheck = ARRAY_SIZE(szCheckHostName);
    IFC(pUriToCheck->GetHost(&cCheck, szCheckHostName));
    IFC(pUriToCheck->GetPortNumber(&nCheckPort));

    // Check for characters in the hostname that would require punycode
    if (ContainsCharactersToEncode(cCheck, szCheckHostName))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // Check if the domains are different.
    if ((nTrustedPort != nCheckPort) || (cTrusted != cCheck) || (slowcmp(szTrustedHostName, szCheckHostName, cTrusted) != 0))
    {
        fDifferentDomains = TRUE;
    }

    // Override to give gadgets access to do whatever they want.
    if (eTrustedScheme == dsXGADGET)
    {
        fDifferentSchemes = FALSE;
        fDifferentDomains = FALSE;
    }

    if (pfCrossDomain)
    {
        *pfCrossDomain = fDifferentDomains || fDifferentSchemes;
    }

    bIsNetworkingUnrestricted = m_pDownloaderSite->IsNetworkingUnrestricted();

    // If anything goes, then any Uri should be ok.
    if ((eUnsecureDownloadAction == udaAllowAll) || m_fAllowCrossDomain)
    {
        hr = S_OK;
        goto Cleanup;
    }

    // Only allow schemes on our approved list (http, file, https)
    if (eCheckScheme == dsUNKNOWN)
    {
        hr = E_ACCESSDENIED;
        goto Cleanup;
    }

    // Check if the request explicitly blocks file access
    if ((eUnsecureDownloadAction & udaBlockFileAccess) && eCheckScheme == dsFILE)
    {
        hr = E_ACCESSDENIED;
        goto Cleanup;
    }

    if (bIsNetworkingUnrestricted)
    {
        hr = S_OK;
        goto Cleanup;
    }

    // Do not allow access to a different scheme unless you have the cross-scheme flag
    // and the target is http or https.
    if( fDifferentSchemes && !((eUnsecureDownloadAction & udaAllowCrossScheme) && ((eCheckScheme == dsHTTP) || (eCheckScheme == dsHTTPS))))
    {
        if (!((eUnsecureDownloadAction & udaAllowOfflineIfFile) && eTrustedScheme == dsFILE && eCheckScheme == dsOFFLINE))
        {
            hr = E_ACCESSDENIED;
            goto Cleanup;
        }
    }

    // Check if we are allowed to access local host, and if the target url is localhost
    // and that we are http or https and still same scheme.
    if ((eUnsecureDownloadAction & udaAllowMediaPermissions) &&
           ((eCheckScheme == dsHTTP) || (eCheckScheme == dsHTTPS) || (eCheckScheme == dsXGADGET)) &&
           (!fDifferentSchemes) &&
           (cCheck == 9) &&
           ((slowcmp(szCheckHostName, L"localhost", 9) == 0) || (slowcmp(szCheckHostName, L"127.0.0.1", 9) == 0)))
    {
        fAllowLocalhostAccess = TRUE;
    }
    else if ( fDifferentDomains && !(eUnsecureDownloadAction & udaAllowCrossDomain))
    {
        // To get into this branch we've already verified the local host exemption doesn't apply.
        // We've also already denied any requests not allowed cross scheme.

        hr = E_ACCESSDENIED;
        goto Cleanup;
    }

    hr = S_OK;

 Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   CreateUnsecureDownloadRequest
//
//  Synopsis: Create an unsecure download request from an IPALUri rather
//     than a string.  Needed for the x-domain download requests.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CDownloader::CreateUnsecureDownloadRequest(
                                _In_ IPALUri* pAbsoluteUri,
                                _In_ IPALDownloadRequest* pRequestInfo,
                                _Outptr_opt_ IPALAbortableOperation ** ppIAbortableDownload,
                                bool bCheckForPolicyExpiration)
{
    HRESULT hr = S_OK;
    XINT32 fCrossDomain = TRUE;
    XINT32 fRemoveSecurityLock = TRUE;

    IPALUri* pBaseUri = NULL;
    IPALUri* pSecurityUri = NULL;

    bool bIsNetworkingUnrestricted = false;

    bIsNetworkingUnrestricted = m_pDownloaderSite->IsNetworkingUnrestricted();

    CDownloadRequestInfo* pCheckedRequestInfo = NULL;

    XUINT32 eUnsecureDownloadAction = udaNone;
    xstring_ptr strCanonicalUrl;
    IPALDownloadResponseCallback* pCallback = NULL;
    bool bUseDefaultCreds = false;
    xstring_ptr strAuthUsername;
    xstring_ptr strAuthDomain;
    xstring_ptr strAuthPassword;
    bool bUsingCustomAuth = false;

    auto lock = m_Lock.lock();

    if (m_fStopDownloads)
    {
        IFC(E_FAIL);
    }

    IFCEXPECT(m_pDownloaderSite);

    IFC(pRequestInfo->GetOptionFlags(&eUnsecureDownloadAction));
    IFC(pRequestInfo->GetCallback(&pCallback));
    IFC(pRequestInfo->GetUseDefaultCredentials(&bUseDefaultCreds));
    if (!bUseDefaultCreds)
    {
        IFC(pRequestInfo->GetUsername(&strAuthUsername));
        IFC(pRequestInfo->GetPassword(&strAuthPassword));
        IFC(pRequestInfo->GetAuthDomain(&strAuthDomain));
        if (!strAuthUsername.IsNullOrEmpty())
        {
            bUsingCustomAuth = TRUE;
        }
    }

    IFC( CheckUri( pAbsoluteUri, eUnsecureDownloadAction, &fCrossDomain, &fRemoveSecurityLock));

    if (bIsNetworkingUnrestricted)
    {
        // For elevated OOB apps, we do not want to remove the security lock icon.
        // (We don't display browser chrome for OOB apps, so there's no lock icon 
        // to remove. On Windows this would also result in an unwanted prompt in 
        // some configurations.)
        fRemoveSecurityLock = FALSE;
    }

    IFC(m_pDownloaderSite->GetDownloaderSiteBaseUri(&pBaseUri));
    IFC(m_pDownloaderSite->GetDownloaderSiteSecurityUri(&pSecurityUri));

    IFC(pAbsoluteUri->GetCanonical(&strCanonicalUrl));

    IFC(CDownloadRequestInfo::Create(strCanonicalUrl, pCallback, &pCheckedRequestInfo));
    IFC(pCheckedRequestInfo->SetOptionFlags(eUnsecureDownloadAction));
    if (!bUseDefaultCreds)
    {
        IFC(pCheckedRequestInfo->SetCredentials(strAuthUsername, strAuthPassword, strAuthDomain));
    }
    
    // Initiate the download.
    IFC( m_pDownloaderSite->CreateDownloadRequest( (IDownloader*) this,
                                                      pAbsoluteUri,
                                                      pCheckedRequestInfo,
                                                      fCrossDomain,
                                                      fRemoveSecurityLock,
                                                      ppIAbortableDownload));

Cleanup:

    ReleaseInterface(pCallback);
    ReleaseInterface(pCheckedRequestInfo);

    RRETURN(hr);
}

ThreadedJobQueue& CDownloader::GetJobQueue()
{
    if (m_threadedJobQueue == nullptr)
    {
        // This queue is meant for urlmon so it requires COM SingleThreadedApartment usage.
        m_threadedJobQueue = wil::make_unique_failfast<ThreadedJobQueue>(
            ThreadedJobQueue::ThreadingModel::ComSingleThreadedApartment,
            true /* terminateOnInactivity */);
    }

    return *m_threadedJobQueue;
}