// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <ImageDecodeBoundsFinder.h>

//------------------------------------------------------------------------
//
//  Synopsis:
//      dtor
//
//------------------------------------------------------------------------
CImageBrush::~CImageBrush()
{
    // If the image brush has been created for DragVisual, we can tell the
    // bitmap image that keeping the software surface is not needed
    if (m_isDragDrop && m_pImageSource)
    {
        m_pImageSource->SetKeepSoftwareSurfaceOnReload(false);
    }

    // If we're holding a lock on the image surface unlock it now.  Currently
    // this can only be true for images from media.  However, the brush cache
    // could use this mechanism as well.

    if (m_pImageSource && m_pImageSource->GetSoftwareSurface() != NULL)
    {
        while ( m_cLockCount-- )
        {
            IGNOREHR(m_pImageSource->GetSoftwareSurface()->Unlock());
        }
    }

    // Both ImageBrush and BitmapImage hold onto a reference to this callback, so
    // just releasing our reference is not enough, we have to tell our BitmapImage
    // source to remove it from its list of callbacks.
    RemoveCallback();

    if (m_pEventList)
    {
        m_pEventList->Clean();
        delete m_pEventList;
        m_pEventList = NULL;
    }

    ReleaseInterface(m_pImageSource);
    ReleaseInterface(m_pImageReportCallback);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Sets the value of the bitmap image's source
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CImageBrush::SetValue(_In_ const SetValueParams& args)
{
    HRESULT hr = S_OK;
    CImageSource *pOriginalImageSource = NULL;

    if (args.m_pDP->GetIndex() == KnownPropertyIndex::ImageBrush_ImageSource)
    {
        // Hold a reference to original image source until reload,
        // so that its hw surface cache entry is retained in case
        // its used with the new source.
        // TODO: Merge the HW surface cache into ImageProvider/ImageCache. Then this isn't needed.
        SetInterface(pOriginalImageSource, m_pImageSource);

        RemoveCallback();
    }

    IFC(CDependencyObject::SetValue(args));

    // If we're currently live in the tree, and the source changed, reload image.
    // If we're not currently live, EnterImpl() will trigger the download.
    if (args.m_pDP->GetIndex() == KnownPropertyIndex::ImageBrush_ImageSource)
    {
        if (HasManagedPeer()
            && args.m_value.GetType() == valueObject)
        {
            IFC(SetPeerReferenceToProperty(args.m_pDP, args.m_value));
        }

        UpdateDecodeToRenderSizeStatusOnSourceChange( pOriginalImageSource );

        if (m_isDragDrop || IsActive())
        {
            if (m_isDragDrop)
            {
                m_decodeToRenderSizeForcedOff = true;
                m_pImageSource->SetDecodeToRenderSizeForcedOff();
                m_pImageSource->TraceDecodeToRenderSizeDisqualified(ImageDecodeBoundsFinder::FallbackReason::DragAndDrop);
                m_pImageSource->SetKeepSoftwareSurfaceOnReload(true);
            }
            // ImageSource has changed in this ImageBrush but note that image source's Uri has not changed.
            IFC(ReloadImage(false /*uriChanged*/, nullptr));
        }
    }

Cleanup:
    ReleaseInterface(pOriginalImageSource);
    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis: Remove the bitmap image callback from our
//            source's callback list. Both ImageBrush and
//            BitmapImage hold onto a reference to this
//            callback, so just releasing our reference
//            is not enough, we have to tell our BitmapImage
//            source to remove it from its list of callbacks.
//
//------------------------------------------------------------------------
void CImageBrush::RemoveCallback()
{
    if(m_pBitmapImageReportCallback && m_pImageSource)
    {
        m_pImageSource->RemoveCallback(m_pBitmapImageReportCallback);
        ReleaseInterface(m_pBitmapImageReportCallback);
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Calls the BitmapCache manager Synchronously to get the decoded surface.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CImageBrush::ReloadImage(
    bool uriChanged,
    _In_opt_ IImageReportCallback *pImageErrorCallback
    )
{
    HRESULT hr = S_OK;

    // Both ImageBrush and BitmapImage hold onto a reference to this callback, so
    // just releasing our reference is not enough, we have to tell our BitmapImage
    // source to remove it from its list of callbacks.
    RemoveCallback();

    ReplaceInterface(m_pImageReportCallback, pImageErrorCallback);

    if (m_pImageSource)
    {
        IFC(CBitmapImageReportCallback::Create(this, &m_pBitmapImageReportCallback));

        IFC(m_pImageSource->AddCallback(m_pBitmapImageReportCallback));

        if (!m_pImageSource->m_strSource.IsNull())
        {
            IFC(m_pImageSource->ReloadImage(uriChanged, false /*retainPlaybackState*/));
        }
    }

Cleanup:
    if (   FAILED(hr)
        || !m_pImageSource
        || m_pImageSource->m_strSource.IsNullOrEmpty())
    {
        UnLock();

        //
        // Image source - Dirties: (Render)
        //
        CDependencyObject::NWSetRenderDirty(this, DirtyFlags::Render);

    }
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Query the ImageSource to determine if we should make an image decode request.
//
//------------------------------------------------------------------------
bool
CImageBrush::ShouldRequestDecode()
{
    bool result = false;

    if (m_pImageSource)
    {
        result = m_pImageSource->ShouldRequestDecode();
    }

    return result;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Calls the image source to enqueue a request to decode an image
//      at a requested size.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CImageBrush::QueueRequestDecode(
    XUINT32 width,
    XUINT32 height,
    bool retainPlaybackState
    )
{
    if (m_pImageSource)
    {
        IFC_RETURN(m_pImageSource->QueueRequestDecode(width, height, retainPlaybackState));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Fire events by Name
//
//------------------------------------------------------------------------
void
CImageBrush::FireImageDownloadEvent(_In_ XFLOAT downloadProgress)
{
    if (m_pImageReportCallback)
    {
        IGNOREHR(m_pImageReportCallback->FireDownloadProgress(downloadProgress));
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      If the image was used only in hardware, then we released the
//      software surface. We need the software surface again, so decode
//      it again.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CImageBrush::ReloadSoftwareSurfaceIfReleased()
{
    // Lock the inner surface ...
    if (m_pImageSource != NULL && m_pImageSource->GetSoftwareSurface() == NULL)
    {
        //
        // There's no software surface, which means the surface has been copied to video
        // memory and the system memory surface has been released. Start another decode to get
        // it back. This will no-op if the image source already started a download for another
        // URI.
        //
        IFC_RETURN(m_pImageSource->ReloadReleasedSoftwareImage());
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Locks the internal surface and keeps a count of locks.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CImageBrush::Lock(
    _Out_ void **ppAddress,
    _Out_ XINT32 *pnStride,
    _Out_ XUINT32 *puWidth,
    _Out_ XUINT32 *puHeight)
{
    // TODO: merge: Method not needed. Brush span can lock the surface directly.
    HRESULT hr = S_OK;

    // Lock the inner surface ...
    if (m_pImageSource)
    {
        if (m_pImageSource->GetSoftwareSurface() != NULL)
        {
            IFC(m_pImageSource->GetSoftwareSurface()->Lock(
                    ppAddress,
                    pnStride,
                    puWidth,
                    puHeight));

            // ... and keep track of it.
            m_cLockCount++;
        }
        else
        {
            //
            // There's still no software surface after the decode. Default to the behavior of no
            // image source.
            //
            hr = E_FAIL;
        }
    }
    else
    {
        hr = E_FAIL;
    }
Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis: Unlocks the internal surface
//
//------------------------------------------------------------------------
XUINT32 CImageBrush::UnLock()
{
    if (m_cLockCount > 0 )
    {
        m_cLockCount--;
        if ( m_pImageSource && m_pImageSource->GetSoftwareSurface() )
        {
            VERIFYHR(m_pImageSource->GetSoftwareSurface()->Unlock());
        }
    }
    return m_cLockCount;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Gets the image surface
//
//------------------------------------------------------------------------
IPALSurface *CImageBrush::GetSurface()
{
    // TODO: merge: remove this method. Call GetSoftwareSurface instead.
    return GetSoftwareSurface();
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns whether the image brush is opaque
//
//------------------------------------------------------------------------
bool CImageBrush::IsOpaque()
{
    return m_pImageSource != NULL ? m_pImageSource->IsOpaque() : false;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the natural bounds
//
//------------------------------------------------------------------------
void CImageBrush::GetNaturalBounds(
    _Out_ XRECTF *pNaturalBounds
    )
{
    pNaturalBounds->X = 0;
    pNaturalBounds->Y = 0;

    if (m_pImageSource != NULL)
    {
        pNaturalBounds->Width = static_cast<XFLOAT>(m_pImageSource->GetLayoutWidth());
        pNaturalBounds->Height = static_cast<XFLOAT>(m_pImageSource->GetLayoutHeight());
    }
    else
    {
        pNaturalBounds->Width = 0;
        pNaturalBounds->Height = 0;
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the actual bounds
//
//------------------------------------------------------------------------
void CImageBrush::GetActualBounds(
    _Out_ XRECTF *pActualBounds
    )
{
    pActualBounds->X = 0;
    pActualBounds->Y = 0;

    if (m_pImageSource != NULL)
    {
        pActualBounds->Width = static_cast<XFLOAT>(m_pImageSource->GetWidth());
        pActualBounds->Height = static_cast<XFLOAT>(m_pImageSource->GetHeight());
    }
    else
    {
        pActualBounds->Width = 0;
        pActualBounds->Height = 0;
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Causes the object and its "children" to enter scope. If bLive,
//      then the object can now respond to OM requests and perform actions
//      like downloads and animation.
//
//      Derived classes are expected to first call <base>::EnterImpl, and
//      then call Enter on any "children".
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CImageBrush::EnterImpl(_In_ CDependencyObject *pNamescopeOwner, EnterParams params)
{
    CEventManager * pEventManager = nullptr;
    IPALUri* preferredBaseURI = GetPreferredBaseUri();

    if (!preferredBaseURI && pNamescopeOwner)
    {
        xref_ptr<IPALUri> baseURI;
        baseURI.attach(pNamescopeOwner->GetBaseUri());
        // GetBaseUri add-refs, but it gets dropped in this scope since CDOSharedState manages lifetime of URI object
        // and in turn this CDO participates in that CDOSharedState's lifetime.
        SetBaseUri(baseURI);
    }

    IFC_RETURN(CTileBrush::EnterImpl(pNamescopeOwner, params));

    // If there are events registered on this element, ask the
    // EventManager to extract them and a request for every event.
    if (params.fIsLive && m_pEventList)
    {
        auto core = GetContext();
        // Get the event manager.
        IFCPTR_RETURN(core);
        pEventManager = core->GetEventManager();
        IFCPTR_RETURN(pEventManager);
        IFC_RETURN(pEventManager->AddRequestsInOrder(this, m_pEventList));
    }

    if (params.fIsLive)
    {
        // Initiate the image download operation, now that the event handlers are hooked
        // up to listen to download status.
        IFC_RETURN(ReloadImage(false /*uriChanged*/, nullptr));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Causes the object and its properties to leave scope. If bLive,
//      then the object is leaving the "Live" tree, and the object can no
//      longer respond to OM requests related to being Live.   Actions
//      like downloads and animation will be halted.
//
//      Derived classes are expected to first call <base>::LeaveImpl, and
//      then call Leave on any "children".
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CImageBrush::LeaveImpl(_In_ CDependencyObject *pNamescopeOwner, LeaveParams params)
{
    IFC_RETURN(CTileBrush::LeaveImpl(pNamescopeOwner, params));

    // If we are Leaving the Live tree and there are events.
    if (params.fIsLive && m_pEventList)
    {
        // Remove the events from EventManager
        CXcpList<REQUEST>::XCPListNode *pTemp = m_pEventList->GetHead();
        while (pTemp)
        {
            REQUEST * pRequest = (REQUEST *)pTemp->m_pData;
            IFC_RETURN(GetContext()->GetEventManager()->RemoveRequest(this, pRequest));
            pTemp = pTemp->m_pNext;
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Override to base AddEventListener
//
//------------------------------------------------------------------------
_Check_return_
HRESULT
CImageBrush::AddEventListener(
    _In_ EventHandle hEvent,
    _In_ CValue *pValue,
    _In_ XINT32 iListenerType,
    _Out_opt_ CValue *pResult,
    _In_ bool fHandledEventsToo)
{
    return CEventManager::AddEventListener(this, &m_pEventList, hEvent, pValue, iListenerType, pResult, fHandledEventsToo);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Override to base RemoveEventListener
//
//------------------------------------------------------------------------
_Check_return_
HRESULT
CImageBrush::RemoveEventListener(
    _In_ EventHandle hEvent,
    _In_ CValue *pValue)
{
    return CEventManager::RemoveEventListener(this, m_pEventList, hEvent, pValue);
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Call the event manager to fire ImageFailed event.
//      If no event handler was added, the error is then reported to
//      the global OnError handler in plugin control level.
//
//------------------------------------------------------------------------
HRESULT
CImageBrush::FireImageFailed(_In_ XUINT32 iErrorCode)
{
    HRESULT hr = S_OK;

    CErrorEventArgs  *pErrArgs = NULL;

    CEventManager *pEventManager = GetContext()->GetEventManager();
    EventHandle hFailedEvent(KnownEventIndex::ImageBrush_ImageFailed);

    if (m_pImageReportCallback)
    {
        IGNOREHR(m_pImageReportCallback->FireImageFailed(iErrorCode));
    }
    else if (pEventManager)
    {
        pErrArgs = new CErrorEventArgs(GetContext());
        pErrArgs->m_eType = ImageError;
        pErrArgs->m_iErrorCode = iErrorCode;

        IGNOREHR(pErrArgs->UpdateErrorMessage( TRUE ));

        pEventManager->Raise(
            EventHandle(KnownEventIndex::ImageBrush_ImageFailed),
            true,
            this,
            pErrArgs
            );
    }

    ReleaseInterface(pErrArgs);
    RRETURN(hr);//RRETURN_REMOVAL
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Fire events by Name
//
//------------------------------------------------------------------------
HRESULT
CImageBrush::FireImageOpened()
{
    HRESULT hr = S_OK;

    CEventManager *pEventManager = GetContext()->GetEventManager();
    EventHandle hEvent(KnownEventIndex::ImageBrush_ImageOpened);
    CRoutedEventArgs *pEventArgs = NULL;

    IFCPTR(pEventManager);

    {
        pEventArgs = new CRoutedEventArgs();

        pEventManager->Raise(
            EventHandle(KnownEventIndex::ImageBrush_ImageOpened),
            true,
            this,
            pEventArgs
            );
    }

    if (m_pImageReportCallback)
    {
        IGNOREHR(m_pImageReportCallback->FireImageOpened());
    }

Cleanup:
    ReleaseInterface(pEventArgs);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Ensures a PAL brush exists and is up-to-date.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CImageBrush::UpdateAcceleratedBrush(
    _In_ const D2DRenderParams &renderParams
    )
{
    HRESULT hr = S_OK;
    IPALAcceleratedBrush *pPALBrush = NULL;
    IPALAcceleratedBitmap *pNewD2DBitmap = NULL;

    //
    // Do nothing if there is no associated image source yet
    //
    if (!m_pImageSource)
    {
        goto Cleanup;
    }

    //
    // See if there is already a copy of the surface that D2D can use
    //
    if (!m_pImageSource->GetD2DBitmap())
    {
        IPALSurface *pSWSurface = NULL;

        //
        // There is no copy of the surface that D2D can use
        // Create one (copied from the software surface)
        //
        pSWSurface = GetSurface();

        if (pSWSurface)
        {
            IFC(renderParams.GetD2DRenderTarget()->CreateBitmap(
                pSWSurface,
                &pNewD2DBitmap
                ));

            //
            // Save the D2D surface in the image source
            // This can cause the image source to release the system memory copy of the surface
            //
            m_pImageSource->SetD2DBitmap(pNewD2DBitmap);
        }
        else
        {
            //
            // Must re-download the image into a SW copy
            //
        }
    }

    if (m_pImageSource->GetD2DBitmap())
    {
        //
        // Create a D2D bitmap brush that points to the D2D surface
        //
        IFC(renderParams.GetD2DRenderTarget()->CreateSharedBitmapBrush(
            m_pImageSource->GetD2DBitmap(),
            GetOpacity(),
            &pPALBrush
            ));

        ReplaceInterface(m_pPALBrush, pPALBrush);

        IFC(CBrush::UpdateAcceleratedBrush(renderParams));
    }

Cleanup:
    ReleaseInterface(pNewD2DBitmap);
    ReleaseInterface(pPALBrush);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the print brush corresponding to this brush.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CImageBrush::GetPrintBrush(
    _In_ const D2DRenderParams& printParams,
    _Outptr_ IPALAcceleratedBrush **ppBrush
    )
{
    *ppBrush = nullptr;
    xref_ptr<IPALAcceleratedBrush> pPALBrush;
    xref_ptr<IPALSurface> spSurface;
    wrl::ComPtr<ID2D1SvgDocument> d2dSvgDocument;
    ID2D1DeviceContext5 *pD2DDeviceContextNoRef = nullptr;
    uint32_t width;
    uint32_t height;
    IFC_RETURN(printParams.GetD2DRenderTarget()->GetDeviceContext(&pD2DDeviceContextNoRef));
    IFC_RETURN(GetID2D1SvgDocument(pD2DDeviceContextNoRef, d2dSvgDocument, &width, &height));

    if (d2dSvgDocument != nullptr)
    {
        IFC_RETURN(printParams.GetD2DRenderTarget()->CreateBitmapBrush(
            d2dSvgDocument.Get(),
            GetOpacity(),
            width,
            height,
            pPALBrush.ReleaseAndGetAddressOf()
            ));
        *ppBrush = pPALBrush.detach();
    }
    else
    {
        IFC_RETURN(GetSoftwareSurfaceForPrinting(spSurface));
        if (spSurface)
        {
            IFC_RETURN(printParams.GetD2DRenderTarget()->CreateBitmapBrush(
                spSurface.get(),
                GetOpacity(),
                pPALBrush.ReleaseAndGetAddressOf()
                ));
            *ppBrush = pPALBrush.detach();
        }
    }
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns TRUE if the image brush has dimensions
//      that can be used for layout
//
//------------------------------------------------------------------------
bool CImageBrush::HasDimensions()
{
    bool result = false;

    if (m_pImageSource)
    {
        result = m_pImageSource->HasDimensions();
    }

    return result;
}

bool CImageBrush::HasRetainedSize()
{
    bool result = false;

    if (m_pImageSource)
    {
        result = m_pImageSource->HasRetainedSize();
    }

    return result;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Gets the software surface for the image brush.
//
//------------------------------------------------------------------------
_Out_opt_ IPALSurface*
CImageBrush::GetSoftwareSurface()
{
    return m_pImageSource != NULL ? m_pImageSource->GetSoftwareSurface() : NULL;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Gets the software surface for the image brush for printing. Will
//      synchronously re-decode the image if the decode software surface
//      has been thrown away, but will not mark the bitmap image as
//      needing a software surface.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CImageBrush::GetSoftwareSurfaceForPrinting(
    _Out_ xref_ptr<IPALSurface>& spSoftwareSurface
    )
{
    if (m_pImageSource != NULL)
    {
        IFC_RETURN(m_pImageSource->GetSoftwareSurfaceForPrinting(spSoftwareSurface));
    }

    return S_OK;
}

_Check_return_ HRESULT CImageBrush::GetID2D1SvgDocument(
    _In_ ID2D1DeviceContext5 *pD2DDeviceContextNoRef,
    _Out_ wrl::ComPtr<ID2D1SvgDocument>& d2dSvgDocument,
    _Out_ uint32_t* width,
    _Out_ uint32_t* height
    )
{
    if (m_pImageSource != nullptr)
    {
        IFC_RETURN(m_pImageSource->GetID2D1SvgDocument(pD2DDeviceContextNoRef, d2dSvgDocument, width, height));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Gets the hardware surface for the image brush.
//
//------------------------------------------------------------------------
_Ret_maybenull_ HWTexture*
CImageBrush::GetHardwareSurface()
{
    return m_pImageSource != NULL ? m_pImageSource->GetHardwareSurface() : NULL;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Some applications recreate a bitmap image for the same source URI
//      on each new frame. Since decode to render size will defer decode
//      until the render walk, these images will not be displayed. If this
//      pattern is detected, decode to render size is disabled for this
//      brush.
//
//------------------------------------------------------------------------

void
CImageBrush::UpdateDecodeToRenderSizeStatusOnSourceChange( _In_opt_ CImageSource * pPreviousSource )
{
    if ( m_pImageSource != NULL &&
         m_pImageSource->OfTypeByIndex<KnownTypeIndex::BitmapImage>( ) )
    {
        if ( !m_decodeToRenderSizeForcedOff &&
             pPreviousSource != NULL &&
             pPreviousSource->OfTypeByIndex<KnownTypeIndex::BitmapImage>( ) &&
             m_pImageSource->IsSourceSetAndEqualTo( pPreviousSource ) )
        {
            m_decodeToRenderSizeForcedOff = true;
        }

        if ( m_decodeToRenderSizeForcedOff )
        {
            static_cast<CBitmapImage*>(m_pImageSource)->SetDecodeToRenderSizeForcedOff();
        }
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      If the ImageSource is available, get its hardware resources so we can render it
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CImageBrush::EnsureAndUpdateHardwareResources(
    _In_ HWTextureManager *pTextureManager,
    _In_ CWindowRenderTarget *pRenderTarget,
    _In_ SurfaceCache *pSurfaceCache
    )
{
    if (m_pImageSource != NULL)
    {
        IFC_RETURN(m_pImageSource->EnsureAndUpdateHardwareResources(pTextureManager, pRenderTarget, pSurfaceCache));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the bounds of the visible portion of the image source
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CImageBrush::GetVisibleImageSourceBounds(
    _In_ const XRECTF_RB *pWindowBounds,
    _Out_ XRECTF *pBounds
    )
{
    ASSERT(m_pImageSource);

    XRECTF combinedBounds;
    EmptyRectF(&combinedBounds);

    // for each parent
    auto parentCount = GetParentCount();

    for (size_t parentIndex = 0; parentIndex < parentCount; parentIndex++)
    {
        CDependencyObject *pParentNoRef = GetParentItem(parentIndex);

        XRECTF boundsForThisParent;
        EmptyRectF(&boundsForThisParent);

        // The parent can be a UI element or a resource dictionary
        // Only UI elements contribute to the visible bounds
        if (pParentNoRef->OfTypeByIndex<KnownTypeIndex::UIElement>())
        {
            // Compute the bounds of the visible region of the VSIS for this parent
            CUIElement *pUIElementNoRef = static_cast<CUIElement *>(pParentNoRef);

            IFC_RETURN(pUIElementNoRef->GetVisibleImageBrushBounds(
                this,
                pWindowBounds,
                &boundsForThisParent
                ));
        }
        else
        {
            ASSERT(pParentNoRef->OfTypeByIndex<KnownTypeIndex::ResourceDictionary>());
        }

        UnionRectF(&combinedBounds, &boundsForThisParent);
    }

    *pBounds = combinedBounds;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CBitmapImageReportCallback::Create
//
//  Synopsis:
//      Creates a image brush error report callback
//
//------------------------------------------------------------------------
HRESULT
CBitmapImageReportCallback::Create(_In_ CImageBrush* pImageBrush,
                                        _Out_ CBitmapImageReportCallback** ppCallback)
{
    HRESULT hr = S_OK;
    CBitmapImageReportCallback* pCallback = NULL;

    IFCPTR(pImageBrush);
    IFCPTR(ppCallback);

    pCallback = new CBitmapImageReportCallback;

    pCallback->m_pImageBrush = pImageBrush;

    *ppCallback = pCallback;
    pCallback = NULL;

Cleanup:
    ReleaseInterface(pCallback);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   CBitmapImageReportCallback::Create
//
//  Synopsis:
//      Creates an image icon error report callback
//
//------------------------------------------------------------------------
HRESULT
CBitmapImageReportCallback::Create(_In_ CBitmapIcon* pBitmapIcon,
                                        _Out_ CBitmapImageReportCallback** ppCallback)
{
    HRESULT hr = S_OK;
    CBitmapImageReportCallback* pCallback = NULL;

    IFCPTR(pBitmapIcon);
    IFCPTR(ppCallback);

    pCallback = new CBitmapImageReportCallback;

    pCallback->m_pBitmapIcon = pBitmapIcon;

    *ppCallback = pCallback;
    pCallback = NULL;

Cleanup:
    ReleaseInterface(pCallback);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   CBitmapImageReportCallback::AddRef
//
//------------------------------------------------------------------------
XUINT32
CBitmapImageReportCallback::AddRef()
{
    return ++m_cRef;
}

//------------------------------------------------------------------------
//
//  Method:   CBitmapImageReportCallback::Release
//
//------------------------------------------------------------------------
XUINT32
CBitmapImageReportCallback::Release()
{
    XUINT32 rv = --m_cRef;
    if (rv == 0)
    {
        delete this;
    }
    return rv;
}

//------------------------------------------------------------------------
//
//  Method:   CBitmapImageReportCallback::FireImageFailed
//
//  Synopsis:
//      ImageBrush will call this method to have Image element to handle
//      ImageFailed event.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CBitmapImageReportCallback::FireImageFailed(_In_ XUINT32 iErrorCode)
{
    HRESULT hr = S_OK;

    if (m_pImageBrush)
    {
        hr = m_pImageBrush->FireImageFailed(iErrorCode);
    }
    else if (m_pBitmapIcon)
    {
        hr = m_pBitmapIcon->FireImageFailed(iErrorCode);
    }

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   CBitmapImageReportCallback::FireDownloadProgress
//
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CBitmapImageReportCallback::FireDownloadProgress(_In_ XFLOAT iProgress)
{
    HRESULT hr = S_OK;

    if (m_pImageBrush)
    {
        m_pImageBrush->FireImageDownloadEvent(iProgress);
    }
    else if (m_pBitmapIcon)
    {
        m_pBitmapIcon->FireImageDownloadEvent(iProgress);
    }

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   CBitmapImageReportCallback::FireImageOpened
//
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CBitmapImageReportCallback::FireImageOpened()
{
    HRESULT hr = S_OK;

    if (m_pImageBrush)
    {
        IGNOREHR(m_pImageBrush->FireImageOpened());
    }
    else if (m_pBitmapIcon)
    {
        IGNOREHR(m_pBitmapIcon->FireImageOpened());
    }

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   CBitmapImageReportCallback::Disable
//
//  Synopsis:
//      Nulls out the image element pointer.  This is called when the image
//      element is destroyed to prevent its callback from attempting to
//      call into it.
//
//------------------------------------------------------------------------
void
CBitmapImageReportCallback::Disable()
{
    m_pImageBrush = NULL;
    m_pBitmapIcon = NULL;
}

//------------------------------------------------------------------------
//
//  Method:   CBitmapImageReportCallback::ctor
//
//------------------------------------------------------------------------
CBitmapImageReportCallback::CBitmapImageReportCallback()
{
    m_pImageBrush = NULL;
    m_pBitmapIcon = NULL;
    m_cRef = 1;
}

//------------------------------------------------------------------------
//
//  Method:   CBitmapImageReportCallback::dtor
//
//------------------------------------------------------------------------
CBitmapImageReportCallback::~CBitmapImageReportCallback()
{
}
