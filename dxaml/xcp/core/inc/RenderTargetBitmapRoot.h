// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class HitTestPolygon;
#include "xmap.h"

//------------------------------------------------------------------------
//
//  Class:  CRenderTargetBitmapRoot
//
//  Synopsis: Used to hook up the disconnected elements needed for RenderTargetBitmaps
//
//------------------------------------------------------------------------
class CRenderTargetBitmapRoot final : public CPanel
{
    friend class HWWalk;

private:

    CRenderTargetBitmapRoot(_In_ CCoreServices *pCore)
        : CPanel(pCore)
        , m_pMapUIElementNoRefToAttachCount(NULL)
    {
    }

protected:
    _Check_return_ HRESULT MeasureOverride(XSIZEF availableSize, XSIZEF& desiredSize) override;

    _Check_return_ HRESULT ArrangeOverride(XSIZEF finalSize, XSIZEF& newFinalSize) override;

    ~CRenderTargetBitmapRoot() override;

    _Check_return_ HRESULT PrintChildren(
        _In_ const SharedRenderParams& sharedPrintParams,
        _In_ const D2DPrecomputeParams& cp,
        _In_ const D2DRenderParams& printParams
        ) override;

public:

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CRenderTargetBitmapRoot>::Index;
    }

    DECLARE_CREATE(CRenderTargetBitmapRoot);

    bool GetIsLayoutElement() const final { return true; }

    bool SkipNameRegistrationForChildren() override {  return true; }

    _Check_return_ HRESULT AttachElement(_In_ CUIElement *pElement);
    _Check_return_ HRESULT DetachElement(_In_ CUIElement *pElement);
    bool IsElementAttached(_In_ CUIElement *pElement);

    //-----------------------------------------------------------------------------
    //
    //  Bounds and Hit Testing
    //
    //-----------------------------------------------------------------------------

protected:
    virtual _Check_return_ HRESULT BoundsTestChildren(
        _In_ const XPOINTF& target,
        _In_ CBoundedHitTestVisitor* pCallback,
        _Out_opt_ BoundsWalkHitResult* pResult,
        bool canHitDisabledElements
        );

    virtual _Check_return_ HRESULT BoundsTestChildren(
        _In_ const HitTestPolygon& target,
        _In_ CBoundedHitTestVisitor* pCallback,
        _Out_opt_ BoundsWalkHitResult* pResult,
        bool canHitDisabledElements
        );

    _Check_return_ HRESULT HitTestLocalInternal(
        _In_ const XPOINTF& target,
        _Out_ bool* pHit
        ) override;

    _Check_return_ HRESULT HitTestLocalInternal(
        _In_ const HitTestPolygon& target,
        _Out_ bool* pHit
        ) override;

private:
    xchainedmap<CUIElement*, XUINT16>* m_pMapUIElementNoRefToAttachCount;
};

