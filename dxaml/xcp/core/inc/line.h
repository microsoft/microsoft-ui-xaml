// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class VisualContentRenderer;

class CLine final : public CShape
{
protected:
    CLine(_In_ CCoreServices *pCore)
        : CShape(pCore)
    {}

public:
    DECLARE_CREATE(CLine);

    // CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CLine>::Index;
    }

    // CShape overrides
    _Check_return_ HRESULT UpdateRenderGeometry() override;

public:
    XFLOAT  m_eX1 = 0.0f;
    XFLOAT  m_eY1 = 0.0f;
    XFLOAT  m_eX2 = 0.0f;
    XFLOAT  m_eY2 = 0.0f;
};


//------------------------------------------------------------------------
//
//  Class:  CLineGeometry
//
//  Synopsis:
//      Linear geometry class
//
//------------------------------------------------------------------------

class CLineGeometry final : public CGeometry
{
private:
    CLineGeometry(_In_ CCoreServices *pCore)
        : CGeometry(pCore)
    {}

public:
    DECLARE_CREATE(CLineGeometry);

    // CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CLineGeometry>::Index;
    }

    WUComp::ICompositionGeometry* GetCompositionGeometry(_In_ VisualContentRenderer* renderer) override;

public:
    XPOINTF m_ptStart   = {};
    XPOINTF m_ptEnd     = {};

//-----------------------------------------------------------------------------
// Printing Methods
//-----------------------------------------------------------------------------
protected:
    _Check_return_ HRESULT GetPrintGeometryVirtual(
        _In_ const D2DPrecomputeParams& cp,
        _In_ const D2DRenderParams &printParams,
        IPALAcceleratedGeometry** ppGeometry
        ) override;

    //-----------------------------------------------------------------------------
    //
    //  Bounds and Hit Testing
    //
    //-----------------------------------------------------------------------------
public:
    _Check_return_ HRESULT GetBounds(
        _Out_ XRECTF_RB* pBounds
        ) override;

protected:
     _Check_return_ HRESULT VisitSinkInternal(
        _In_ IPALGeometrySink* pSink
        ) override;
};
