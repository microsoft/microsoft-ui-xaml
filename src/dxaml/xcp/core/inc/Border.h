// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      CornerRadius.
//      Border framework element.

#pragma once

#include "Framework.h"
#include "corep.h"
#include <RootScale.h>

class HitTestGeometrySink;
class HWShapeRealization;

//------------------------------------------------------------------------
//
//  Class:  CBorder
//
//  Synopsis:
//
//      Object created for <Border> tag.
//      Encompass a child with a border that has a thickness
//
//------------------------------------------------------------------------
class CBorder final : public CFrameworkElement
{
private:
    CBorder(_In_ CCoreServices *pCore)
        : CFrameworkElement(pCore)
        , m_fNWBackgroundBrushDirty(FALSE)
        , m_fNWBorderBrushDirty(FALSE)
        , m_useBackgroundOverride(FALSE)
    {
    }

   ~CBorder() override;

public:
    // Creation method
    DECLARE_CREATE(CBorder);

    // CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CBorder>::Index;
    }

    bool GetIsLayoutElement() const final { return true; }

    bool CanHaveChildren() const final { return true; }

    bool AreChildrenInLogicalTree() final { return true; }

    _Check_return_ HRESULT SetValue(_In_ const SetValueParams& args) override;

    _Check_return_ HRESULT AddChild(_In_ CUIElement *pChild) override;

    _Check_return_ CTransitionCollection* GetTransitionsForChildElementNoAddRef(_In_ CUIElement* child) override;

    // Method to set the Child property
    // using PROP_METHOD_CALL
    static _Check_return_ HRESULT Child(
        _In_ CDependencyObject *pObject,
        _In_ XUINT32 cArgs,
        _Inout_updates_(cArgs) CValue *ppArgs,
        _In_opt_ IInspectable* pValueOuter,
        _Out_ CValue *pResult);

    _Check_return_ HRESULT SetChild(_In_ CUIElement *pContent);

    _Check_return_ HRESULT GetChild(_Outptr_ CUIElement **ppContent);

    static void NWSetBackgroundBrushDirty(_In_ CDependencyObject *pTarget, DirtyFlags Flags);

    static void NWSetBorderBrushDirty(_In_ CDependencyObject *pTarget, DirtyFlags Flags);

    bool NWIsContentDirty() override
    {
        return m_fNWBackgroundBrushDirty
            || m_fNWBorderBrushDirty
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

    static void HelperDeflateRect(
        _In_ const XRECTF& rect,
        _In_ const XTHICKNESS& thickness,
        _Out_ XRECTF& innerRect
        );

    static XSIZEF HelperCollapseThickness(
        _In_ const XTHICKNESS& thickness
        );

    static XRECTF HelperGetInnerRect(
        _In_ CFrameworkElement* pElement,
        _In_ const XSIZEF& outerSize
        );

    static XSIZEF HelperGetCombinedThickness(
        _In_ CFrameworkElement* pElement
        );

    static XTHICKNESS GetLayoutRoundedThickness(
        _In_ CFrameworkElement* pElement
        );

    static bool HasNonZeroCornerRadius(_In_ const XCORNERRADIUS cornerRadius)
    {
        return (cornerRadius.topLeft > 0.0f) ||
               (cornerRadius.topRight > 0.0f) ||
               (cornerRadius.bottomRight > 0.0f) ||
               (cornerRadius.bottomLeft > 0.0f);
    }

    static bool HasNonZeroThickness(_In_ const XTHICKNESS thickness)
    {
        return (thickness.left > 0.0f)
            || (thickness.top > 0.0f)
            || (thickness.right > 0.0f)
            || (thickness.bottom > 0.0f);
    }

    template <typename HitType>
    static _Check_return_ HRESULT CreateHitTestGeometrySink(
        _In_ const HitType& target,
        XFLOAT tolerance,
        _In_opt_ const CMILMatrix* pTransform,
        _Out_ HitTestGeometrySink** ppHitTestSink
        );

    template <typename HitType>
    static _Check_return_ HRESULT DoesBorderRectIntersectHitType(
        _In_ const XRECTF& rect,
        _In_ const bool useComplexDrawing,
        _In_ const XTHICKNESS& borderThickness,
        _In_ const XCORNERRADIUS& cornerRadius,
        _In_ const HitType& target,
        _In_ const bool isOuter,
        _Out_ bool& intersects
    );

    template <typename HitType>
    static _Check_return_ HRESULT HitTestLocalInternalImpl(
        _In_ CFrameworkElement* pElement,
        _In_ const HitType& target,
        _Out_ bool* pHit
        );

    static bool HitTestRoundedCornerClip(
        _In_ CFrameworkElement* element,
        _In_ const XPOINTF& target
        );

    static bool HitTestRoundedCornerClip(
        _In_ CFrameworkElement* element,
        _In_ const HitTestPolygon& target
        );

    template <typename HitType>
    static bool HitTestRoundedCornerClipImpl(
        _In_ CFrameworkElement* element,
        _In_ const HitType& target
        );

    xref_ptr<CBrush> GetBackgroundBrush() const final;

    xref_ptr<CBrush> GetBorderBrush() const final {return xref_ptr<CBrush>(m_pBorderBrush);}
    XTHICKNESS GetBorderThickness() const final {return m_borderThickness;}
    XTHICKNESS GetPadding() const final {return m_padding;}
    XCORNERRADIUS GetCornerRadius() const final {return m_cornerRadius;}
    DirectUI::BackgroundSizing GetBackgroundSizing() const final;

    void SetHWRealizationCache(_In_opt_ HWRealization *pNewRenderingCache) override;

    static _Check_return_ HRESULT CreateForFocusRendering(_In_ CCoreServices* core, _Outptr_ CBorder** newBorder);

    void SetUseBackgroundOverride(bool useBackgroundOverride);

protected:
    _Check_return_ HRESULT MeasureOverride(
        XSIZEF availableSize,
        XSIZEF& desiredSize
        ) override;

    _Check_return_ HRESULT ArrangeOverride(
        XSIZEF finalSize,
        XSIZEF& newFinalSize
        ) override;

    void NWCleanDirtyFlags() override
    {
        m_fNWBackgroundBrushDirty = m_fNWBorderBrushDirty = FALSE;
        CFrameworkElement::NWCleanDirtyFlags();
    }

    _Check_return_ HRESULT GetBoundsForImageBrushVirtual(
        _In_ const CImageBrush *pImageBrush,
        _Out_ XRECTF *pBounds
        ) override;

    void OnContentDirty() override;

//-----------------------------------------------------------------------------
// Printing Methods
//-----------------------------------------------------------------------------
protected:
    _Check_return_ HRESULT PreChildrenPrintVirtual(
        _In_ const SharedRenderParams& sharedPrintParams,
        _In_ const D2DPrecomputeParams& cp,
        _In_ const D2DRenderParams& printParams
        ) override;

    //-----------------------------------------------------------------------------
    //
    //  Bounds and Hit Testing
    //
    //-----------------------------------------------------------------------------
protected:
    _Check_return_ HRESULT GenerateContentBounds(
        _Out_ XRECTF_RB* pBounds
        ) override;

    _Check_return_ HRESULT HitTestLocalInternal(
        _In_ const XPOINTF& target,
        _Out_ bool* pHit
        ) override;

    _Check_return_ HRESULT HitTestLocalInternal(
        _In_ const HitTestPolygon& target,
        _Out_ bool* pHit
        ) override;

public:
    // Public properties

    XTHICKNESS      m_borderThickness = {};
    XTHICKNESS      m_padding = {};
    XCORNERRADIUS   m_cornerRadius = {};
    CBrush*         m_pBorderBrush = nullptr;
    CBrush*         m_pBackgroundBrush = nullptr;

private:
    bool m_fNWBorderBrushDirty     : 1;
    bool m_fNWBackgroundBrushDirty : 1;
    bool m_useBackgroundOverride   : 1;
};
