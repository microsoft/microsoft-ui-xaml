// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <FrameworkTheming.h>
#define SWAP(Type, a, b) {Type temp = a; a = b; b = temp;}

//------------------------------------------------------------------------
//
//  Method:   CBorder destructor
//
//------------------------------------------------------------------------
CBorder::~CBorder()
{
    // Visual children
    ReleaseInterface(m_pBorderBrush);
    ReleaseInterface(m_pBackgroundBrush);
}

//-------------------------------------------------------------------------
//  Synopsis:   Set a value in the dependency object. This is the local value.
//-------------------------------------------------------------------------
_Check_return_ HRESULT CBorder::SetValue(_In_ const SetValueParams& args)
{
    switch (args.m_pDP->GetIndex())
    {
        // when this elements receives a transition meant for its child,
        // proactively ensure storage on that child so it starts to
        // take snapshots.
        case KnownPropertyIndex::Border_ChildTransitions:
        {
            CUIElement* pChild = do_pointer_cast<CUIElement>(GetFirstChildNoAddRef());
            if (pChild)
            {
                IFC_RETURN(CUIElement::EnsureLayoutTransitionStorage(pChild, NULL, TRUE));
            }
            // lifetime of transitioncollection is manually handled since it has no parent
            IFC_RETURN(SetPeerReferenceToProperty(args.m_pDP, args.m_value));
            break;
        }

        case KnownPropertyIndex::Border_Background:
        {
            bool isAnimationEnabled = true;
            IGNOREHR(FxCallbacks::FrameworkCallbacks_IsAnimationEnabled(&isAnimationEnabled));

            DCompTreeHost* dcompTreeHost = GetDCompTreeHost();

            if (dcompTreeHost != nullptr && isAnimationEnabled)
            {
                CBrushTransition* const backgroundTransition = CUIElement::GetBrushTransitionNoRef(*this, KnownPropertyIndex::Border_BackgroundTransition);

                if (backgroundTransition != nullptr)
                {
                    dcompTreeHost->GetWUCBrushManager()->SetUpBrushTransitionIfAllowed(
                        m_pBackgroundBrush /* from */,
                        do_pointer_cast<CSolidColorBrush>(args.m_value) /* to */,
                        *this,
                        ElementBrushProperty::Fill,
                        *backgroundTransition);
                }
                else
                {
                    dcompTreeHost->GetWUCBrushManager()->CleanUpBrushTransition(*this, ElementBrushProperty::Fill);
                }
            }

            break;
        }
    }

    IFC_RETURN(CFrameworkElement::SetValue(args));

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Synopsis:   This is overridden so that we can return an error if the
//              parser adds more than one child to the Border.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CBorder::AddChild(_In_ CUIElement * pChild)
{
    CUIElement* pExistingChild = NULL;

    IFCEXPECT_RETURN(pChild != NULL);

    // Can only have one child!
    pExistingChild = GetFirstChildNoAddRef();
    IFCEXPECT_RETURN(pExistingChild == NULL);

    IFC_RETURN(CUIElement::AddChild(pChild));

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Synopsis:   This is the child property getter and setter method. Note
//              that the storage for this property is actually this children
//              collection.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CBorder::Child(
    _In_ CDependencyObject *pObject,
    _In_ XUINT32 cArgs,
    _Inout_updates_(cArgs) CValue *ppArgs,
    _In_opt_ IInspectable* pValueOuter,
    _Out_ CValue *pResult)
{
    HRESULT hr = S_OK;
    CBorder* pBorder = NULL;

    // Validate parameters
    if (!pObject || !pObject->OfTypeByIndex<KnownTypeIndex::Border>())
    {
        IFC(E_INVALIDARG);
    }
    IFC(DoPointerCast(pBorder, pObject));

    if (cArgs == 0)
    {
        // Getting the child
        CUIElement* pChild = NULL;
        hr = pBorder->GetChild(&pChild);
        if (SUCCEEDED(hr))
        {
            pResult->SetObjectNoRef(pChild);
        }
        else
        {
            pResult->SetNull();
        }
    }
    else if (cArgs == 1 && ppArgs->GetType() == valueObject && ppArgs->AsObject()->OfTypeByIndex<KnownTypeIndex::UIElement>())
    {
        // Setting the child
        CUIElement* pChild;
        IFC(DoPointerCast(pChild, *ppArgs));
        IFC(pBorder->SetChild(pChild));
    }
    else if (cArgs == 1 && ppArgs->GetType() == valueNull)
    {
        IFC(pBorder->SetChild(NULL));
    }
    else
    {
        IFC(E_INVALIDARG);
    }
Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Synopsis:   Remove any existing child and set the new child tree.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CBorder::SetChild(_In_ CUIElement* pChild)
{
    HRESULT hr = S_OK;

    CUIElement* pExistingChild = NULL;
    IFC(GetChild(&pExistingChild));

    if (pExistingChild)
    {
        IFC(RemoveChild(pExistingChild));
    }

    if (pChild)
    {
        IFC(AddChild(pChild));
    }

Cleanup:
    // GetChild will AddRef, so match that here -
    // When we remove the child we want to give up
    // our current reference or we will leak.
    ReleaseInterface(pExistingChild);
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CBorder::GetChild()
//
//  Synopsis:   This will return the first (and only) child, or NULL if
//              the there is no Child yet.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CBorder::GetChild(_Outptr_ CUIElement** ppChild)
{
    IFCPTR_RETURN(ppChild);

    // Get First Child will not return a Addref'ed interface
    // so we need to add ref it.
    *ppChild = do_pointer_cast<CUIElement>(GetFirstChildNoAddRef());
    AddRefInterface(*ppChild);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CBorder::MeasureOverride
//
//  Synopsis: Returns the desired size for layout purposes.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CBorder::MeasureOverride(
    XSIZEF availableSize,
    XSIZEF& desiredSize)
{
    HRESULT hr = S_OK;
    XSIZEF childAvailableSize;

    XSIZEF combined = CBorder::HelperGetCombinedThickness(this);

    // Get the child to measure it - if any.
    CUIElement* pChild = NULL;
    IFC(GetChild(&pChild));

    //If we have a child
    if (pChild)
    {
        // Remove combined size from child's reference size.
        childAvailableSize.width    = MAX(0.0f, availableSize.width     - combined.width);
        childAvailableSize.height   = MAX(0.0f, availableSize.height    - combined.height);

        IFC(pChild->Measure(childAvailableSize));
        IFC(pChild->EnsureLayoutStorage());

        // Desired size would be my child's desired size plus the border
        desiredSize.width   = pChild->DesiredSize.width    + combined.width;
        desiredSize.height  = pChild->DesiredSize.height   + combined.height;
    }
    else
    {
        desiredSize.width   = combined.width;
        desiredSize.height  = combined.height;
    }

Cleanup:
    ReleaseInterface(pChild);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   CBorder::ArrangeOverride
//
//  Synopsis: Returns the final render size for layout purposes.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CBorder::ArrangeOverride(
    XSIZEF finalSize,
    XSIZEF& newFinalSize)
{
    HRESULT hr = S_OK;

    // Get the child to arrange it - if any.
    CUIElement* pChild = NULL;
    IFC(GetChild(&pChild));

    //If we have a child
    if (pChild)
    {
        XRECTF childRect = HelperGetInnerRect(this, finalSize);

        // Give the child the inner rectangle as the available size
        // and ask it to arrange itself within this rectangle.
        IFC(pChild->Arrange(childRect));
    }

Cleanup:
    ReleaseInterface(pChild);
    newFinalSize = finalSize;
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Helper function to return a size object based on a thickness.
//
//------------------------------------------------------------------------
/*static*/ XSIZEF
CBorder::HelperCollapseThickness(
    _In_ const XTHICKNESS& thickness
    )
{
    XSIZEF size;
    size.width  = thickness.left + thickness.right;
    size.height = thickness.top  + thickness.bottom;
    return size;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Helper function used to 'subtract' a thickness from a a rectangle to calculate the
//      inner rectangle dimensions.
//
//------------------------------------------------------------------------
/*static*/ void
CBorder::HelperDeflateRect(
    _In_ const XRECTF& rect,
    _In_ const XTHICKNESS& thickness,
    _Out_ XRECTF& innerRect
    )
{
    innerRect.X = rect.X + thickness.left;
    innerRect.Y = rect.Y + thickness.top;
    innerRect.Width     = MAX(0.0f, rect.Width  - (thickness.left + thickness.right));
    innerRect.Height    = MAX(0.0f, rect.Height - (thickness.top  + thickness.bottom));
}

/*static*/
XRECTF CBorder::HelperGetInnerRect(
    _In_ CFrameworkElement* pElement,
    _In_ const XSIZEF& outerSize
    )
{
    XRECTF innerRect;
    XRECTF outerRect;
    XRECTF rcChild;

    XTHICKNESS thickness = pElement->GetBorderThickness();
    // Set up the bound rectangle
    outerRect.X = 0;
    outerRect.Y = 0;
    outerRect.Width = outerSize.width;
    outerRect.Height = outerSize.height;

    if (pElement->GetUseLayoutRounding())
    {
        outerRect.Width  = pElement->LayoutRound(outerRect.Width);
        outerRect.Height = pElement->LayoutRound(outerRect.Height);
        thickness = CBorder::GetLayoutRoundedThickness(pElement);
    }

    // Calculate the inner one
    CBorder::HelperDeflateRect(outerRect, thickness, innerRect);
    CBorder::HelperDeflateRect(innerRect, pElement->GetPadding(), rcChild);

    return rcChild;
}

/*static*/
XSIZEF CBorder::HelperGetCombinedThickness(
    _In_ CFrameworkElement* pElement
    )
{
    XSIZEF border;
    XSIZEF padding;
    XTHICKNESS thickness = pElement->GetBorderThickness();

    if (pElement->GetUseLayoutRounding())
    {
        thickness = GetLayoutRoundedThickness(pElement);
    }

    // Compute the chrome size added by the border
    border = CBorder::HelperCollapseThickness(thickness);

    // Compute the chrome size added by the padding.
    // No need to adjust for layout rounding here since padding is not "drawn" by the border.
    padding = CBorder::HelperCollapseThickness(pElement->GetPadding());

    // Combine both.
    XSIZEF combined;
    combined.width  = border.width  + padding.width;
    combined.height = border.height + padding.height;

    return combined;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Helper function used to produce a thickness rounded at current
//      plateau scale.
//
//------------------------------------------------------------------------
XTHICKNESS
CBorder::GetLayoutRoundedThickness(
    _In_ CFrameworkElement* pElement
    )
{
    // Layout rounding will correctly round element sizes and offsets at the current plateau,
    // but does not round BorderThicnkess. Since plateau scale is applied as a scale transform at the
    // root element, all values will be scaled by it including BorderThickness so if a user sets
    // BorderThickness = 1 at PLateau=1.4 this will be scaled to 1.4, producing blurry edges at the
    // inner edges and other rendering artifacts. This method rounds the BorderThickness at the current plateau
    // using plateau-aware LayoutRound utility.
    XTHICKNESS roundedThickness;
    XTHICKNESS thickness = pElement->GetBorderThickness();
    roundedThickness.left = pElement->LayoutRound(thickness.left);
    roundedThickness.right = pElement->LayoutRound(thickness.right);
    roundedThickness.top = pElement->LayoutRound(thickness.top);
    roundedThickness.bottom = pElement->LayoutRound(thickness.bottom);

    return roundedThickness;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Prints the Border by sending print instructions to the print render target.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CBorder::PreChildrenPrintVirtual(
    _In_ const SharedRenderParams& sharedPrintParams,
    _In_ const D2DPrecomputeParams& cp,
    _In_ const D2DRenderParams& printParams
    )
{
    xref_ptr<IPALAcceleratedGeometry> pPALBackgroundGeometry;
    xref_ptr<IPALAcceleratedBrush> pPALBackgroundBrush;
    xref_ptr<IPALAcceleratedGeometry> pPALBorderGeometry;
    xref_ptr<IPALAcceleratedBrush> pPALBorderBrush;
    AcceleratedBrushParams PALBackgroundBrushParams;
    AcceleratedBrushParams PALBorderBrushParams;

    if (printParams.GetOverrideBrush() != nullptr)
    {
        pPALBackgroundBrush = printParams.GetOverrideBrush().Get();
        pPALBorderBrush = printParams.GetOverrideBrush().Get();
    }
    else
    {
        if (printParams.m_renderFill && m_pBackgroundBrush != NULL)
        {
            IFC_RETURN(m_pBackgroundBrush->GetPrintBrush(printParams, pPALBackgroundBrush.ReleaseAndGetAddressOf()));
        }

        if (printParams.m_renderStroke && m_pBorderBrush != NULL)
        {
            IFC_RETURN(m_pBorderBrush->GetPrintBrush(printParams, pPALBorderBrush.ReleaseAndGetAddressOf()));
        }
    }

    // Calculate the border and background geometries.
    IFC_RETURN(CreateBorderGeometriesAndBrushClipsCommon(
        sharedPrintParams.renderCollapsedMask,
        cp,
        sharedPrintParams.pCurrentTransform,
        &PALBackgroundBrushParams,
        &PALBorderBrushParams,
        printParams.m_renderFill ? pPALBackgroundGeometry.ReleaseAndGetAddressOf() : nullptr,
        printParams.m_renderStroke ? pPALBorderGeometry.ReleaseAndGetAddressOf() : nullptr));

    IFC_RETURN(AcceleratedBorderRenderCommon(
        sharedPrintParams,
        printParams,
        printParams.m_renderFill ? pPALBackgroundGeometry.get() : nullptr,
        printParams.m_renderFill ? pPALBackgroundBrush.get() : nullptr,
        &PALBackgroundBrushParams,
        printParams.m_renderStroke ? pPALBorderGeometry.get() : nullptr,
        printParams.m_renderStroke ? pPALBorderBrush.get() : nullptr,
        &PALBorderBrushParams));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      This RENDERCHANGEDPFN marks this Border's background brush as dirty.
//
//------------------------------------------------------------------------
/* static */ void
CBorder::NWSetBackgroundBrushDirty(_In_ CDependencyObject *pTarget, DirtyFlags flags)
{
    ASSERT(pTarget->OfTypeByIndex<KnownTypeIndex::Border>());

    if (!flags_enum::is_set(flags, DirtyFlags::Independent))
    {
        CBorder *pBorder = static_cast<CBorder*>(pTarget);

        //
        // Background brush - Dirties: (Render)
        //
        pBorder->NWSetDirtyFlagsAndPropagate(flags | DirtyFlags::Render, pBorder->m_fNWBackgroundBrushDirty);
        pBorder->m_fNWBackgroundBrushDirty = TRUE;
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      This RENDERCHANGEDPFN marks this Border's border brush as dirty for rendering.
//
//------------------------------------------------------------------------
/* static */ void
CBorder::NWSetBorderBrushDirty(_In_ CDependencyObject *pTarget, DirtyFlags flags)
{
    ASSERT(pTarget->OfTypeByIndex<KnownTypeIndex::Border>());

    if (!flags_enum::is_set(flags, DirtyFlags::Independent))
    {
        CBorder *pBorder = static_cast<CBorder*>(pTarget);

        //
        // Border brush - Dirties: (Render)
        //
        pBorder->NWSetDirtyFlagsAndPropagate(flags | DirtyFlags::Render, pBorder->m_fNWBorderBrushDirty);
        pBorder->m_fNWBorderBrushDirty = TRUE;
    }
}

// Child of border should get the Child transitions of its parent (this border).
_Check_return_ CTransitionCollection*
CBorder::GetTransitionsForChildElementNoAddRef(_In_ CUIElement*)
{
    CValue result = CheckOnDemandProperty(KnownPropertyIndex::Border_ChildTransitions);

    return static_cast<CTransitionCollection*>(result.AsObject());
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Overridden implementation of GetBoundsForImageBrushVirtual.  This returns
//      the bounds used to compute the brush transform for the specified image brush.
//      This accounts for the deflation of the interior region for the background brush.
//      This matches the implementations of BorderBackgroundPart and BorderOutlinePart
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CBorder::GetBoundsForImageBrushVirtual(
    _In_ const CImageBrush *pImageBrush,
    _Out_ XRECTF *pBounds
    )
{
    XRECTF boundRect = {};

    // Call the default implementation which returns
    // { 0, 0, GetActualWidth(), GetActualHeight() }
    IFC_RETURN(CUIElement::GetBoundsForImageBrushVirtual(
        pImageBrush,
        &boundRect
        ));

    const bool extendBackgroundUnderBorder =
        (GetBackgroundSizing() == DirectUI::BackgroundSizing::OuterBorderEdge);

    if (m_pBackgroundBrush == pImageBrush)
    {
        if (extendBackgroundUnderBorder)
        {
            // If the background extends under the border, just return the
            // regular bounds.
            *pBounds = boundRect;
        }
        else
        {
            // For an inset background, deflate the rect to account for
            // the border thickness.
            XRECTF innerRect = {};

            CBorder::HelperDeflateRect(boundRect, m_borderThickness, innerRect);
            *pBounds = innerRect;
        }
    }
    else
    {
        ASSERT(pImageBrush == m_pBorderBrush);

        // For the outline, just return the regular bounds
        *pBounds = boundRect;
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Generate bounds for the content of the element.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CBorder::GenerateContentBounds(
    _Out_ XRECTF_RB* pBounds
    )
{
    HRESULT hr = S_OK;

    pBounds->left = 0.0f;
    pBounds->top = 0.0f;
    pBounds->right = GetActualWidth();
    pBounds->bottom = GetActualHeight();

    RRETURN(hr);
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
CBorder::HitTestLocalInternal(
    _In_ const XPOINTF& target,
    _Out_ bool* pHit
    )
{
    RRETURN(HitTestLocalInternalImpl(this, target, pHit));
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
CBorder::HitTestLocalInternal(
    _In_ const HitTestPolygon& target,
    _Out_ bool* pHit
    )
{
    RRETURN(HitTestLocalInternalImpl(this, target, pHit));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Test if a point/polygon intersects with the element in local space.
//
//------------------------------------------------------------------------
template <typename HitType>
_Check_return_ HRESULT
CBorder::HitTestLocalInternalImpl(
    _In_ CFrameworkElement* pElement,
    _In_ const HitType& target,
    _Out_ bool* pHit
    )
{
    ASSERT(pElement->OfTypeByIndex<KnownTypeIndex::Panel>()
        || pElement->OfTypeByIndex<KnownTypeIndex::Border>()
        || pElement->OfTypeByIndex<KnownTypeIndex::ContentPresenter>());

    XRECTF outerRect = { 0.0f, 0.0f, pElement->GetActualWidth(), pElement->GetActualHeight() };
    bool intersectsTarget = false;

    if (outerRect.Width != 0.0f && outerRect.Height != 0.0f)
    {
        const XCORNERRADIUS cornerRadius = pElement->GetCornerRadius();
        const XTHICKNESS thickness = pElement->GetBorderThickness();
        const xref_ptr<CBrush> backgroundBrush = pElement->GetBackgroundBrush();
        const xref_ptr<CBrush> borderBrush = pElement->GetBorderBrush();
        const bool extendBackgroundUnderBorder = (pElement->GetBackgroundSizing() == DirectUI::BackgroundSizing::OuterBorderEdge);
        const bool useComplexDrawing = CBorder::HasNonZeroCornerRadius(cornerRadius);
        const bool invisibleHitTestMode = pElement->GetContext()->InvisibleHitTestMode();
        const bool hasValidBorder = borderBrush != nullptr;
        const bool hasValidBackground = (backgroundBrush != nullptr || invisibleHitTestMode);
        XRECTF innerRect = {};
        CBorder::HelperDeflateRect(outerRect, thickness, innerRect);
        bool targetIntersectsInnerRect = false;
        bool targetIntersectsOuterRect = false;

        if (hasValidBorder || (hasValidBackground && extendBackgroundUnderBorder))
        {
            IFC_RETURN(CBorder::DoesBorderRectIntersectHitType(
                outerRect,
                useComplexDrawing,
                thickness,
                cornerRadius,
                target,
                true, /* isOuter */
                targetIntersectsOuterRect));
        }

        // If there is a background brush that extends underneath the border,
        // we hit test against this outer rect to see if it contains the point.
        // Note that if InvisibleHitTestMode is set and the background is null,
        // we test this element as if it had a solid color background instead,
        // i.e. if the point intersects, it's considered a hit.
        if (hasValidBackground && extendBackgroundUnderBorder && targetIntersectsOuterRect)
        {
            if (backgroundBrush)
            {
                IFC_RETURN(backgroundBrush->HitTestBrushClipInLocalSpace(&outerRect, target, &intersectsTarget));
            }
            else
            {
                ASSERT(invisibleHitTestMode);
                intersectsTarget = true;
            }
        }

        // If we have not confirmed a hit yet, there are a few other scenarios
        // we have to test. First, we have to determine if we need to test for
        // a hit within the inner rect (if it exists at all). We only need to
        // do that if we have a) a valid inset background, or b) if we have a
        // valid border, because we will use a hit within the inner rect as an
        // exclusion to verify that a hit within the outer rect did indeed hit
        // the border stroke.
        if (!intersectsTarget
            && ((hasValidBackground && !extendBackgroundUnderBorder) || hasValidBorder)
            && innerRect.Width != 0.0f
            && innerRect.Height != 0.0f)
        {
            IFC_RETURN(CBorder::DoesBorderRectIntersectHitType(
                innerRect,
                useComplexDrawing,
                thickness,
                cornerRadius,
                target,
                false, /* isOuter */
                targetIntersectsInnerRect));
        }

        // At this point, if the target intersects with the inner rect and
        // InvisibleHitTestMode is set, this is considered a hit. If it is
        // not set, but there is an inset background brush, we do a hit test on
        // the brush.
        if (!intersectsTarget && hasValidBackground && !extendBackgroundUnderBorder && targetIntersectsInnerRect)
        {
            if (backgroundBrush)
            {
                IFC_RETURN(backgroundBrush->HitTestBrushClipInLocalSpace(&innerRect, target, &intersectsTarget));
            }
            else
            {
                ASSERT(invisibleHitTestMode);
                intersectsTarget = true;
            }
        }

        // If we have not confirmed a hit yet and there is a border brush, then
        // we must test if the point hits this outer stroke. Note that we don't
        // do any additional work for InvisibleHitTestMode here; if the border
        // brush is null, we treat hit testing as if the thickness is zero.
        // Now, the border is the difference of the outer rect and the inner
        // rect, so a hit on the border can be successful only if it lands on
        // the outer rect and not within the inner rect.
        if (!intersectsTarget && hasValidBorder && targetIntersectsOuterRect && !targetIntersectsInnerRect)
        {
            IFC_RETURN(borderBrush->HitTestBrushClipInLocalSpace(&outerRect, target, &intersectsTarget));
        }
    }

    *pHit = intersectsTarget;

    return S_OK;
}

bool CBorder::HitTestRoundedCornerClip(
    _In_ CFrameworkElement* element,
    _In_ const XPOINTF& target
)
{
    return HitTestRoundedCornerClipImpl(element, target);
}

bool CBorder::HitTestRoundedCornerClip(
    _In_ CFrameworkElement* element,
    _In_ const HitTestPolygon& target
    )
{
    return HitTestRoundedCornerClipImpl(element, target);
}

// Perform rounded corner hit-testing for the element itself.
// In this scenario we care only about hit-testing the actual bounds of the element,
// clipped down to its rounded corner rectangle shape.  Content is not relevant here.
template <typename HitType>
bool CBorder::HitTestRoundedCornerClipImpl(
    _In_ CFrameworkElement* element,
    _In_ const HitType& target
    )
{
    xref_ptr<HitTestGeometrySink> hitTestSink;

    XRECTF actualBounds;
    actualBounds.X = 0.0f;
    actualBounds.Y = 0.0f;
    actualBounds.Width  = element->GetActualWidth();
    actualBounds.Height = element->GetActualHeight();

    IFCFAILFAST(CBorder::CreateHitTestGeometrySink(target, 0.25f, nullptr, hitTestSink.ReleaseAndGetAddressOf()));

    IFCFAILFAST(CGeometryBuilder::DrawRoundedCornersRectangle(hitTestSink, actualBounds, element->GetCornerRadius(), nullptr, FALSE));

    bool targetIntersectsFill = false;
    IFCFAILFAST(hitTestSink->GetResult(&targetIntersectsFill));

    return targetIntersectsFill;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates a hit geometry sink used for point hit testing.
//
//------------------------------------------------------------------------
template <>
_Check_return_ HRESULT
CBorder::CreateHitTestGeometrySink(
    _In_ const XPOINTF& target,
    XFLOAT tolerance,
    _In_opt_ const CMILMatrix* pTransform,
    _Out_ HitTestGeometrySink** ppHitTestSink
        )
{
    HRESULT hr = S_OK;

    *ppHitTestSink = new PointHitTestGeometrySink(target, tolerance, pTransform);

    RRETURN(hr);//RRETURN_REMOVAL
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates a hit geometry sink used for polygon hit testing.
//
//------------------------------------------------------------------------
template <>
_Check_return_ HRESULT
CBorder::CreateHitTestGeometrySink(
    _In_ const HitTestPolygon& target,
    XFLOAT tolerance,
    _In_opt_ const CMILMatrix* pTransform,
    _Out_ HitTestGeometrySink** ppHitTestSink
        )
{
    HRESULT hr = S_OK;

    *ppHitTestSink = new PolygonHitTestGeometrySink(target, tolerance, pTransform);

    RRETURN(hr);//RRETURN_REMOVAL
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Verifies if a hit intersects within a given rect constructed
//      from a border.
//
//------------------------------------------------------------------------
template <typename HitType>
_Check_return_ HRESULT
CBorder::DoesBorderRectIntersectHitType(
    _In_ const XRECTF& rect,
    _In_ const bool useComplexDrawing,
    _In_ const XTHICKNESS& borderThickness,
    _In_ const XCORNERRADIUS& cornerRadius,
    _In_ const HitType& target,
    _In_ const bool isOuter,
    _Out_ bool& intersects
)
{
    intersects = false;

    if (useComplexDrawing)
    {
        const float hitTestTolerance = 0.25f;
        xref_ptr<HitTestGeometrySink> hitTestSink;

        IFC_RETURN(CBorder::CreateHitTestGeometrySink(target, hitTestTolerance, nullptr, hitTestSink.ReleaseAndGetAddressOf()));
        IFC_RETURN(CGeometryBuilder::DrawRoundedCornersRectangle(hitTestSink.get(), rect, cornerRadius, &borderThickness, isOuter));
        IFC_RETURN(hitTestSink->GetResult(&intersects));
    }
    else
    {
        intersects = DoesRectIntersectHitType(rect, target);
    }

    return S_OK;
}

//----------------------------------------------------------------------------
//  Synopsis:
//      Determines if a new mask part of the element is needed.  The element
//      will only have a mask if both the brush and texture are non-null
//-----------------------------------------------------------------------------
bool CBorder::IsMaskDirty(
    _In_ HWShapeRealization *pHwShapeRealization,
    const bool renderCollapsedMask,
    bool isFillBrushAnimated,
    bool isStrokeBrushAnimated,
    _Out_ bool* pIsFillForHitTestOnly,
    _Out_ bool* pIsStrokeForHitTestOnly
    )
{
    return CFrameworkElement::NWIsContentDirty()
        || pHwShapeRealization->MaskNeedsUpdate(
            renderCollapsedMask,
            m_pBackgroundBrush, m_fNWBackgroundBrushDirty, isFillBrushAnimated, pIsFillForHitTestOnly,
            m_pBorderBrush, m_fNWBorderBrushDirty, isStrokeBrushAnimated, pIsStrokeForHitTestOnly);
}

void CBorder::OnContentDirty()
{
    HWRealization *hwRealizationNoRef = GetHWRealizationCache();

    if (hwRealizationNoRef != nullptr)
    {
        ASSERT(hwRealizationNoRef->GetType() == HWRealizationType::Shape);
    }
}

void CBorder::SetHWRealizationCache(_In_opt_ HWRealization *pNewRenderingCache)
{
    __super::SetHWRealizationCache(pNewRenderingCache);
}


/*static*/ _Check_return_ HRESULT CBorder::CreateForFocusRendering(_In_ CCoreServices* core, _Outptr_ CBorder** newBorder)
{
    CBorder* border = new CBorder(core);

    border->EnsurePropertyRenderData(RWT_WinRTComposition);

    *newBorder = border;
    return S_OK;
}

xref_ptr<CBrush> CBorder::GetBackgroundBrush() const
{
    auto core = GetContext();

    // If HighContrastAdjustment is enabled override the backgroud color to ensure high contrast.
    if (core->GetFrameworkTheming()->HasHighContrastTheme() && m_useBackgroundOverride)
    {
        return xref_ptr<CBrush>(core->GetSystemColorWindowBrushNoRef());
    }
    else
    {
        return xref_ptr<CBrush>(m_pBackgroundBrush);
    }
}

DirectUI::BackgroundSizing CBorder::GetBackgroundSizing() const
{
    CValue result;
    VERIFYHR(GetValueByIndex(KnownPropertyIndex::Border_BackgroundSizing, &result));
    return static_cast<DirectUI::BackgroundSizing>(result.AsEnum());
}

void CBorder::SetUseBackgroundOverride(bool useBackgroundOverride)
{
    m_useBackgroundOverride = useBackgroundOverride;
}

