// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

//------------------------------------------------------------------------
//
//  Method:   dtor
//
//  Synopsis:
//      Destructor for canvas object
//
//------------------------------------------------------------------------

CCanvas::~CCanvas()
{
}

_Check_return_ HRESULT
CCanvas::MeasureOverride(XSIZEF availableSize, XSIZEF& desiredSize)
{
    desiredSize.width = 0;
    desiredSize.height = 0;

    XSIZEF childConstraint = {XFLOAT_INF, XFLOAT_INF};

    auto children = GetUnsortedChildren();
    UINT32 count = children.GetCount();

    for (XUINT32 childIndex = 0; childIndex < count; ++childIndex)
    {
        CUIElement* pChild = children[childIndex];
        if (!pChild) continue;

        if (pChild->GetIsMeasureDirty() || pChild->GetIsOnMeasureDirtyPath())
            IFC_RETURN(pChild->Measure(childConstraint));
    }

    return S_OK;
}

_Check_return_ HRESULT
CCanvas::ArrangeOverride(XSIZEF finalSize, XSIZEF& newFinalSize)
{
    HRESULT hr = S_OK;

    auto children = GetUnsortedChildren();
    UINT32 count = children.GetCount();

    for (XUINT32 childIndex = 0; childIndex < count; ++childIndex)
    {
        CUIElement* pChild = children[childIndex];
        if (!pChild) continue;

        XFLOAT x = pChild->GetOffsetX();
        XFLOAT y = pChild->GetOffsetY();

        if (pChild->GetIsArrangeDirty() || pChild->GetIsOnArrangeDirtyPath())
        {
            IFC(pChild->EnsureLayoutStorage());

            XRECTF childRect = { x, y, pChild->GetLayoutStorage()->m_desiredSize.width, pChild->GetLayoutStorage()->m_desiredSize.height };

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
//      Canvas does not normally support layout clipping.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CCanvas::UpdateLayoutClip(bool forceClipToRenderSize)
{
    // Canvas doesn't clip to its bounds typically.  However, if clipping is being forced it
    // should happen for all elements, even this one.
    if (forceClipToRenderSize)
    {
        return CFrameworkElement::UpdateLayoutClip(forceClipToRenderSize);
    }
    else
    {
        return CUIElement::UpdateLayoutClip(forceClipToRenderSize);
    }
}
