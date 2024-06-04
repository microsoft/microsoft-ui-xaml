// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <DoubleUtil.h>


//------------------------------------------------------------------------
//
//  Method:   dtor
//
//  Synopsis:
//      Destructor for Image object
//
//------------------------------------------------------------------------

CMediaBase::~CMediaBase()
{
    if (m_pBackgroundRect)
    {
        VERIFYHR(m_pBackgroundRect->RemoveParent(this));
    }

    ReleaseInterface(m_pBackgroundBrush);
    ReleaseInterface(m_pBackgroundRect);
    IGNOREHR(UnregisterForUpdate());
}

_Check_return_ HRESULT CMediaBase::GetNaturalBounds(_Inout_ XRECTF& pNaturalBounds)
{
    RRETURN(E_NOTIMPL);
}

_Check_return_ HRESULT CMediaBase::MeasureOverride(_In_ XSIZEF availableSize, _Inout_ XSIZEF &desiredSize)
{
    IFC_RETURN(EnsureMedia());

    IFC_RETURN(MeasureArrangeHelper(
        availableSize,
        desiredSize,
        PrimaryStretchDirection::Unknown /* preferPrimaryStretchDirection */,
        &m_lastMeasureStretchDirection /* [out] actualStretchDirection */));

    return S_OK;
}


_Check_return_ HRESULT CMediaBase::ArrangeOverride(_In_ XSIZEF finalSize, _Inout_ XSIZEF& newFinalSize)
{
    // Due to rounding the finalSize may happen to be bigger than the desired size returned from MeasureOverride.
    // When that happens the stretch algorithm here may choose a different dimension to stretch to which may
    // result in the output primitive to not be properly aligned with the element boundaries.
    // To prevent this from happening we force the same primary stretch direction that was used in the measure pass.

    // TFS Bug #13196815:
    // It's possible layout will send us a finalSize that's radically different from the desired size.
    // If this happens, and the finalSize would result in us choosing a different stretch direction,
    // we need to re-compute the stretch direction rather than using our cached stretch direction.
    // To detect this, we'll take the desired size we requested in measure and compare to the finalSize.
    // If they don't match then we'll re-compute the stretch direction.

    PrimaryStretchDirection stretchDirection = PrimaryStretchDirection::Unknown;    // Assume we need to re-compute

    // The desired size we return to CFrameworkElement will be layout rounded if necessary and stored in m_unclippedDesiredSize.
    // CFrameworkElement goes on to apply constraints and that result is stored in m_desiredSize.  What this means is
    // we can compare m_unclippedDesiredSize to the arrange rect (finalSize) and if they are the same this indicates
    // we actually got the size we requested.  They may not match though!  This can happen if layout "changes its mind" -
    // and doesn't give us what we asked for - in that case we need to re-compute the stretch direction.
    float unclippedDesiredWidth = GetLayoutStorage()->m_unclippedDesiredSize.width;
    float unclippedDesiredHeight = GetLayoutStorage()->m_unclippedDesiredSize.height;


    if (DirectUI::DoubleUtil::AreClose(finalSize.width, unclippedDesiredWidth)
    &&  DirectUI::DoubleUtil::AreClose(finalSize.height, unclippedDesiredHeight))
    {
        // Only now can we assume it's safe to used our cached stretch direction
        stretchDirection = m_lastMeasureStretchDirection;
    }

    IFC_RETURN(MeasureArrangeHelper(
        finalSize,
        newFinalSize,
        stretchDirection,
        nullptr /* [out] actualStretchDirection */));

    return S_OK;
}


