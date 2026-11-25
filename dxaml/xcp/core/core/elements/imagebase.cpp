// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "MetadataAPI.h"
#include "LayoutManager.h"

using namespace DirectUI;

// Lists fill a reused container with another item while they lay out (ListView and ItemsRepeater prepare items in
// Measure) or run ListView's deferred build-tree work (content, x:Phase). That is the window in which layout
// transitions are suppressed as "fill-ins of data" (see CCoreServices::NWDrawTree), so a source set then belongs to
// another item.
static bool IsListFillIn(_In_ CUIElement* element)
{
    CLayoutManager* layoutManager = VisualTree::GetLayoutManagerForElement(element);
    if (layoutManager != nullptr && !layoutManager->GetAllowTransitionsToRunUnderCurrentSubtree())
    {
        return true;
    }

    for (CUIElement* ancestor = element->GetUIElementParentInternal(false); ancestor != nullptr;
        ancestor = ancestor->GetUIElementParentInternal(false))
    {
        if (ancestor->GetIsOnMeasureStack() || ancestor->GetIsOnArrangeStack())
        {
            return true;
        }
    }
    return false;
}

//------------------------------------------------------------------------
//
//  Method:   dtor
//
//  Synopsis:
//      Destructor for Image object
//
//------------------------------------------------------------------------
CImageBase::~CImageBase()
{
    if (m_pImageReportCallback)
    {
        m_pImageReportCallback->Disable();
        ReleaseInterface(m_pImageReportCallback);
    }
    if(m_pImageSource)
    {
        ReleaseInterface(m_pImageSource);
    }
}

//------------------------------------------------------------------------
//
//  Method:   CloseMedia
//
//  Synopsis:
//      Implement the Close behavior for Image element.
//
//------------------------------------------------------------------------
void
CImageBase::CloseMedia()
{
    CMediaBase::CloseMedia();

    if (m_pImageReportCallback)
    {
        m_pImageReportCallback->Disable();
        ReleaseInterface(m_pImageReportCallback);
    }

    if (m_pBackgroundBrush)
    {
        ReleaseInterface(m_pBackgroundBrush);
    }
    ReleaseFallbackBrush();
}

//------------------------------------------------------------------------
//
//  Method: Invoke
//
//  Synopsis:
//      Activate side-effects of a given property.
//
//------------------------------------------------------------------------

