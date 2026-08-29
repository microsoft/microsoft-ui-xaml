// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "StickyHeaderWrapper.h"
#include "CompositeTransform.g.h"
#include "SecondaryContentRelationship_Partial.h"
#include "ScrollViewer.g.h"
#include "Panel.g.h"
#include "TranslateTransform.g.h"
#include "DoubleAnimation.g.h"
#include "Storyboard.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

// Work around disruptive max/min macros
#undef max
#undef min

StickyHeaderWrapper::StickyHeaderWrapper(const wf::Rect& desiredBounds, DOUBLE groupBottom, DOUBLE panelOffset, DOUBLE pannableExtent)
    : m_pLTEParentNoRef(nullptr)
    , m_desiredBounds(desiredBounds)
    , m_groupBottom(groupBottom)
    , m_panelOffset(panelOffset)
    , m_pannableExtent(pannableExtent)
{
}

StickyHeaderWrapper::~StickyHeaderWrapper()
{
    VERIFYHR(Clear());
    if (m_epStoryboardCompleted)
    {
        VERIFYHR(m_epStoryboardCompleted.DetachEventHandler(m_spStoryboard.Get()));
    }
}

_Check_return_ HRESULT StickyHeaderWrapper::Clear()
{
    HRESULT hr = S_OK;

    auto spTargetElement = m_wrTarget.AsOrNull<IUIElement>();
    CUIElement* pNativeTarget = nullptr;
    if (spTargetElement)
    {
        IFC(spTargetElement.Cast<UIElement>()->put_RenderTransform(nullptr));
        pNativeTarget = spTargetElement.Cast<UIElement>()->GetHandle();
    }
    IFC(CoreImports::LayoutTransitionElement_Destroy(
        DXamlCore::GetCurrent()->GetHandle(),
        pNativeTarget,
        m_pLTEParentNoRef,
        m_spLayoutTransitionElement->GetHandle()));

    IFC(m_spRelationship->Remove());

Cleanup:
    m_spRelationship = nullptr;
    m_wrTarget = nullptr;
    m_wrTargetHost = nullptr;
    m_pLTEParentNoRef = nullptr;
    m_spTransform = nullptr;
    m_spRelationship = nullptr;
    RRETURN(hr);
}

_Check_return_ HRESULT
StickyHeaderWrapper::Initialize(
    _In_ DirectUI::UIElement* pHeader,
    _In_ DirectUI::Panel* pPanel,
    _In_ DirectUI::ScrollViewer* pScrollViewer,
    _In_ DirectUI::UIElement* pLTEParent,
    _In_ xaml::Internal::ISecondaryContentRelationshipStatics* pSecondaryContentRelationshipStatics)
{
    HRESULT hr = S_OK;
    DOUBLE effectiveTop = 0;
    DOUBLE effectiveBottom = 0;
    ctl::ComPtr<CompositeTransform> spHeaderTransform;

    CUIElement* pNativeLTE = nullptr;

    ASSERT(!m_spLayoutTransitionElement);
    ASSERT(!m_wrTarget);
    ASSERT(!m_wrTargetHost);
    ASSERT(!m_pLTEParentNoRef);

    IFC(ctl::AsWeak(pHeader, &m_wrTarget));
    IFC(ctl::AsWeak(pPanel, &m_wrTargetHost));
    m_pLTEParentNoRef = pLTEParent->GetHandle();

    IFC(CoreImports::LayoutTransitionElement_Create(
        DXamlCore::GetCurrent()->GetHandle(),
        pHeader->GetHandle(),
        m_pLTEParentNoRef,
        false,
        &pNativeLTE));
    {
        ctl::ComPtr<DependencyObject> spLTEAsDO;
        IFC(DXamlCore::GetCurrent()->GetPeer(pNativeLTE, KnownTypeIndex::UIElement, &spLTEAsDO));
        m_spLayoutTransitionElement.Attach(static_cast<UIElement*>(spLTEAsDO.Detach()));
    }

    UpdateLTEPosition();

    // Now that we've created the persistent LTE, let's do some of the other work of setting this up
    ASSERT(!m_spTransform);
    IFC(ctl::make(&m_spTransform));
    IFC(m_spLayoutTransitionElement->put_RenderTransform(m_spTransform.Get()));

    IFC(m_spLayoutTransitionElement->put_Opacity(1));

    // Also put a render transform on the header itself as this also needs to reflect the sticky header curve
    // See notes in ParametricCurve.cpp - CSecondaryContentRelationship::UpdateDependencyProperties().
    IFC(ctl::make(&spHeaderTransform));
    IFC(pHeader->put_RenderTransform(spHeaderTransform.Get()));

    GetEffectiveDimensions(&effectiveTop, &effectiveBottom);

    ASSERT(!m_spRelationship);
    IFC(pSecondaryContentRelationshipStatics->CreateStickyHeaderRelationship(
        pScrollViewer,
        m_spLayoutTransitionElement.Get(),
        m_spTransform.Get(),
        spHeaderTransform.Get(),
        effectiveTop,
        effectiveBottom,
        m_desiredBounds.Height,
        &m_spRelationship));
    IFC(m_spRelationship->Apply());

Cleanup:
    ReleaseInterface(pNativeLTE);
    RRETURN(hr);
}