_Check_return_ HRESULT CMediaBase::MeasureArrangeHelper(
    _In_ XSIZEF availableSize,
    _Inout_ XSIZEF &desiredSize,
    _In_ PrimaryStretchDirection preferPrimaryStretchDirection,
    _Out_opt_ PrimaryStretchDirection* actualStretchDirection)
{
    HRESULT hr = S_OK;
    XSIZEF scaleFactor = {1.0, 1.0};
    XRECTF pNaturalBounds = {0};

    desiredSize.height = 0.0;
    desiredSize.width = 0.0;

    hr = GetNaturalBounds(pNaturalBounds);
    if (hr == E_UNEXPECTED)
    {
        // A height and/or width of 0 is a very common occurrence when the video is first started but not fully downloaded.
        // So we ignore that problem for layout unless the user has set an explicit width/height or minimum thereof.
        desiredSize.height = GetSpecifiedHeight();
        desiredSize.width = GetSpecifiedWidth();
        hr = S_OK;
    }
    else if(SUCCEEDED(hr))
    {
        IFC(ComputeScaleFactor(&availableSize, scaleFactor, pNaturalBounds.Width, pNaturalBounds.Height, preferPrimaryStretchDirection, actualStretchDirection));
        desiredSize.height = pNaturalBounds.Height * scaleFactor.height;
        desiredSize.width = pNaturalBounds.Width * scaleFactor.width;
    }

Cleanup:
    RRETURN(hr);

}

