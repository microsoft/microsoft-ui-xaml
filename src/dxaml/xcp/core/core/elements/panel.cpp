// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "DependencyObject.h"
#include "BrushTransition.g.h"

//------------------------------------------------------------------------
//
//  Method:   ctor
//
//  Synopsis:
//      Constructor for control object
//
//------------------------------------------------------------------------
CPanel::CPanel(_In_ CCoreServices *pCore)
    : CFrameworkElement(pCore)
    , m_pBackground(nullptr)
    , m_fNWBackgroundDirty(FALSE)
    , m_fNWBorderBrushDirty(FALSE)
{
    m_eWidth = static_cast<XFLOAT>(XDOUBLE_NAN);
    m_eHeight = static_cast<XFLOAT>(XDOUBLE_NAN);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Destructor for panel
//
//------------------------------------------------------------------------
CPanel::~CPanel()
{
    ReleaseInterface(m_pBackground);
}

bool CPanel::GetIsIgnoringTransitions() const
{
    CValue result;
    VERIFYHR(GetValueByIndex(KnownPropertyIndex::Panel_IsIgnoringTransitions, &result));
    ASSERT(result.GetType() == valueBool);
    return result.AsBool();
}

bool CPanel::GetIsItemsHost() const
{
    CValue result;
    VERIFYHR(GetValueByIndex(KnownPropertyIndex::Panel_IsItemsHost, &result));
    ASSERT(result.GetType() == valueBool);
    return result.AsBool();
}

_Check_return_ HRESULT CPanel::SetValue(_In_ const SetValueParams& args)
{
    switch (args.m_pDP->GetIndex())
    {
        case KnownPropertyIndex::Panel_ChildrenTransitions:
        {
            if (args.m_value.AsObject() || args.m_value.AsIInspectable())
            {
                IFC_RETURN(CPanel::EnsureTransitionStorageForChildren(this));
            }
            // lifetime of transitioncollection is manually handled since it has no parent
            IFC_RETURN(SetPeerReferenceToProperty(args.m_pDP, args.m_value));
            break;
        }

        case KnownPropertyIndex::Panel_Background:
        {
            bool isAnimationEnabled = true;
            IGNOREHR(FxCallbacks::FrameworkCallbacks_IsAnimationEnabled(&isAnimationEnabled));

            DCompTreeHost* dcompTreeHost = GetDCompTreeHost();

            if (dcompTreeHost != nullptr && isAnimationEnabled)
            {

                CBrushTransition* const backgroundTransition = CUIElement::GetBrushTransitionNoRef(*this, KnownPropertyIndex::Panel_BackgroundTransition);

                if (backgroundTransition != nullptr)
                {
                    dcompTreeHost->GetWUCBrushManager()->SetUpBrushTransitionIfAllowed(
                        m_pBackground /* from */,
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

//------------------------------------------------------------------------
//  Synopsis: Adding a child will add storage, but if a transition is set
//  for the first time, it should proactively go into each child to ensure storage.
//------------------------------------------------------------------------
_Check_return_ HRESULT CPanel::EnsureTransitionStorageForChildren(_In_ CPanel* pPanel)
{
    HRESULT hr = S_OK;
    CUIElement* pChild = NULL;

    if (pPanel)
    {
        CCollection *pCollection = pPanel->GetChildren();
        if (pCollection)
        {
            // only do this if we have children
            XUINT32 cKids =  pCollection->GetCount();

            for (XUINT32 i =0; i < cKids; i++)
            {
                pChild = (CUIElement *)pCollection->GetItemWithAddRef(i);
                IFC(CUIElement::EnsureLayoutTransitionStorage(pChild, NULL, TRUE));
                ReleaseInterface(pChild);
            }
        }
    }

Cleanup:
    ReleaseInterface(pChild);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      This RENDERCHANGEDPFN marks this Panel's background brush as dirty for rendering.
//
//------------------------------------------------------------------------
/* static */ void
CPanel::NWSetBackgroundDirty(_In_ CDependencyObject *pTarget, DirtyFlags flags)
{
    ASSERT(pTarget->OfTypeByIndex<KnownTypeIndex::Panel>());

    if (!flags_enum::is_set(flags, DirtyFlags::Independent))
    {
        CPanel *pPanel = static_cast<CPanel*>(pTarget);

        //
        // Background brush - Dirties: (Render | Bounds)
        //
        pPanel->NWSetDirtyFlagsAndPropagate(flags | DirtyFlags::Render | DirtyFlags::Bounds, pPanel->m_fNWBackgroundDirty);
        pPanel->m_fNWBackgroundDirty = TRUE;
    }
}

/* static */ void
CPanel::NWSetBorderBrushDirty(_In_ CDependencyObject *pTarget, DirtyFlags flags)
{
    ASSERT(pTarget->OfTypeByIndex<KnownTypeIndex::Panel>());

    if (!flags_enum::is_set(flags, DirtyFlags::Independent))
    {
        CPanel *pPanel = static_cast<CPanel*>(pTarget);

        //
        // Border brush - Dirties: (Render)
        //
        pPanel->NWSetDirtyFlagsAndPropagate(flags | DirtyFlags::Render, pPanel->m_fNWBorderBrushDirty);
        pPanel->m_fNWBorderBrushDirty = TRUE;
    }
}

_Check_return_ HRESULT
CPanel::PreChildrenPrintVirtual(
    _In_ const SharedRenderParams& sharedPrintParams,
    _In_ const D2DPrecomputeParams& cp,
    _In_ const D2DRenderParams& printParams
    )
{
    xref_ptr<IPALAcceleratedGeometry> pPALBackgroundGeometry;
    xref_ptr<IPALAcceleratedBrush> pPALBackgroundBrush;
    xref_ptr<IPALAcceleratedGeometry> pPALBorderGeometry;
    xref_ptr<IPALAcceleratedBrush> pPALBorderBrush;
    AcceleratedBrushParams PALBorderBrushParams;
    AcceleratedBrushParams PALBackgroundBrushParams;

    if (printParams.GetOverrideBrush() != nullptr)
    {
        pPALBackgroundBrush = printParams.GetOverrideBrush().Get();
        pPALBorderBrush = printParams.GetOverrideBrush().Get();
    }
    else
    {
        if (printParams.m_renderFill && m_pBackground)
        {
            IFC_RETURN(m_pBackground->GetPrintBrush(printParams, pPALBackgroundBrush.ReleaseAndGetAddressOf()));
        }

        if (printParams.m_renderStroke && GetBorderBrush())
        {
            IFC_RETURN(GetBorderBrush()->GetPrintBrush(printParams, pPALBorderBrush.ReleaseAndGetAddressOf()));
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
//  Synopsis: Children of panel should inherit the child transitions
//            of its parents.
//
//  Note: In the case of this panel being an itemshost we need to
//  return the itemscontrols child transitions.
//
//------------------------------------------------------------------------
CTransitionCollection*
CPanel::GetTransitionsForChildElementNoAddRef(_In_ CUIElement* pChild)
{
    // Panel.ChildrenTransitions is a create-on-demand property, so we need to first check
    // if it's set in a manner that doesn't automatically create a value for it
    CValue value = CheckOnDemandProperty(KnownPropertyIndex::Panel_ChildrenTransitions);
    auto childrenTransitions = checked_cast<CTransitionCollection>(value.AsObject());

    if ((childrenTransitions == nullptr) && GetIsItemsHost())
    {
        // todo: need to do a test for perf here.
        CDependencyObject* pItemsControlDO = do_pointer_cast<CItemsPresenter>(GetParent());
        CItemsControl* pItemsControl = NULL;
        if (pItemsControlDO)
        {
            pItemsControlDO = pItemsControlDO->GetInheritanceParentInternal();
        }
        while (pItemsControlDO)
        {
            pItemsControl = do_pointer_cast<CItemsControl>(pItemsControlDO);
            if (pItemsControl)
                break;
            pItemsControlDO = pItemsControlDO->GetInheritanceParentInternal();
        }

        if (pItemsControl)
        {
            childrenTransitions = pItemsControl->m_pItemContainerTransitions;
        }
    }

    // Caller doesn't expect us to AddRef.
    return childrenTransitions;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Generate bounds for the content of the element if there is a background/border.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CPanel::GenerateContentBounds(
    _Out_ XRECTF_RB* pBounds
    )
{
    if (m_pBackground != nullptr || GetBorderBrush()!= nullptr)
    {
        IFC_RETURN(GenerateContentBoundsImpl(pBounds));
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
//      Always generate bounds for the content of the element.
//
//  NOTE:
//      CPanel would normally return an emptyrect if no background exists, this
//      forces the bounding rectangle to be calculated
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
CPanel::GenerateContentBoundsImpl(
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
CPanel::HitTestLocalInternal(
    _In_ const XPOINTF& target,
    _Out_ bool* pHit
    )
{
    RRETURN(CBorder::HitTestLocalInternalImpl(this, target, pHit));
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
CPanel::HitTestLocalInternal(
    _In_ const HitTestPolygon& target,
    _Out_ bool* pHit
    )
{
    RRETURN(CBorder::HitTestLocalInternalImpl(this, target, pHit));
}

_Check_return_
HRESULT
CPanel::PanelGetClosestIndexSlow(
    _In_ XPOINTF location,
    _Out_ XINT32* pReturnValue)
{
    HRESULT hr = S_OK;
    CUIElement* pChild = NULL;
    XFLOAT shortestDistance = XFLOAT_MAX;
    XUINT32 index = XUINT32_MAX;
    XUINT32 cKids = 0;

    // naive implementation that knows nothing about internals, but just looks at where a
    // child has been arranged. It will cost N.
    // it does not involve transforms.

    // the algorithm is simply to find the closest point on the bounding rect of the item
    // to the passed in point.
    // 1. see if there is a straight line on x-axis
    // 2. see if there is a straight line on y-axis
    // both: we can short-circuit the algorithm since we are within the item.
    //       (currently I do not care about multiple items taking up the same space).
    // single: that is the shortest distance
    // none  : find the closest edge and calculate the distance

    *pReturnValue = -1;

    CCollection *pCollection = GetChildren();
    if (pCollection)
    {
        // only do this if we have children
        cKids =  pCollection->GetCount();

        for (XUINT32 i=0; i < cKids; i++)
        {
            XFLOAT x1 = 0.0f;
            XFLOAT y1 = 0.0f;
            XFLOAT x2 = 0.0f;
            XFLOAT y2 = 0.0f;
            XFLOAT distance = XFLOAT_MAX;

            pChild = static_cast<CUIElement*>(pCollection->GetItemWithAddRef(i));

            x1 = pChild->GetActualOffsetX();
            y1 = pChild->GetActualOffsetY();
            x2 = x1 + pChild->GetActualWidth();
            y2 = y1 + pChild->GetActualHeight();

            // short circuit - point is located within boundaries
            if (x1 <= location.x && location.x <= x2 &&
                y1 <= location.y && location.y <= y2)
            {
                // we have found a great point
                distance = 0.0f;
            }
            else
            {
                // straight line up/down from point would intersect the x-axis of the bounds
                if (x1 <= location.x && location.x <= x2)
                {
                    // so the shortest distance would be the distance in the y direction
                    distance = XcpAbsF(location.y - y1);
                    distance = MIN(XcpAbsF(location.y - y2), distance);
                }
                else if (y1 <= location.y && location.y <= y2)
                {
                    // so the shortest distance would be the distance in the x direction
                    distance = XcpAbsF(location.x - x1);
                    distance = MIN(XcpAbsF(location.x - x2), distance);
                }
                else if ( (XcpAbsF(location.x - x1) < shortestDistance || XcpAbsF(location.x - x2) < shortestDistance) &&
                          (XcpAbsF(location.y - y1) < shortestDistance || XcpAbsF(location.y - y2) < shortestDistance))
                {
                    // need to find the point on the bounds that is closest and then
                    // calculate the distance.

                    // the if-condition above makes sure we only undertake this calculation if we have a chance of being smaller

                    XFLOAT a = MIN(XcpAbsF(location.x - x1), XcpAbsF(location.x - x2));
                    XFLOAT b = MIN(XcpAbsF(location.y - y1), XcpAbsF(location.y - y2));
                    distance =  sqrtf(a * a + b * b);
                }
            }

            // update
            if (distance < shortestDistance)
            {
                shortestDistance = distance;
                index = i;
                if (distance == 0.0f)
                {
                    break;
                }
            }

            ReleaseInterface(pChild);
        }
    }

    // set the returnvalue if we found something
    if (index <= cKids)
    {
        *pReturnValue = (XINT32)index;
    }


    ReleaseInterface(pChild);
    RRETURN(hr);
}

bool CPanel::IsMaskDirty(
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
            m_pBackground, m_fNWBackgroundDirty, isFillBrushAnimated, pIsFillForHitTestOnly,
            GetBorderBrush().get(), m_fNWBorderBrushDirty, isStrokeBrushAnimated, pIsStrokeForHitTestOnly);
}

xref_ptr<CBrush> CPanel::GetBackgroundBrush() const
{
    return xref_ptr<CBrush>(m_pBackground);
}

xref_ptr<CBrush> CPanel::GetBorderBrush() const
{
    CValue result;
    VERIFYHR(GetValueByIndex(KnownPropertyIndex::Panel_BorderBrushProtected, &result));
    return static_sp_cast<CBrush>(result.DetachObject());
}

XTHICKNESS CPanel::GetBorderThickness() const
{
    CValue result;
    VERIFYHR(GetValueByIndex(KnownPropertyIndex::Panel_BorderThicknessProtected, &result));
    return *(result.AsThickness());
}

XCORNERRADIUS CPanel::GetCornerRadius() const
{
    CValue result;
    VERIFYHR(GetValueByIndex(KnownPropertyIndex::Panel_CornerRadiusProtected, &result));
    return *(result.AsCornerRadius());
}
