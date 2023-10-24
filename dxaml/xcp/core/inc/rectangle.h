// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Shape.h>
#include <RectangleGeometry.h>

class TransformAndClipStack;

//------------------------------------------------------------------------
//
//  Class:  CRectangle
//
//  Synopsis:
//      Rectangle shape class
//
//------------------------------------------------------------------------

class CRectangle : public CShape
{
    friend class CMediaBase;    // For calling NWRenderPreChildrenEdgesAcceleratedVirtual

protected:
    CRectangle(_In_ CCoreServices *pCore)
        : CShape(pCore)
        , m_eRadiusX(0.0f)
        , m_eRadiusY(0.0f)
    {
        m_Stretch = DirectUI::Stretch::Fill;
    }

public:
    static _Check_return_ HRESULT Create(
        _Outptr_ CDependencyObject **ppObject,
        _In_ CREATEPARAMETERS *pCreate
        );

    // CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CRectangle>::Index;
    }
    
    // CShape overrides
    _Check_return_ HRESULT UpdateRenderGeometry() final;

protected:
    // FrameworkElement overrides
    _Check_return_ HRESULT MeasureOverride(XSIZEF availableSize, XSIZEF& desiredSize) final;
    _Check_return_ HRESULT ArrangeOverride(XSIZEF finalSize, XSIZEF& newFinalSize) final;


public:
    // CRectangle fields
    XFLOAT  m_eRadiusX;
    XFLOAT  m_eRadiusY;

//-----------------------------------------------------------------------------
// D2D Methods/Fields
//-----------------------------------------------------------------------------
protected:
    bool CanStretchGeometry() const final;

};