void
StickyHeaderWrapper::GetEffectiveDimensions(
    _Out_ DOUBLE* pEffectiveTop,
    _Out_ DOUBLE* pEffectiveBottom)
{
    *pEffectiveTop = m_desiredBounds.Y + m_panelOffset;
    *pEffectiveBottom = m_groupBottom + m_panelOffset;
}

_Check_return_ HRESULT
StickyHeaderWrapper::SetBounds(
    _In_ const wf::Rect& desiredBounds, DOUBLE groupBottom, DOUBLE panelOffset, DOUBLE pannableExtent, _Out_ bool* pCurveUpdated)
{
    HRESULT hr = S_OK;
    bool updated = false;
    *pCurveUpdated = false;

    if (!DoubleUtil::AreClose(desiredBounds.Y, m_desiredBounds.Y)
        || !DoubleUtil::AreClose(desiredBounds.Height, m_desiredBounds.Height)
        || !DoubleUtil::AreClose(groupBottom, m_groupBottom)
        || !DoubleUtil::AreClose(panelOffset, m_panelOffset)
        || !DoubleUtil::AreClose(pannableExtent, m_pannableExtent))
    {
        DOUBLE effectiveTop = 0;
        DOUBLE effectiveBottom = 0;

        m_desiredBounds = desiredBounds;
        m_groupBottom = groupBottom;
        m_panelOffset = panelOffset;
        m_pannableExtent = pannableExtent;

        GetEffectiveDimensions(&effectiveTop, &effectiveBottom);

        // Update the curve
        IFC(UpdateStickyHeaderCurve(m_spRelationship.Cast<SecondaryContentRelationship>(), effectiveTop, effectiveBottom, m_desiredBounds.Height));

        // Synchronize the change of this sticky header's curve
        // See extensive banner comments on CInputServices::PrepareSecondaryContentRelationshipForCurveUpdate().
        IFC(m_spRelationship->PrepareForCurveUpdate());

        IFC(m_spRelationship->Apply());

        UpdateLTEPosition();
        updated = true;
    }

    if (!updated)
    {
        if (!DoubleUtil::AreClose(desiredBounds.X, m_desiredBounds.X)
            || !DoubleUtil::AreClose(desiredBounds.Width, m_desiredBounds.Width))
        {
            UpdateLTEPosition();
        }
    }

    *pCurveUpdated = updated;

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT StickyHeaderWrapper::SetDesiredBounds(_In_ const wf::Rect& desiredBounds)
{
    bool curveUpdated = false;
    RRETURN(SetBounds(desiredBounds, m_groupBottom, m_panelOffset, m_pannableExtent, &curveUpdated));
}

void StickyHeaderWrapper::UpdateLTEPosition()
{
    auto pCoreLTE = static_cast<CLayoutTransitionElement*>(m_spLayoutTransitionElement->GetHandle());
    float xOffset = m_desiredBounds.X;

    auto spTargetElement = m_wrTarget.AsOrNull<IUIElement>();
    if (spTargetElement)
    {
        auto pNativeTarget = spTargetElement.Cast<UIElement>()->GetHandle();
        xOffset = pNativeTarget->GetActualOffsetX();

        // Update the stored X offset so that the cached value
        // matches for the next check to see if we need to update.
        m_desiredBounds.X = xOffset;
    }

    const XPOINTF position = { xOffset, m_desiredBounds.Y };
    pCoreLTE->SetDestinationOffset(position);
    CUIElement::NWSetTransformDirty(pCoreLTE, DirtyFlags::Render | DirtyFlags::Bounds);
}

_Check_return_ HRESULT StickyHeaderWrapper::NotifyLayoutTransitionStart()
{
    IFC_RETURN(m_spLayoutTransitionElement->put_Opacity(0));
    return S_OK;
}

_Check_return_ HRESULT StickyHeaderWrapper::NotifyLayoutTransitionEnd()
{
    IFC_RETURN(m_spLayoutTransitionElement->put_Opacity(1));
    return S_OK;
}

_Check_return_ HRESULT StickyHeaderWrapper::UpdateHeaderPosition()
{
    HRESULT hr = S_OK;

    IFC(m_spRelationship->UpdateDependencyProperties());

Cleanup:
    RRETURN(hr);
}
