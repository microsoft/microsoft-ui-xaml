// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Elements {
    class UIElementFlagTests;
} } } } } }

//---------------------------------------------------------------------------------------------------------------------
//
//  Synopsis:
//      An element that renders with another element's cache. This is particularly useful in combination with calling
//      SetHiddenForLayoutTransition on the target element, which allows for this element to visually 'replace' its
//      target in the tree, but in another location.
//
//  Limitations:
//      - This element will only render if its target element is cached or is hidden for a layout transition, and as such
//        will only work when hardware-acceleration is enabled (which is required for caching).
//      - This element must appear in the tree in front of its target to ensure the target's cache is updated first, or
//        this element may not render or may render with stale content.
//
//---------------------------------------------------------------------------------------------------------------------
class CLayoutTransitionElement final : public CUIElement
{
    friend class HWWalk;
    friend class Microsoft::UI::Xaml::Tests::Foundation::Elements::UIElementFlagTests;

private:
    CLayoutTransitionElement(
        _In_ CUIElement *pTargetUIE,
        bool isAbsolutelyPositioned
        );

    ~CLayoutTransitionElement() override;

public:
    static _Check_return_ HRESULT Create(
        _In_ CUIElement *pTargetUIE,
        bool isAbsolutelyPositioned,
        _Outptr_ CLayoutTransitionElement **ppLTE
        );

    _Check_return_ HRESULT Initialize();

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CLayoutTransitionElement>::Index;
    }

    void AttachTransition(_In_ CTransitionRoot *parent);
    void DetachTransition();

    _Check_return_ HRESULT AttachTransition(
        _In_ CUIElement *pTarget,
        _In_ CTransitionRoot *pParent);

    _Check_return_ HRESULT DetachTransition(
        _In_opt_ CUIElement *pTarget,
        _In_ CTransitionRoot *pParent);

    CUIElement* GetTargetElement() const { return m_pTarget; }

    // Returns whether or not the element is absolutely positioned (i.e., ignores parent transforms).
    // This is currently only used for ListView's and GridView's drag and drop.
    // If absolutely positioned renderers are to be used for a new scenario, certain assumptions in DragDropVisual
    // must be updated (specifically, IsRendererActive at the time of this comment's writing).
    bool IsAbsolutelyPositioned() const { return m_isAbsolutelyPositioned; }

    _Check_return_ XFLOAT GetActualOffsetX() override;
    _Check_return_ XFLOAT GetActualOffsetY() override;
    _Check_return_ XFLOAT GetActualWidth() override { return m_pTarget->GetActualWidth(); }
    _Check_return_ XFLOAT GetActualHeight() override { return m_pTarget->GetActualHeight(); }

    void GetShouldFlipRTL(
        _Out_ bool *pShouldFlipRTL,
        _Out_ bool *pShouldFlipRTLInPlace
        ) override;

    // calculates the position relative to parent (transform + offset)
    _Check_return_ XPOINTF GetPositionRelativeToParent();
    void SetDestinationOffset(_In_ XPOINTF offset) { m_destinationOffset = offset; }

    void ClearPCRenderData() override;

    bool IsPrimaryTransitionForTarget() const { return m_isPrimaryTransition; }
    void SetIsPrimaryTransitionForTarget(bool isPrimaryTransition) { m_isPrimaryTransition = isPrimaryTransition; }

    PCRenderDataList* GetSecondaryTransitionRenderDataNoRef();
    void ClearSecondaryRenderData();

    void CleanupDeviceRelatedResourcesRecursive(_In_ bool cleanupDComp) final;

    bool IsIndependentlyAnimating() const { return IsTransformIndependentlyAnimating() || IsProjectionIndependentlyAnimating() || IsTransform3DIndependentlyAnimating(); }

protected:
    _Check_return_ HRESULT GenerateChildOuterBounds(
        _In_opt_ HitTestParams *hitTestParams,
        _Out_ XRECTF_RB* pBounds
        ) override;

    _Check_return_ HRESULT BoundsTestChildren(
        _In_ const XPOINTF& target,
        _In_ CBoundedHitTestVisitor* pCallback,
        _In_opt_ const HitTestParams *hitTestParams,
        _In_ bool canHitDisabledElements,
        _In_ bool canHitInvisibleElements,
        _Out_opt_ BoundsWalkHitResult* pResult
        ) final;

    _Check_return_ HRESULT BoundsTestChildren(
        _In_ const HitTestPolygon& target,
        _In_ CBoundedHitTestVisitor* pCallback,
        _In_opt_ const HitTestParams *hitTestParams,
        _In_ bool canHitDisabledElements,
        _In_ bool canHitInvisibleElements,
        _Out_opt_ BoundsWalkHitResult* pResult
        ) final;

    bool IgnoresInheritedClips() override { return true; }

private:
    void LeavePCSceneSubgraph() final;

private:
    CUIElement *m_pTarget;
    XPOINTF m_destinationOffset;    // the offset of this element. Transform is based on this location

    bool m_isAbsolutelyPositioned : 1;
    bool m_isPrimaryTransition    : 1;
    bool m_isRegisteredOnCore     : 1;

    PCRenderDataList *m_pSecondaryPathRenderData;
};
