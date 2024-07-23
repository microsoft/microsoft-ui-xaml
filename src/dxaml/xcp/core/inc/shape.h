// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "framework.h"
#include "AlphaMask.h"
#include "GraphicsUtility.h"
#include <Brush.h>
#include <geometry.h>

class CCoreServices;
class HWShapeRealization;
class VisualContentRenderer;

WUComp::CompositionStrokeCap GetCompCapFromPenCap(XcpPenCap xcpCap);
WUComp::CompositionStrokeLineJoin GetCompLineJoinFromXcpLineJoin(XcpLineJoin xcpLineJoin);

class CShape : public CFrameworkElement
{
public:
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CShape>::Index;
    }

    // CUIElement overrides
    _Check_return_ HRESULT SetValue(_In_ const SetValueParams& args) override;

    void GetIndependentlyAnimatedBrushes(
        _Outptr_ CSolidColorBrush **ppFillBrush,
        _Outptr_ CSolidColorBrush **ppStrokeBrush
    ) final;

    // CShape methods
    virtual _Check_return_ HRESULT UpdateRenderGeometry()
    {
        return E_NOTIMPL;
    }

    _Check_return_ HRESULT static GetShapeGeometryTransform(
            _In_ CDependencyObject *pObject,
            _In_ XUINT32 cArgs,
            _In_reads_(cArgs) CValue *pArgs,
            _In_opt_ IInspectable* pValueOuter,
            _Out_ CValue *pResult
            );

    XCP_FORCEINLINE CGeometry* GetRenderGeometry() const { return m_pRender; }

    static void NWSetFillBrushDirty(
        _In_ CDependencyObject *pTarget,
        DirtyFlags flags
        );

    static void NWSetStrokeBrushDirty(
        _In_ CDependencyObject *pTarget,
        DirtyFlags flags
        );

    static void UpdateRectangleBoundsForStretchMode(
        DirectUI::Stretch stretchMode,
        _Inout_ XRECTF& rectBounds
        );

    float GetStrokeMiterLimit() const;
    float GetStrokeThickness() const;
    XcpPenCap GetStrokeStartLineCap() const;
    XcpPenCap GetStrokeEndLineCap() const;
    XcpLineJoin GetStrokeLineJoin() const;
    XcpPenCap GetStrokeDashCap() const;
    float GetStrokeDashOffset() const;

    float GetAbsStrokeThickness() const;

    // FocusRectangle incorrectly sets these properties from the RenderWalk as part of ListViewItemChrome.
    // We can't perform a normal property set in this case as doing so trigger the render changed handler
    // and ASSERTs on a bad flag propagation. To get the best of both worlds we implement these properties
    // as method-based properties, where in normal CShape types they are sprase, but FocusRectangle implements
    // them as member-backed properties where they can be modified out-of-band without worry, both avoiding
    // the Enter/Leave and typical effective object state update paths.
    virtual bool HasStrokeDashArray() const;
    virtual xref_ptr<CDoubleCollection> GetStrokeDashArray() const;
    virtual _Check_return_ HRESULT SetStrokeDashArray(xref_ptr<CDoubleCollection> brush);

    virtual xref_ptr<CBrush> GetStroke() const;
    virtual _Check_return_ HRESULT SetStroke(xref_ptr<CBrush> brush);

    _Check_return_ HRESULT static Stroke(
        _In_ CDependencyObject* obj,
        _In_ UINT32 numberOfArgs,
        _Inout_updates_(numberOfArgs) CValue* args,
        _In_opt_ IInspectable* valueOuter,
        _Out_ CValue* result);

    _Check_return_ HRESULT static StrokeDashArray(
        _In_ CDependencyObject* obj,
        _In_ UINT32 numberOfArgs,
        _Inout_updates_(numberOfArgs) CValue* args,
        _In_opt_ IInspectable* valueOuter,
        _Out_ CValue* result);

    _Check_return_ HRESULT NotifyRenderContent(
        HWRenderVisibility visibility);

    _Check_return_ HRESULT GetAlphaMask(
        _Outptr_ WUComp::ICompositionBrush** ppReturnValue);

    void CleanupDeviceRelatedResourcesRecursive(
        _In_ bool cleanupDComp) override;

    void ClearPCRenderData() override;

protected:
    CShape(_In_ CCoreServices *pCore)
        : CFrameworkElement(pCore)
        , m_pRender(nullptr)
        , m_pFill(nullptr)
        , m_Stretch(DirectUI::Stretch::None)
        , m_fIsDefaultWidth(true)
        , m_fIsDefaultHeight(true)
        , m_fFillBrushDirty(false)
        , m_fStrokeBrushDirty(false)
        , m_geometryBounds()
        , m_geometryBoundsDirty(false)
        , m_innerFillBounds()
        , m_innerStrokeBounds()
    {
    }

    ~CShape() override;

    _Check_return_ HRESULT InitPenFromShape(_Out_ CPlainPen *pPen);
    _Check_return_ HRESULT MeasureOverride(_In_ XSIZEF availableSize, _Inout_ XSIZEF &desiredSize) override;
    _Check_return_ HRESULT ArrangeOverride(_In_ XSIZEF finalSize, _Inout_ XSIZEF& newFinalSize) override;
    virtual bool NeedsSmoothJoin() const
    {
        return false; // Shapes don't use smooth join by default.  Ellipse overrides and do need a smooth join.
    }

    void ReplaceRenderGeometry(CGeometry * pRender)
    {
        if (m_pRender != pRender)
        {
            ReplaceInterface(m_pRender, pRender);
        }
    }

    void EnsureContentRenderDataVirtual(
        RenderWalkType oldType,
        RenderWalkType newType
        ) final;

    _Check_return_ HRESULT GetBoundsForImageBrushVirtual(
        _In_ const CImageBrush *pImageBrush,
        _Out_ XRECTF *pBounds
        ) final;

