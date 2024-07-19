// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "FocusRectangle.h"

#include <MetadataAPI.h>

using namespace DirectUI;

CFocusRectangle::CFocusRectangle(_In_ CCoreServices *pCore)
    : CRectangle(pCore)
{
    EmptyRectF(&m_bounds);
}

//  Creates and initializes new instance of CFocusRectangle class.
_Check_return_ HRESULT CFocusRectangle::Create(
    _In_ CCoreServices *pCore,
    _Outptr_ CFocusRectangle **ppFocusRectangle
    )
{
    auto focusRect = make_xref<CFocusRectangle>(pCore);

    CValue val;
    auto dashArrayCollection = make_xref<CDoubleCollection>(pCore);
    val.SetFloat(1.0f);
    VERIFYHR(dashArrayCollection->Append(val));
    VERIFYHR(dashArrayCollection->Append(val));

    VERIFYHR(focusRect->SetStroke(nullptr));
    VERIFYHR(focusRect->SetStrokeDashArray(dashArrayCollection));

    val.SetDouble(0.5);
    VERIFYHR(focusRect->SetEffectiveValueInSparseStorage(
        MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::Shape_StrokeDashOffset), val));

    val.SetEnum8(XcpPenCapSquare);
    VERIFYHR(focusRect->SetEffectiveValueInSparseStorage(
        MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::Shape_StrokeEndLineCap), val));

    focusRect->EnsurePropertyRenderData(RWT_WinRTComposition);

    *ppFocusRectangle = focusRect.detach();
    return S_OK;
}

_Check_return_ HRESULT CFocusRectangle::SetStrokeThickness(float thickness)
{
    CValue val;
    val.SetDouble(thickness);
    return SetEffectiveValueInSparseStorage(
        MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::Shape_StrokeThickness),
        val);
}

_Check_return_ HRESULT CFocusRectangle::SetStrokeDashOffset(float dashOffset)
{
    CValue val;
    val.SetDouble(dashOffset);
    return SetEffectiveValueInSparseStorage(
        MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::Shape_StrokeDashOffset),
        val);
}

xref_ptr<CBrush> CFocusRectangle::GetStroke() const
{
    return m_stroke;
}

_Check_return_ HRESULT CFocusRectangle::SetStroke(xref_ptr<CBrush> stroke)
{
    m_stroke = std::move(stroke);
    return S_OK;
}

bool CFocusRectangle::HasStrokeDashArray() const
{
    return m_strokeDashArray != nullptr;
}

xref_ptr<CDoubleCollection> CFocusRectangle::GetStrokeDashArray() const
{
    return m_strokeDashArray;
}

_Check_return_ HRESULT CFocusRectangle::SetStrokeDashArray(xref_ptr<CDoubleCollection> strokeDashArray)
{
    m_strokeDashArray = std::move(strokeDashArray);
    return S_OK;
}


//------------------------------------------------------------------------
//
//  Sets the bounds for the focus rectangle and updates the layout.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CFocusRectangle::SetBounds(
    _In_ const XRECTF &bounds,
    _In_ CBrush *pStrokeBrush
    )
{
    HRESULT hr = S_OK;
    bool needsArrange = false;

    auto stroke = GetStroke();
    if (pStrokeBrush != stroke.get())
    {
        needsArrange = TRUE;
        VERIFYHR(SetStroke(xref_ptr<CBrush>(pStrokeBrush)));
    }

    if (bounds.X != m_bounds.X ||
        bounds.Y != m_bounds.Y ||
        bounds.Width != m_bounds.Width ||
        bounds.Height != m_bounds.Height)
    {
        m_bounds  = bounds;
        m_eWidth  = bounds.Width;
        m_eHeight = bounds.Height;
        EnterPCScene();
        ClearPCRenderData();
    }
    else if (needsArrange)
    {
        EnterPCScene();
        ClearPCRenderData();
    }

    RRETURN(hr);
}