_Check_return_
HRESULT
CImageBase::InvokeImpl(_In_ const CDependencyProperty *pdp, _In_opt_ CDependencyObject *pNamescopeOwner)
{
    if (pdp->GetIndex() == KnownPropertyIndex::Image_Source)
    {
        if (m_pImageSource != nullptr)
        {
            m_strSource = m_pImageSource->m_strSource;
        }
        else
        {
            m_strSource = xstring_ptr::NullString();
        }

        if (HasValidMediaSource())
        {
            IFC_RETURN(EnsureMedia());
        }
        else
        {
            // Close Image element.
            CloseMedia();
        }
    }

    IFC_RETURN(CMediaBase::InvokeImpl(pdp, pNamescopeOwner));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   EnsureBrush
//
//  Synopsis:
//      Make sure our internal brush is current
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CImageBase::EnsureBrush()
{
    xref_ptr<CImageSource> tempImage;

    // Determine if the ImageSource needs to be remeasured.
    // This happens if a bitmap has been set via a stream, since the ImageSource is a DO (not UIElement),
    // we're forced to make the UpdateMeasure call here.
    if ((m_pImageSource && m_pImageSource->m_fSourceNeedsMeasure) || m_containerRequiresMeasure)
    {
        // The block is also entered when only m_containerRequiresMeasure is set. That flag is set
        // source-independently by CMediaBase::UpdateInternalSize on container size changes (including
        // 0x0) per TFS 6673752, so m_pImageSource can legitimately be null here (Source=null,
        // virtualized item recycle, or a torn-down async decode). Guard the deref to match every
        // other m_pImageSource access in this function; the remeasure must still run for the
        // container-size change, so only the flag clear is conditional.
        if (m_pImageSource)
        {
            m_pImageSource->m_fSourceNeedsMeasure = FALSE;
        }
        m_containerRequiresMeasure = false;
        IFC_RETURN(UpdateMeasure());
    }

    if (m_pBackgroundBrush == NULL)
    {
        CREATEPARAMETERS cp(GetContext());

        IFC_RETURN(CImageBrush::Create((CDependencyObject **)&(m_pBackgroundBrush), &cp));

        // Set default values
        m_pBackgroundBrush->m_AlignmentX = DirectUI::AlignmentX::Center;
        m_pBackgroundBrush->m_AlignmentY = DirectUI::AlignmentY::Center;
    }

    ASSERT(m_pBackgroundBrush);

    bool fSourceChanged = (m_pBackgroundBrush->m_pImageSource != m_pImageSource);
    bool fReloadNeeded = false;
    if (m_pImageSource)
    {
        fReloadNeeded = m_pImageSource->m_fReloadNeeded;
    }

    if (fSourceChanged)
    {
        // Keep drawing the current picture while a new URI source loads; see BaseContentRenderer::ImageRenderContent.
        // The fallback is per Image, so the new source and anything else sharing it are not affected.
        // Not when the Image now shows another item, e.g. a list fills a reused container, unless it's the same picture.
        // Not for a source that has already failed either: it won't load, and the fallback's size would keep a naturally
        // sized Image laid out.
        const bool samePicture = m_pImageSource && m_pImageSource->IsSourceSetAndEqualTo(m_pBackgroundBrush->m_pImageSource);
        bool available = false;
        const bool alreadyFailed = m_pImageSource && m_pImageSource->IsImageDecoded(&available) && !available;
        const bool keepPicture = m_pImageSource && !m_pImageSource->m_strSource.IsNull() && IsActive() && !alreadyFailed &&
            (samePicture || (!m_itemChanged && !IsListFillIn(this)));
        m_itemChanged = false;
        if (keepPicture && m_pBackgroundBrush->GetHardwareSurface() != nullptr)
        {
            CREATEPARAMETERS cp(GetContext());
            IFC_RETURN(CImageBrush::Create(reinterpret_cast<CDependencyObject**>(m_fallbackBrush.ReleaseAndGetAddressOf()), &cp));
            SetInterface(m_fallbackBrush->m_pImageSource, m_pBackgroundBrush->m_pImageSource);
            m_fallbackBrush->m_Stretch = m_pBackgroundBrush->m_Stretch;
            m_fallbackBrush->m_AlignmentX = m_pBackgroundBrush->m_AlignmentX;
            m_fallbackBrush->m_AlignmentY = m_pBackgroundBrush->m_AlignmentY;
        }
        else if (!keepPicture)
        {
            ReleaseFallbackBrush();
        }

        // Since we are detaching ImageSource from ImageBrush, in addition to releasing the source, we
        // have to tell our internal ImageBrush to take care of releasing the callback between the
        // ImageBrush and ImageSource before releasing the source.
        m_pBackgroundBrush->RemoveCallback();

        AddRefInterface(m_pImageSource);

        // m_pBackgroundBrush->m_pImageSource could be holding on to the
        // same image surface which m_pImageSource could request. In order to avoid losing the
        // surface from the bitmap' we release m_pBackgroundBrush->m_pImageSource
        // at the end of the function.
        tempImage.attach(m_pBackgroundBrush->m_pImageSource);
        m_pBackgroundBrush->m_pImageSource = m_pImageSource;

        m_pBackgroundBrush->UpdateDecodeToRenderSizeStatusOnSourceChange(tempImage);
    }

    if (fSourceChanged || fReloadNeeded)
    {
        IPALUri* preferredBaseURI = GetPreferredBaseUri();

        ReleaseInterface(m_pImageReportCallback);
        IFC_RETURN(CImageBrushReportCallback::Create(this, &m_pImageReportCallback));

        if (!preferredBaseURI)
        {
            xref_ptr<IPALUri> baseURI;
            // GetBaseUri add-refs, but it gets dropped in this scope since CDOSharedState manages lifetime of URI object
            // and in turn this CDO participates in that CDOSharedState's lifetime.
            baseURI.attach(GetBaseUri());
            SetBaseUri(baseURI);
            preferredBaseURI = baseURI;
        }

        m_pBackgroundBrush->SetBaseUri(preferredBaseURI);

        IFC_RETURN(m_pBackgroundBrush->ReloadImage(true, m_pImageReportCallback));
    }

    // Determine if Source properties need to be updated.
    // This happens if a bitmap image's source changed, but a new CBitmapImage was not created.
    if (m_pImageSource && m_pImageSource->m_fSourceUpdated)
    {
        m_pImageSource->m_fSourceUpdated = FALSE;
        m_strSource = m_pImageSource->m_strSource;
    }

    return S_OK;
}

void CImageBase::OnItemChanged()
{
    if (m_fallbackBrush)
    {
        // The new item's source was set before this notification (x:Bind ran first): stop drawing the old picture,
        // unless it's the same picture.
        if (!m_pImageSource || !m_pImageSource->IsSourceSetAndEqualTo(m_fallbackBrush->m_pImageSource))
        {
            ReleaseFallbackBrush();
            CUIElement::NWSetContentDirty(this, DirtyFlags::Render);
        }
        else
        {
            // Same picture kept for now; the new item's own source may still come next.
            m_itemChanged = true;
        }
    }
    else if (m_pBackgroundBrush && m_pBackgroundBrush->GetHardwareSurface() != nullptr)
    {
        // The new item's source comes next, e.g. from a {Binding} or a later x:Phase.
        m_itemChanged = true;
    }
}

//------------------------------------------------------------------------
//
//  Method:   CImageBase::FireImageFailed
//
//  Synopsis:
//      Handle the ImageFailed event in Image class level.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT CImageBase::FireImageFailed(_In_ XUINT32 iErrorCode)
{
    HRESULT hr = S_OK;
    CErrorEventArgs  *pErrArgs = NULL;
    CEventManager *pEventManager = GetContext()->GetEventManager();
    EventHandle hFailedEvent(KnownEventIndex::Image_ImageFailed);

    // A failed source draws nothing, so stop drawing the previous picture.
    if (m_fallbackBrush)
    {
        ReleaseFallbackBrush();
        CUIElement::NWSetContentDirty(this, DirtyFlags::Render);
    }

    // If the eventhandler is not registered, it will fall back to OnError handler
    // so there is no need to do the m_eventList check here.

    if (pEventManager)
    {
        // Generate the ErrorEventArgs based on the error code.
        pErrArgs = new CErrorEventArgs(GetContext());
        pErrArgs->m_eType = ImageError;
        pErrArgs->m_iErrorCode = iErrorCode;

        IGNOREHR(pErrArgs->UpdateErrorMessage( TRUE ));

        // ... And fire it!
        pEventManager->ThreadSafeRaise(hFailedEvent, TRUE, this, reinterpret_cast<CEventArgs**>(&pErrArgs));
    }

    ReleaseInterface(pErrArgs);
    RRETURN(hr);//RRETURN_REMOVAL
}


//------------------------------------------------------------------------
//
//  Method:   CImageBase::FireDownloadProgress
//
//  Synopsis:
//
//
//------------------------------------------------------------------------

_Check_return_ HRESULT CImageBase::FireDownloadProgress(_In_ XFLOAT downloadProgress)
{
    if(GetIsParentLayoutElement() && downloadProgress == 1)
    {
        IFC_RETURN(UpdateMeasure());
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CImageBase::FireImageOpened
//
//  Synopsis:
//      Fire the ImageOpened event to listeners.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT CImageBase::FireImageOpened()
{
    HRESULT hr = S_OK;

    CEventManager *pEventManager = GetContext()->GetEventManager();
    EventHandle hEvent(KnownEventIndex::Image_ImageOpened);
    CRoutedEventArgs *pEventArgs = NULL;

    IFCPTR(pEventManager);

    //
    // Generate the event args
    //
    {
        pEventArgs = new CRoutedEventArgs();

        // Raise the event
        pEventManager->ThreadSafeRaise(hEvent, TRUE, this, reinterpret_cast<CEventArgs**>(&pEventArgs));
    }

Cleanup:
    ReleaseInterface(pEventArgs);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   HasValidMediaSource
//
//  Synopsis:
//      Detect if a valid MediaSource is set to this element.
//
//------------------------------------------------------------------------
bool CImageBase::HasValidMediaSource( )
{
    return !m_strSource.IsNullOrEmpty()
        || (m_pImageSource);
}



_Check_return_ HRESULT CImageBase::GetNaturalBounds(_Inout_ XRECTF& pNaturalBounds)
{
    // While the new source has no size yet, keep the size of the picture still being drawn
    // (as a UriSource change does), so a naturally sized Image doesn't collapse to 0x0.
    if (m_fallbackBrush && m_pBackgroundBrush && m_pImageSource &&
        !m_pBackgroundBrush->HasDimensions() && !m_pBackgroundBrush->HasRetainedSize())
    {
        m_fallbackBrush->GetNaturalBounds(&pNaturalBounds);
        return S_OK;
    }

    // m_pBackgroundBrush->HasDimensions() is checked rather than
    // m_pBackgroundBrush->HasEitherSurface() because DX interop image sources (SurfaceImageSource, VirtualSurfaceImageSource)
    // can be in a state where they don't have surfaces, but do have dimensions for layout purposes
    if (m_pImageSource == NULL
        || m_pBackgroundBrush == NULL
        || (!m_pBackgroundBrush->HasDimensions() && !m_pBackgroundBrush->HasRetainedSize())
        )
    {
        IFC_NOTRACE_RETURN(E_UNEXPECTED);
    }

    m_pBackgroundBrush->GetNaturalBounds(&pNaturalBounds);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   OnCreateAutomationPeerImpl
//
//  Synopsis:  Creates and returns CAutomationPeer associated with this image object
//
//------------------------------------------------------------------------
CAutomationPeer* CImageBase::OnCreateAutomationPeerImpl()
{
    CImageAutomationPeer* pAP = nullptr;
    CValue value;
    value.WrapObjectNoRef(this);
    CREATEPARAMETERS cp(GetContext(), value);
    TRACE_HR_NORETURN(CImageAutomationPeer::Create((CDependencyObject**)(&pAP), &cp));
    return pAP;
}

//------------------------------------------------------------------------
//
//  Method:   UpdateMeasure
//
//  Synopsis:  Checks current image size and requested size, invalidates measure is sizes are different
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CImageBase::UpdateMeasure()
{
    if (!IsPropertyDefaultByIndex(KnownPropertyIndex::FrameworkElement_Width) && !IsPropertyDefaultByIndex(KnownPropertyIndex::FrameworkElement_Height))
    {
        // we could initialize availableSize with m_eWidth and m_eHeight as following:
        // XSIZEF availableSize = {m_eWidth, m_eHeight};
        // but since properties can be bounded to something we have to call GetValue.
        XSIZEF availableSize = {0, 0};
        XSIZEF desiredSize = {0, 0};
        CValue width;
        CValue height;

        IFC_RETURN(GetValue(GetPropertyByIndexInline(KnownPropertyIndex::FrameworkElement_Width), &width));
        IFC_RETURN(GetValue(GetPropertyByIndexInline(KnownPropertyIndex::FrameworkElement_Height), &height));

        IFC_RETURN(width.GetFloat(availableSize.width));
        IFC_RETURN(height.GetFloat(availableSize.height));

        IFC_RETURN(MeasureArrangeHelper(
            availableSize,
            desiredSize,
            PrimaryStretchDirection::Unknown /* preferPrimaryStretchDirection */,
            nullptr /* [out] actualStretchDirection */));

        if (availableSize.height != desiredSize.height ||
            availableSize.width != desiredSize.width)
        {
            InvalidateMeasure();
        }
        else
        {
            InvalidateArrange();
        }
    }
    else
    {
        InvalidateMeasure();
    }

    // The browser host and/or frame scheduler can be NULL during shutdown.
    if (IXcpBrowserHost *pBH = GetContext()->GetBrowserHost())
    {
        ITickableFrameScheduler *pFrameScheduler = pBH->GetFrameScheduler();

        if (pFrameScheduler != NULL && pFrameScheduler->IsInTick())
        {
            //
            // Request another tick specifically when we're in the middle of a tick. Normally,
            // when the layout dirty flag propagates to the root and we're not already in the middle of a tick,
            // we request another tick (see CUIElement::PropagateOnPathInternal). If we are already in the
            // middle of a tick, the assumption is that layout will run in the same tick so there's no need
            // to request another one.
            //
            // This assumption isn't always correct. CCoreServices::UpdateDirtyState can get here after all
            // layout has completed, and before any rendering begins. The layout dirty flag will then persist
            // in the tree, without another tick being requested to clean it up. This puts the tree in a corrupt
            // state where dirtying layout will not cause any more rendering.
            //
            // The safest fix is to detect this case and explicitly request another tick. The alternatives are:
            //  - Run layout again after UpdateDirtyState, which we don't want because layout can call out to
            //    user code. Running layout again would have a publically observable effect.
            //  - Run UpdateDirtyState earlier, which we don't want because things like CMediaBase::UpdateState
            //    depend on having accurate layout information. The next tick will provide the accurate layout
            //    information.
            //

            VERIFYHR(pFrameScheduler->RequestAdditionalFrame(0 /* immediate */, RequestFrameReason::ImageDirty));
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CImageBrushReportCallback::Create
//
//  Synopsis:
//      Creates a image brush error report callback
//
//------------------------------------------------------------------------
HRESULT
CImageBrushReportCallback::Create(_In_ CImageBase* pImage,
                                        _Out_ CImageBrushReportCallback** ppCallback)
{
    IFCPTR_RETURN(pImage);
    IFCPTR_RETURN(ppCallback);

    xref_ptr<CImageBrushReportCallback> pCallback = make_xref<CImageBrushReportCallback>();

    pCallback->m_pImage = pImage;

    *ppCallback = pCallback.detach();

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CImageBrushReportCallback::AddRef
//
//------------------------------------------------------------------------
XUINT32
CImageBrushReportCallback::AddRef()
{
    return ++m_cRef;
}

//------------------------------------------------------------------------
//
//  Method:   CImageBrushReportCallback::Release
//
//------------------------------------------------------------------------
XUINT32
CImageBrushReportCallback::Release()
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
//  Method:   CImageBrushReportCallback::FireImageFailed
//
//  Synopsis:
//      ImageBrush will call this method to have Image element to handle
//      ImageFailed event.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CImageBrushReportCallback::FireImageFailed(_In_ XUINT32 iErrorCode)
{
    HRESULT hr = S_OK;

    if (m_pImage)
    {
        hr = m_pImage->FireImageFailed(iErrorCode);
    }

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   CImageBrushReportCallback::FireDownloadProgress
//
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CImageBrushReportCallback::FireDownloadProgress(_In_ XFLOAT iProgress)
{
    HRESULT hr = S_OK;

    if (m_pImage)
    {
        hr = m_pImage->FireDownloadProgress(iProgress);
    }

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   CImageBrushReportCallback::FireImageOpened
//
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CImageBrushReportCallback::FireImageOpened()
{
    HRESULT hr = S_OK;

    if (m_pImage)
    {
        hr = m_pImage->FireImageOpened();
    }

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   CImageBrushReportCallback::Disable
//
//  Synopsis:
//      Nulls out the image element pointer.  This is called when the image
//      element is destroyed to prevent its callback from attempting to
//      call into it.
//
//------------------------------------------------------------------------
void
CImageBrushReportCallback::Disable()
{
    m_pImage = NULL;
}