_Check_return_ HRESULT CMediaBase::ComputeScaleFactor(
    _In_ XSIZEF *pRenderBounds,
    _Inout_ XSIZEF& scaleFactor,
    _In_ XFLOAT naturalWidth,
    _In_ XFLOAT naturalHeight,
    _In_ PrimaryStretchDirection preferPrimaryStretchDirection,
    _Out_opt_ PrimaryStretchDirection* actualStretchDirection)
{
    scaleFactor.height = 1.0;
    scaleFactor.width = 1.0;

    if((IsInfiniteF(pRenderBounds->height) && IsInfiniteF(pRenderBounds->width))
        || m_Stretch == DirectUI::Stretch::None)
    {
        return S_OK;
    }

    scaleFactor.height = naturalHeight == 0.0f ? 0.0f : pRenderBounds->height / naturalHeight;
    scaleFactor.width = naturalWidth == 0.0f ? 0.0f : pRenderBounds->width / naturalWidth;

    if(IsInfiniteF(pRenderBounds->width))
        scaleFactor.width = scaleFactor.height;
    else if(IsInfiniteF(pRenderBounds->height))
        scaleFactor.height = scaleFactor.width;
    else
    {
        PrimaryStretchDirection stretchDirection;

        switch (preferPrimaryStretchDirection)
        {
        case PrimaryStretchDirection::Horizontal:
        case PrimaryStretchDirection::Vertical:
            stretchDirection = preferPrimaryStretchDirection;
            break;
        case PrimaryStretchDirection::Unknown:
            switch(m_Stretch)
            {
            case DirectUI::Stretch::Uniform:
                stretchDirection = scaleFactor.width < scaleFactor.height ? PrimaryStretchDirection::Horizontal : PrimaryStretchDirection::Vertical;
                break;
            case DirectUI::Stretch::UniformToFill:
                stretchDirection = scaleFactor.width > scaleFactor.height ? PrimaryStretchDirection::Horizontal : PrimaryStretchDirection::Vertical;
                break;
            case DirectUI::Stretch::Fill:
                stretchDirection = PrimaryStretchDirection::Unknown;
                break;
            default :
                ASSERT( FALSE );
                IFC_RETURN(E_UNEXPECTED);
                break;
            }
            break;
        default:
            ASSERT( FALSE );
            IFC_RETURN(E_UNEXPECTED);
            break;
        }

        switch (stretchDirection)
        {
        case PrimaryStretchDirection::Horizontal:
            scaleFactor.height = scaleFactor.width;
            break;
        case PrimaryStretchDirection::Vertical:
            scaleFactor.width = scaleFactor.height;
            break;
        }

        if (actualStretchDirection != nullptr)
        {
            *actualStretchDirection = stretchDirection;
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   EnterImpl
//
//  Synopsis:
//      CUIElement override to register for update its dirty state before the rendering starts
//
//------------------------------------------------------------------------
 _Check_return_ HRESULT
 CMediaBase::EnterImpl(_In_ CDependencyObject *pNamescopeOwner, _In_ EnterParams params)
{
    IFC_RETURN(CFrameworkElement::EnterImpl(pNamescopeOwner, params));
    if(params.fIsLive)
    {
        IFC_RETURN(RegisterForUpdate());
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   LeaveImpl
//
//  Synopsis:
//      CUIElement override to unregister for update its dirty state before the rendering starts
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
 CMediaBase::LeaveImpl(_In_ CDependencyObject *pNamescopeOwner, LeaveParams params)
{
    IFC_RETURN(CFrameworkElement::LeaveImpl(pNamescopeOwner, params));

    // Only stop requesting update callbacks when truly leaving the tree. In the case
    // that this element becomes part of a popup it will get a non-live leave of the
    // tree and then a non-live enter.
    if(params.fIsLive)
    {
        IFC_RETURN(UnregisterForUpdate());
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   RegisterForUpdate
//
//  Synopsis:
//      Registers the element for update before rendering
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CMediaBase::RegisterForUpdate()
{
    //register the media element for update so it can update its dirty state before rendering
    if(!m_fRegisteredForUpdate)
    {
        IFC_RETURN(GetContext()->AddChildForUpdate(this));
        m_fRegisteredForUpdate = TRUE;
    }

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Method:   UnregisterForUpdate
//
//  Synopsis:
//      Unregisters the element from the root visual
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CMediaBase::UnregisterForUpdate()
{
    //register the media element for update so it can update its dirty state before rendering
    if(m_fRegisteredForUpdate)
    {
        IFC_RETURN(GetContext()->RemoveChildForUpdate(this));
        m_fRegisteredForUpdate = FALSE;
    }

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Method:   UpdateState
//
//  Synopsis:
//      CUIElement override to update its dirty state before the rendering starts
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CMediaBase::UpdateState()
{
    // Make sure our contained geometry is there
    IFC_RETURN(EnsureMedia());

    if (m_pBackgroundBrush && m_pBackgroundRect)
    {
        // Set-up internal rectangle and brush.
        // Technically, the following checks and the one in EnsureMedia should probably use GetValue but this is simpler.
        if (NWIsContentDirty())
        {
            XFLOAT rActualWidth;
            XFLOAT rActualHeight;
            DirectUI::AlignmentX alignmentX;
            DirectUI::AlignmentY alignmentY;
            IFC_RETURN(UpdateInternalSize(rActualWidth, rActualHeight, alignmentX, alignmentY));

            if (   m_pBackgroundRect->m_eWidth != rActualWidth
                || m_pBackgroundRect->m_eHeight != rActualHeight)
            {
                CValue val;
                val.SetFloat(rActualWidth);
                IFC_RETURN(m_pBackgroundRect->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_Width, val));

                val.SetFloat(rActualHeight);
                IFC_RETURN(m_pBackgroundRect->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_Height, val));
            }

            if (m_pBackgroundBrush->m_Stretch != m_Stretch)
            {
                CValue val;
                val.Set(m_Stretch);
                IFC_RETURN(m_pBackgroundBrush->SetValueByKnownIndex(KnownPropertyIndex::TileBrush_Stretch, val));
            }

            if (m_pBackgroundBrush->m_AlignmentX != alignmentX)
            {
                CValue val;
                val.Set(alignmentX);
                IFC_RETURN(m_pBackgroundBrush->SetValueByKnownIndex(KnownPropertyIndex::TileBrush_AlignmentX, val));
            }

            if (m_pBackgroundBrush->m_AlignmentY != alignmentY)
            {
                CValue val;
                val.Set(alignmentY);
                IFC_RETURN(m_pBackgroundBrush->SetValueByKnownIndex(KnownPropertyIndex::TileBrush_AlignmentY, val));
            }
        }

    }

    return S_OK;
}

bool CMediaBase::IsRightToLeft()
{
    if (     m_pTextFormatting != NULL
        &&  (    !m_pTextFormatting->IsOld()
                ||  !IsPropertyDefaultByIndex(KnownPropertyIndex::FrameworkElement_FlowDirection)))
    {
        // There is a locally set or otherwise up-to-date flow direction
        return m_pTextFormatting->m_nFlowDirection == DirectUI::FlowDirection::RightToLeft;
    }
    else
    {
        // Flow direction is not set locally on this media element
        return false; // Media objects do not inherit FlowDirection
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Computes and updates internal size based on user size, stretch mode
//  layout size and natural media size.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CMediaBase::UpdateInternalSize(
    _Out_ XFLOAT &rActualWidth,
    _Out_ XFLOAT &rActualHeight,
    _Out_ DirectUI::AlignmentX &alignmentX,
    _Out_ DirectUI::AlignmentY &alignmentY
    )
{
    HRESULT hr = S_OK;
    XFLOAT rImageWidth = 0.0f;
    XFLOAT rImageHeight = 0.0f;

    // If GetNaturalBounds returns an error, we'll use the zero-size
    // rectangle as bounds.
    XRECTF naturalBounds = { 0 };
    if (SUCCEEDED(GetNaturalBounds(naturalBounds)))
    {
        rImageWidth = naturalBounds.Width;
        rImageHeight = naturalBounds.Height;
    }

    // Compute Actual Width & Height
    if (HasLayoutStorage())
    {
        // If Layout is present, use rendered size.
        rActualWidth = RenderSize.width;
        rActualHeight = RenderSize.height;
    }
    else
    {
        bool needsWidth = false;
        bool needsHeight = false;

        // Is Height or Width the default?
        if (IsDefaultWidth())
        {
            needsWidth = TRUE;
        }
        if (IsDefaultHeight())
        {
            needsHeight = TRUE;
        }

        // If Layout is not present, use given size if available.
        // If only height or width are given, compute the other using
        // aspect ratio. If given size is not available, use natural
        // size.

        rActualWidth = m_eWidth;
        rActualHeight = m_eHeight;

        if (needsWidth || needsHeight)
        {
            if (needsWidth ^ needsHeight)
            {
                // Only one dimension is specified

                //Compute natural aspect ratio
                XFLOAT rAspectRatio = 1.0f;
                if (rImageHeight > 0.0f)
                {
                    rAspectRatio = rImageWidth / rImageHeight;
                }

                // Compute other dimension using aspect ratio
                if (needsWidth)
                {
                    if (m_Stretch == DirectUI::Stretch::None)
                    {
                        rActualWidth = rImageWidth;
                    }
                    else
                    {
                        rActualWidth = m_eHeight * rAspectRatio;
                    }
                }
                else
                {
                    if (m_Stretch == DirectUI::Stretch::None)
                    {
                        rActualHeight = rImageHeight;
                    }
                    else
                    {
                        rActualHeight = m_eWidth / rAspectRatio;
                    }
                }
            }
            else
            {
                // Neither dimension is specified. Use natural size.
                rActualWidth = rImageWidth;
                rActualHeight = rImageHeight;
            }
        }

        // Clamp to max/min values
        rActualWidth =  MAX(MIN(rActualWidth, m_pLayoutProperties->m_eMaxWidth), m_pLayoutProperties->m_eMinWidth);
        rActualHeight = MAX(MIN(rActualHeight, m_pLayoutProperties->m_eMaxHeight), m_pLayoutProperties->m_eMinHeight);
    }

    // Fix for 94176: Don't set the values of the ActualWidth/ActualHeight properties
    // until we have loaded the image/media, and have a a non-zero natural size.
    if (!(rImageWidth == 0.0f && rImageHeight == 0.0f))
    {
        // Track whether the container needs to be re-measured during the next brush ensure.
        // RS1 changed the behavior of source measure to only update the m_fSourceNeedsMeasure
        // if the content size has changed whereas previously it would always set it to true when
        // a decode operation completes.  However, in this location, the parent container size
        // could change for various other reasons including setting the size to 0x0.  This adds
        // another condition in which a measure is required in addition to the content changing.
        // See TFS 6673752 for more details on the bug this fixes.
        if ((m_eActualWidth != rActualWidth) || (m_eActualHeight != rActualHeight))
        {
            m_containerRequiresMeasure = true;
        }

        m_eActualWidth = rActualWidth;
        m_eActualHeight = rActualHeight;
    }

    // There is an odd behavior with Stretch=NONE. If the element is smaller than
    // the image natural dimensions, then use Top/Left alignment.  If it is larger
    // use Center/Center.
    alignmentX = (m_Stretch == DirectUI::Stretch::None && rActualWidth < rImageWidth)
        ? DirectUI::AlignmentX::Left : DirectUI::AlignmentX::Center;

    alignmentY = (m_Stretch == DirectUI::Stretch::None && rActualHeight < rImageHeight)
        ? DirectUI::AlignmentY::Top : DirectUI::AlignmentY::Center;

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      When layout is not present (e.g. inside Canvas), UpdateInternalSize
//      computes actual width using natural bounds or aspect ratio.
//
//------------------------------------------------------------------------
_Check_return_ XCP_FORCEINLINE XFLOAT
CMediaBase::GetActualWidth()
{
    XFLOAT width;

    if (!HasLayoutStorage() && !_isnan(m_eActualWidth))
    {
        // If the actual width is not available, but the cached computed value is, use that.
        width = m_eActualWidth;
    }
    else
    {
        width = CFrameworkElement::GetActualWidth();
    }

    return width;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      When layout is not present (e.g. inside Canvas), UpdateInternalSize
//      computes actual height using natural bounds or aspect ratio.
//
//------------------------------------------------------------------------
_Check_return_ XCP_FORCEINLINE XFLOAT
CMediaBase::GetActualHeight()
{
    XFLOAT height;

    if (!HasLayoutStorage() && !_isnan(m_eActualHeight))
    {
        // If the actual height is not available, but the cached computed value is, use that.
        height = m_eActualHeight;
    }
    else
    {
        height = CFrameworkElement::GetActualHeight();
    }

    return height;
}

//------------------------------------------------------------------------
//
//  This computes the bounds of the visible region in the image source
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CMediaBase::GetVisibleImageSourceBounds(
    _In_ const XRECTF_RB *pWindowBounds,
    _Out_ XRECTF *pBounds
    )
{
    // By default, return the empty rect
    XRECTF result;
    EmptyRectF(&result);

    if (m_pBackgroundBrush)
    {
        // Compute the visible bounds
        IFC_RETURN(GetVisibleImageBrushBounds(m_pBackgroundBrush, pWindowBounds, &result));
    }

    *pBounds = result;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   HasValidMediaSource
//
//  Synopsis:
//      Detect if a valid MediaSource is set to this element.
//
//------------------------------------------------------------------------
bool CMediaBase::HasValidMediaSource()
{
    return !m_strSource.IsNullOrEmpty();
}

//------------------------------------------------------------------------
//
//  Method:   ShouldCreateBackgroundBrush
//
//  Synopsis:
//      Determine if the background brush needs to be created or not.
//
//------------------------------------------------------------------------
bool CMediaBase::ShouldCreateBackgroundBrush()
{
    return HasValidMediaSource();
}

//------------------------------------------------------------------------
//
//  Method:   EnsureMedia
//
//  Synopsis:
//      Make sure our internal rect and imagebrush are current
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CMediaBase::EnsureMedia()
{
    //
    // Primitive composition doesn't draw video in HW cache as this would require
    // a thread-safe device and read-back with the current architecture.
    //
    // Outside of HW cache, media state changes also call EnsureMedia which
    // creates an off thread invalid call.
    //
    // So, disable UI thread media realization in the PC case to address
    // both HW cached and non-HW cache cases and enable use of a single-threaded
    // device.
    //

    if (ShouldCreateBackgroundBrush())
    {
        // If we don't have a brush, create one now
        IFC_RETURN(EnsureBrush());

        // If we don't have a rectangle, make sure We create one
        IFC_RETURN(EnsureGeometry());

        // Ensure the brush is set
        if (m_pBackgroundRect->m_pFill != m_pBackgroundBrush)
        {
            IFC_RETURN(m_pBackgroundRect->SetValueByKnownIndex(KnownPropertyIndex::Shape_Fill, m_pBackgroundBrush));
        }
    }

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
CMediaBase::EnsureBrush()
{
    // REVIEW: MediaElement reference.
    //
    // The derived classes such as Image, MediaElement should implement
    // this method to ensure their own brush is current.
    //
    RRETURN(E_NOTIMPL);
}

//------------------------------------------------------------------------
//
//  Method:   EnsureGeometry
//
//  Synopsis:
//      Make sure our internal geometry is current
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CMediaBase::EnsureGeometry()
{
    if (m_pBackgroundRect == NULL)
    {
        CREATEPARAMETERS cp(GetContext());

        IFC_RETURN(CRectangle::Create(
                (CDependencyObject **)&(m_pBackgroundRect),
                &cp));

        // For mouse events to get processed for media element we need to
        // set the parent pointer of rect to this.
        // TODO: MERGE: Should the background brush be parented like the rect for dirty state?
        IFC_RETURN(m_pBackgroundRect->AddParent(this, TRUE, RENDERCHANGEDPFN(CUIElement::NWSetContentDirty)));
    }

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Method:   CloseMedia
//
//  Synopsis:
//      Close the Media element or Image element.
//
//------------------------------------------------------------------------
void
CMediaBase::CloseMedia()
{
    if (m_pBackgroundRect != NULL)
    {
        CValue val;
        val.SetObjectNoRef(nullptr);
        VERIFYHR(m_pBackgroundRect->SetValueByKnownIndex(KnownPropertyIndex::Shape_Fill, val));
    }
}

//------------------------------------------------------------------------
//
//  Method:   FireMediaEvent
//
//  Synopsis:
//      Fire events by Name
//
//------------------------------------------------------------------------
void
CMediaBase::FireMediaEvent(
    _In_ EventHandle hEvent,
    _Inout_opt_ CEventArgs **ppArgs)
{
    // Get an event handle ...
    CEventManager *pEventManager = GetContext()->GetEventManager();

    if (pEventManager)
    {
        // ... And fire it!
        pEventManager->ThreadSafeRaise( hEvent, TRUE, this, ppArgs );
    }
}

_Check_return_ HRESULT
CMediaBase::PreChildrenPrintVirtual(
    _In_ const SharedRenderParams& sharedPrintParams,
    _In_ const D2DPrecomputeParams& cp,
    _In_ const D2DRenderParams &printParams
    )
{
    IFC_RETURN(UpdateState());
    if (m_pBackgroundBrush && m_pBackgroundRect)
    {
        // Delegate to contained object - every thing must be updated in UpdateState
        IFC_RETURN(m_pBackgroundRect->PreChildrenPrintVirtual(sharedPrintParams, cp, printParams));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Override inheritance, specifically for flow direction.
//
//  Media elements do not inherit flow direction from their parent, defaulting
//  always to left-to-right.
//
//  The flow direction is recorded in the uielement->m_pTextFormatting property
//  storage group. We override this to control inheritance.
//
//  PullInheritedTextFormatting is called by depends.cpp UpdateTextFormatting
//  which has already created and initialised a TextFormatting to default
//  values.
//
//  Further, media elements make no use of other text properties, nor expose
//  them to children.
//
//  Therefore we make PullInheritedTextFormatting a no-op.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT CMediaBase::PullInheritedTextFormatting()
{
    IFCEXPECT_ASSERT_RETURN(m_pTextFormatting != NULL);
    m_pTextFormatting->SetIsUpToDate();

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Generate bounds for the content of the element.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CMediaBase::GenerateContentBounds(
    _Out_ XRECTF_RB* pBounds
    )
{
    if (m_pBackgroundRect != NULL)
    {
        IFC_RETURN(m_pBackgroundRect->GetInnerBounds(pBounds));
    }
    else
    {
        EmptyRectF(pBounds);
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Test if a point intersects with the element in local space.
//
//  NOTE:
//      Overridden in derived classes to provide more detailed hit testing.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CMediaBase::HitTestLocalInternal(
    _In_ const XPOINTF& target,
    _Out_ bool* pHit
    )
{
    if (m_pBackgroundRect != NULL)
    {
        IFC_RETURN(m_pBackgroundRect->HitTestLocal(target, pHit));
    }
    else
    {
        *pHit = FALSE;
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Test if a polygon intersects with the element in local space.
//
//  NOTE:
//      Overridden in derived classes to provide more detailed hit testing.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CMediaBase::HitTestLocalInternal(
    _In_ const HitTestPolygon& target,
    _Out_ bool* pHit
    )
{
    if (m_pBackgroundRect != NULL)
    {
        IFC_RETURN(m_pBackgroundRect->HitTestLocal(target, pHit));
    }
    else
    {
        *pHit = FALSE;
    }

    return S_OK;
}

