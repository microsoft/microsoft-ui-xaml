// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//  Abstract:
//      Definition of elliptical shape and geometry objects

class CEllipse final : public CShape
{
protected:
    CEllipse(_In_ CCoreServices *pCore)
        : CShape(pCore)
    {
        m_Stretch = DirectUI::Stretch::Fill;
    }

public:
    // Creation method
    DECLARE_CREATE(CEllipse);

    // CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CEllipse>::Index;
    }

    // CShape overrides
    _Check_return_ HRESULT UpdateRenderGeometry() override;

protected:
    bool NeedsSmoothJoin() const override
    {
        return true;
    }

    _Check_return_ HRESULT MeasureOverride(_In_ XSIZEF availableSize, _Out_ XSIZEF& desiredSize) override;
    _Check_return_ HRESULT ArrangeOverride(_In_ XSIZEF finalSize, _Out_ XSIZEF& newFinalSize) override;

//-----------------------------------------------------------------------------
// D2D Methods/Fields
//-----------------------------------------------------------------------------
protected:
    bool CanStretchGeometry() const override;

};

//------------------------------------------------------------------------
//
//  Class:  CEllipseGeometry
//
//  Synopsis:
//      Elliptical geometry class
//
//------------------------------------------------------------------------

class CEllipseGeometry final : public CGeometry
{
private:
    CEllipseGeometry(_In_ CCoreServices *pCore)
        : CGeometry(pCore)
    {
        m_fSmoothJoin = TRUE;
    }

public:
    DECLARE_CREATE(CEllipseGeometry);

    // CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CEllipseGeometry>::Index;
    }

    // CGeometry overrides
    WUComp::ICompositionGeometry* GetCompositionGeometry(_In_ VisualContentRenderer* renderer) override;

public:
    XPOINTF m_ptCenter = {};
    XFLOAT  m_eRadiusX = 0.0f;
    XFLOAT  m_eRadiusY = 0.0f;

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

    _Check_return_ HRESULT HitTestFill(
        _In_ const XPOINTF& target,
        _In_opt_ const CMILMatrix* pTransform,
        _Out_ bool* pHit
        ) override;

    _Check_return_ HRESULT HitTestFill(
        _In_ const HitTestPolygon& target,
        _In_opt_ const CMILMatrix* pTransform,
        _Out_ bool* pHit
        ) override;

protected:
    _Check_return_ HRESULT VisitSinkInternal(
        _In_ IPALGeometrySink* pSink
        ) override;
};
