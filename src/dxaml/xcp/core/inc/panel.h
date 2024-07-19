// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Framework.h>

//------------------------------------------------------------------------
//
//  Class:  CPanel
//
//  Synopsis:
//      Base class for all panel elements
//
//------------------------------------------------------------------------
class CPanel : public CFrameworkElement
{
protected:
    CPanel(_In_ CCoreServices *pCore);
    ~CPanel() override;

    bool CanHaveChildren() const final { return true; }

public:
    DECLARE_CREATE(CPanel);

    // CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CPanel>::Index;
    }

    bool GetIsIgnoringTransitions() const;
    bool GetIsItemsHost() const;

    bool GetIsLayoutElement() const override { return true; }

    bool AreChildrenInLogicalTree() final { return true; }

    XUINT32 ParticipatesInManagedTreeInternal() override
    {
        // Panel peers have state
        return PARTICIPATES_IN_MANAGED_TREE;
    }

    _Check_return_ HRESULT SetValue(_In_ const SetValueParams& args) override;

    static void NWSetBackgroundDirty(
        _In_ CDependencyObject *pTarget,
        DirtyFlags flags
        );


    _Check_return_ static HRESULT EnsureTransitionStorageForChildren(_In_ CPanel* pPanel);

    static void NWSetBorderBrushDirty(_In_ CDependencyObject *pTarget, DirtyFlags Flags);

    // naive/slow implementation for all panels to get to a closest index, given a point
    // it is intended that virtualizing panels or panels that would know how to do better
    // (given their knowledge of how they arrange) would implement IItemLookup.
    // This method is used as a fallback during semanticzoom operations.
    _Check_return_ HRESULT PanelGetClosestIndexSlow(
        _In_ XPOINTF location,
        _Out_ XINT32* pReturnValue);

    bool IsMaskDirty(
        _In_ HWShapeRealization *pHwShapeRealization,
        const bool renderCollapsedMask,
        bool isFillBrushAnimated,
        bool isStrokeBrushAnimated,
        _Out_ bool* pIsFillForHitTestOnly,
        _Out_ bool* pIsStrokeForHitTestOnly
        ) final;

    xref_ptr<CBrush> GetBackgroundBrush() const final;

    xref_ptr<CBrush> GetBorderBrush() const override;

    XTHICKNESS GetBorderThickness() const override;

    XCORNERRADIUS GetCornerRadius() const override;

protected:
    _Check_return_ CTransitionCollection* GetTransitionsForChildElementNoAddRef(_In_ CUIElement* pChild) override;

    bool NWIsContentDirty() final
    {
        return m_fNWBackgroundDirty || m_fNWBorderBrushDirty || CFrameworkElement::NWIsContentDirty();
    }

    void NWCleanDirtyFlags() final
    {
        m_fNWBackgroundDirty = FALSE;
        m_fNWBorderBrushDirty = FALSE;
        CFrameworkElement::NWCleanDirtyFlags();
    }

//-----------------------------------------------------------------------------
// Printing Methods
//-----------------------------------------------------------------------------
protected:
    _Check_return_ HRESULT PreChildrenPrintVirtual(
        _In_ const SharedRenderParams& sharedPrintParams,
        _In_ const D2DPrecomputeParams& cp,
        _In_ const D2DRenderParams &printParams
        ) final;

    _Check_return_ HRESULT GenerateContentBounds(
        _Out_ XRECTF_RB* pBounds
        ) override;

    _Check_return_ HRESULT GenerateContentBoundsImpl(
        _Out_ XRECTF_RB* pBounds
        );

    _Check_return_ HRESULT HitTestLocalInternal(
        _In_ const XPOINTF& target,
        _Out_ bool* pHit
        ) override;

    _Check_return_ HRESULT HitTestLocalInternal(
        _In_ const HitTestPolygon& target,
        _Out_ bool* pHit
        ) override;

public:
    // CPanel fields

    CBrush     *m_pBackground;  // Control.Background

private:
    bool m_fNWBackgroundDirty : 1;
    bool m_fNWBorderBrushDirty : 1;
};