public:
    bool NWIsContentDirty() final
    {
        return m_fFillBrushDirty
            || m_fStrokeBrushDirty
            || CFrameworkElement::NWIsContentDirty();
    }

    bool IsMaskDirty(
        _In_ HWShapeRealization *pHwShapeRealization,
        const bool renderCollapsedMask,
        bool isFillBrushAnimated,
        bool isStrokeBrushAnimated,
        _Inout_ bool* pIsFillForHitTestOnly,
        _Inout_ bool* pIsStrokeForHitTestOnly
        ) final;

protected:
    _Check_return_ bool NWCanApplyOpacityWithoutLayer() final
    {
        // A CShape with just a fill or just a stroke can optimize by adding transparent
        // geometries directly to the vector buffer without using a layer. But if it has both,
        // it must render into a layer, otherwise the area where the stroke overlaps with the
        // fill will have the incorrect opacity.
        return m_pFill == nullptr || !GetStroke();
    }

    void NWCleanDirtyFlags() final
    {
        m_fFillBrushDirty = m_fStrokeBrushDirty = FALSE;
        CFrameworkElement::NWCleanDirtyFlags();
    }

    XRECTF GetOutlineRect(
        bool fCheckDegenerateStroke
        );

private:
    _Check_return_ HRESULT GetStretchTransform(_Out_ CMILMatrix *pMatrix);

public:
    CBrush *m_pFill;
    CGeometry *m_pRender;
    AlphaMask m_alphaMask;
    DirectUI::Stretch m_Stretch;

private:
    bool m_fIsDefaultWidth   : 1;
    bool m_fIsDefaultHeight  : 1;
    bool m_fFillBrushDirty   : 1;
    bool m_fStrokeBrushDirty : 1;
    bool m_geometryBoundsDirty : 1;
    bool m_geometryWidenedBoundsDirty : 1;

protected:
    virtual bool CanStretchGeometry() const;

    void OnContentDirty() override;

private:
    _Check_return_ HRESULT CreateStrokeStyle(
        _In_ IPALAcceleratedGraphicsFactory *pD2DFactory,
        _Inout_ IPALStrokeStyle **ppStrokeStyle
        );

protected:
    _Check_return_ HRESULT PreChildrenPrintVirtual(
        _In_ const SharedRenderParams& sharedPrintParams,
        _In_ const D2DPrecomputeParams& cp,
        _In_ const D2DRenderParams &printParams
        ) final;

//-----------------------------------------------------------------------------
//
//  Bounds and Hit Testing
//
//-----------------------------------------------------------------------------
public:
    bool ShouldRender(
        _Out_opt_ bool *pHasElementBounds,
        _Out_opt_ XRECTF *pElementBounds
        );

    static void NWSetGeometryDirty(
        _In_ CDependencyObject *pTarget,
        DirtyFlags flags
        );

    _Check_return_ HRESULT GetFillBounds(
        _Out_ XRECTF_RB* pBounds
        );

    _Check_return_ HRESULT GetStrokeBounds(
        _Out_ XRECTF_RB* pBounds
        );

protected:
    _Check_return_ HRESULT GenerateContentBounds(
        _Out_ XRECTF_RB* pBounds
        ) final;

    _Check_return_ HRESULT HitTestLocalInternal(
        _In_ const XPOINTF& target,
        _Out_ bool* pHit
        ) final;

    _Check_return_ HRESULT HitTestLocalInternal(
        _In_ const HitTestPolygon& target,
        _Out_ bool* pHit
        ) final;

    _Check_return_ HRESULT GetGeometryBounds(
        _Out_ XRECTF_RB* pBounds
        );

    _Check_return_ HRESULT GetWidenedGeometryBounds(
        _Out_ XRECTF_RB* pBounds
        );

    void InvalidateGeometryBounds(
        );

private:
    template<typename HitType>
    _Check_return_ HRESULT HitTestLocalInternalImpl(
        _In_ const HitType& target,
        _Out_ bool* pHit
        );

    // Natural bounds of the geometry, not including the stretch
    XRECTF_RB m_geometryBounds;
    XRECTF_RB m_geometryWidenedBounds{};

    // Bounds of the fill & stroke, including the stretch. Used to realize brush transforms in PC.
    XRECTF_RB m_innerFillBounds;
    XRECTF_RB m_innerStrokeBounds;

    static _Check_return_ HRESULT ApplyContentStretch(
        XFLOAT strokeThickness,
        DirectUI::Stretch stretchMode,
        _In_ const XRECTF_RB& geometryNaturalBounds,
        _In_ const XSIZEF& renderConstraintBounds,
        _Out_ XSIZEF* pStretchedBounds
        );

    _Check_return_ HRESULT GenerateGeometryBounds(
        _In_ const XSIZEF* pContraintBounds,
        DirectUI::Stretch stretchMode,
        _Out_ XRECTF_RB* pBounds
        );

//-----------------------------------------------------------------------------
//
//  Shape WUC API
//
//-----------------------------------------------------------------------------
public:
    _Check_return_ HRESULT Render(
        _In_ VisualContentRenderer* renderer,
        _Outptr_ WUComp::ICompositionSpriteShape** ppSprite);

private:
    wrl::ComPtr<WUComp::ICompositionSpriteShape> InitializeSprite(_In_ VisualContentRenderer* renderer);

    void PopulateCompositionSpriteShape(
        _In_ VisualContentRenderer* renderer,
        _Inout_ WUComp::ICompositionSpriteShape* sprite);
};
