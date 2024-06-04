// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

//------------------------------------------------------------------------
//
//  Method: CRenderTargetBitmapRoot destructor
//
//------------------------------------------------------------------------
CRenderTargetBitmapRoot::~CRenderTargetBitmapRoot()
{
    SAFE_DELETE(m_pMapUIElementNoRefToAttachCount);
}

//------------------------------------------------------------------------
//
//  Method: MeasureOverride
//
//  Synopsis:
//      Implementation for MeasureOverride virtual.
//      Measures children at infinity but doesn't set the Desired size of self.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CRenderTargetBitmapRoot::MeasureOverride(XSIZEF availableSize, XSIZEF& desiredSize)
{
    desiredSize.width = 0;
    desiredSize.height = 0;

    XSIZEF childConstraint = { XFLOAT_INF, XFLOAT_INF };

    auto children = GetUnsortedChildren();
    UINT32 count = children.GetCount();

    for (XUINT32 childIndex = 0; childIndex < count; ++childIndex)
    {
        CUIElement* pChild = children[childIndex];
        if (!pChild) continue;

        if (pChild->GetIsMeasureDirty() || pChild->GetIsOnMeasureDirtyPath())
        {
            IFC_RETURN(pChild->Measure(childConstraint));
        }
    }

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Method: ArrangeOverride
//
//  Synopsis:
//      implementation for ArrangeOverride virtual.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CRenderTargetBitmapRoot::ArrangeOverride(XSIZEF finalSize, XSIZEF& newFinalSize)
{
    HRESULT hr = S_OK;

    auto children = GetUnsortedChildren();
    UINT32 count = children.GetCount();

    for (XUINT32 childIndex = 0; childIndex < count; ++childIndex)
    {
        CUIElement* pChild = children[childIndex];
        if (!pChild) continue;

        if (pChild->GetIsArrangeDirty() || pChild->GetIsOnArrangeDirtyPath())
        {
            IFC(pChild->EnsureLayoutStorage());

            XRECTF childRect = { 0, 0, pChild->GetLayoutStorage()->m_desiredSize.width, pChild->GetLayoutStorage()->m_desiredSize.height };

            IFC(pChild->Arrange(childRect));
        }
    }

Cleanup:
    newFinalSize = finalSize;

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Attaches an element needed for RTB as a child.
//      Does the ref count management for the same.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CRenderTargetBitmapRoot::AttachElement(_In_ CUIElement *pElement)
{
    XUINT16 attachCount = 0;

    if (m_pMapUIElementNoRefToAttachCount == NULL)
    {
        m_pMapUIElementNoRefToAttachCount = new xchainedmap<CUIElement*, XUINT16>();
    }

    if (m_pMapUIElementNoRefToAttachCount->ContainsKey(pElement))
    {
        IFC_RETURN(m_pMapUIElementNoRefToAttachCount->Remove(pElement, attachCount));
    }
    attachCount++;
    if (attachCount == 1)
    {
        IFC_RETURN(AddChild(pElement));
    }
    IFC_RETURN(m_pMapUIElementNoRefToAttachCount->Add(pElement, attachCount));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Detaches an element needed for RTB as a child.
//      Does the ref count management for the same.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CRenderTargetBitmapRoot::DetachElement(_In_ CUIElement *pElement)
{
    XUINT16 attachCount = 0;
    ASSERT(m_pMapUIElementNoRefToAttachCount != NULL);
    ASSERT(m_pMapUIElementNoRefToAttachCount->ContainsKey(pElement));
    IFC_RETURN(m_pMapUIElementNoRefToAttachCount->Remove(pElement, attachCount));
    ASSERT(attachCount > 0);
    attachCount--;
    if (attachCount == 0)
    {
        // The element could have been removed from
        // children collection by some one else. Do not fail in such cases.
        CDOCollection* pCollection = NULL;
        pCollection = static_cast<CDOCollection*>(GetChildren());
        if (pCollection)
        {
            XINT32 iIndex = -1;
            IFC_RETURN(pCollection->IndexOf(pElement, &iIndex));
            if (iIndex >= 0)
            {
                IFC_RETURN(RemoveChild(pElement));
            }
        }
    }
    else
    {
        IFC_RETURN(m_pMapUIElementNoRefToAttachCount->Add(pElement, attachCount));
    }
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns if an element is already attached to the root.
//
//------------------------------------------------------------------------
bool
CRenderTargetBitmapRoot::IsElementAttached(_In_ CUIElement *pElement)
{
    if (m_pMapUIElementNoRefToAttachCount != NULL)
    {
        return m_pMapUIElementNoRefToAttachCount->ContainsKey(pElement);
    }
    return false;
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Walk the children of the element finding elements that intersect
//      with the given point.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CRenderTargetBitmapRoot::BoundsTestChildren(
    _In_ const XPOINTF& target,
    _In_ CBoundedHitTestVisitor* pCallback,
    _Out_opt_ BoundsWalkHitResult* pResult,
    bool canHitDisabledElements
    )
{
    if (pResult)
    {
        *pResult = BoundsWalkHitResult::Continue;
    }
    RRETURN(S_OK);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Walk the children of the element finding elements that intersect
//      with the given polygon.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CRenderTargetBitmapRoot::BoundsTestChildren(
    _In_ const HitTestPolygon& target,
    _In_ CBoundedHitTestVisitor* pCallback,
    _Out_opt_ BoundsWalkHitResult* pResult,
    bool canHitDisabledElements
    )
{
    if (pResult)
    {
        *pResult = BoundsWalkHitResult::Continue;
    }
    RRETURN(S_OK);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Printing should ignore the subtree
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CRenderTargetBitmapRoot::PrintChildren(
    _In_ const SharedRenderParams& sharedPrintParams,
    _In_ const D2DPrecomputeParams& cp,
    _In_ const D2DRenderParams& printParams
    )
{
    RRETURN(S_OK);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Subtree should be ignored for hittesting.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CRenderTargetBitmapRoot::HitTestLocalInternal(
    _In_ const XPOINTF& target,
    _Out_ bool* pHit
    )
{
    *pHit = FALSE;
    RRETURN(S_OK);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Subtree should be ignored for hittesting.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CRenderTargetBitmapRoot::HitTestLocalInternal(
    _In_ const HitTestPolygon& target,
    _Out_ bool* pHit
    )
{
    *pHit = FALSE;
    RRETURN(S_OK);
}

