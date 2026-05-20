// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

#include <PivotStateMachine.h>
#include <PivotCommon.h>

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace Pivot {

class StateMachineCallbackImplementor
    : public xaml_controls::IPivotStateMachineCallbacks
{
public:
    bool IsHeaderItemsCarouselEnabled() const override { return true; }
    unsigned GetPivotPanelMultiplier() const override { return 1000u; }

    HRESULT SetPivotSectionOffset(DOUBLE offset) override
    {
        m_sectionOffset = offset;
        m_setPivotOffsetCalled = true;
        return S_OK;
    }

    HRESULT SetViewportOffset(DOUBLE offset, BOOLEAN animate, _Out_ bool *success) override
    {
        m_viewportOffset = offset;
        m_viewportAnimate = animate;
        m_setViewportOffsetCalled = true;
        return S_OK;
    }

    HRESULT SetPivotSectionWidth(FLOAT width) override
    {
        m_sectionWidth = width;
        m_setPivotSectionWidthCalled = true;
        return S_OK;
    }

    HRESULT SetSelectedIndex(INT32 idx, BOOLEAN updateVisual, BOOLEAN updateIndex, BOOLEAN updateItem, xaml_controls::PivotAnimationDirection animationHint) override
    {
        m_selectedIdx = idx;
        m_updateSelectedVisual = updateVisual;
        m_updateSelectedIndex = updateIndex;
        m_updateSelectedItem = updateItem;
        m_updateDirection = animationHint;
        m_setSelectedIndexCalled = true;
        return S_OK;
    }

    HRESULT StartFlyOutAnimation(DOUBLE from, _In_ DOUBLE headerOffset, bool toLeft) override
    {
        m_animateFrom = from;
        m_animateHeaderOffset = headerOffset;
        m_animateToLeft = toLeft;
        m_startFlyOutAnimationCalled = true;
        return S_OK;
    }

    HRESULT StartFlyInAnimation(DOUBLE to, bool fromLeft) override
    {
        m_animateTo = to;
        m_animateFromLeft = fromLeft;
        m_startFlyInAnimationCalled = true;
        return S_OK;
    }

    HRESULT SetParallaxRelationship(DOUBLE sectionOffset, DOUBLE sectionWidth, float viewportSize) override
    {
        m_parallaxSectionOffset = sectionOffset;
        m_parallaxSectionWidth = sectionWidth;
        m_viewportSize = viewportSize;
        m_setParallaxRelationshipCalled = true;
        return S_OK;
    }

    HRESULT SetSnappingBehavior(BOOLEAN single) override
    {
        m_snappingBehavior = single;
        m_setSnappingBehaviorCalled = true;
        return S_OK;
    }

    HRESULT SetViewportEnabled(BOOLEAN enabled) override
    {
        m_viewportEnabled = enabled;
        m_setViewportEnabledCalled = true;
        return S_OK;
    }

    HRESULT GetIsInDManipAnimation(_Out_ bool * isInDManipAnimation) override
    {
        *isInDManipAnimation = false;
        m_getIsInDManipAnimationCalled = true;
        return S_OK;
    }

    HRESULT CancelDManipAnimations() override
    {
        m_cancelDManipAnimationsCalled = true;
        return S_OK;
    }

    HRESULT UpdateScrollViewerDragDirection(xaml_controls::PivotAnimationDirection direction)
    {
        m_updateScrollViewerDragDirectionCalled = true;
        m_dragDirection = direction;
        return S_OK;
    }

    HRESULT UpdateFocusFollower()
    {
        m_updateFocusFollowerCalled = true;
        return S_OK;
    }

    StateMachineCallbackImplementor()
        : m_setPivotOffsetCalled(false)
        , m_setViewportOffsetCalled(false)
        , m_setPivotSectionWidthCalled(false)
        , m_setSelectedIndexCalled(false)
        , m_startFlyOutAnimationCalled(false)
        , m_startFlyInAnimationCalled(false)
        , m_setParallaxRelationshipCalled(false)
        , m_setSnappingBehaviorCalled(false)
        , m_setViewportEnabledCalled(false)
        , m_getIsInDManipAnimationCalled(false)
        , m_cancelDManipAnimationsCalled(false)
        , m_updateScrollViewerDragDirectionCalled(false)
        , m_updateFocusFollowerCalled(false)
        , m_sectionOffset(0.0)
        , m_viewportOffset(0.0)
        , m_viewportAnimate(FALSE)
        , m_sectionWidth(0.0f)
        , m_selectedIdx(-1)
        , m_updateSelectedVisual(FALSE)
        , m_updateSelectedIndex(FALSE)
        , m_updateSelectedItem(FALSE)
        , m_updateDirection(xaml_controls::PivotAnimationDirection_Center)
        , m_dragDirection(xaml_controls::PivotAnimationDirection_Center)
        , m_animateFrom(0.0)
        , m_animateHeaderOffset(0.0)
        , m_animateTo(0.0)
        , m_animateDuration(0.0)
        , m_animateToLeft(false)
        , m_animateFromLeft(false)
        , m_parallaxSectionOffset(0.0)
        , m_parallaxSectionWidth(0.0)
        , m_viewportSize(0.0f)
        , m_snappingBehavior(FALSE)
        , m_viewportEnabled(FALSE)
    {}

    void ClearAllFlags()
    {
        // TODO: Implement.
        VERIFY_FAIL(L"Not implemented.");
    }

    void ValidateNothingFired()
    {
        // TODO: Implement.
        VERIFY_FAIL(L"Not implemented.");
    }

    bool m_setPivotOffsetCalled;
    bool m_setViewportOffsetCalled;
    bool m_setPivotSectionWidthCalled;
    bool m_setSelectedIndexCalled;
    bool m_startFlyOutAnimationCalled;
    bool m_startFlyInAnimationCalled;
    bool m_setParallaxRelationshipCalled;
    bool m_setSnappingBehaviorCalled;
    bool m_setViewportEnabledCalled;
    bool m_getIsInDManipAnimationCalled;
    bool m_cancelDManipAnimationsCalled;
    bool m_updateScrollViewerDragDirectionCalled;
    bool m_updateFocusFollowerCalled;

    DOUBLE m_sectionOffset;

    DOUBLE m_viewportOffset;
    BOOLEAN m_viewportAnimate;

    FLOAT m_sectionWidth;

    INT32 m_selectedIdx;
    BOOLEAN m_updateSelectedVisual;
    BOOLEAN m_updateSelectedIndex;
    BOOLEAN m_updateSelectedItem;
    xaml_controls::PivotAnimationDirection m_updateDirection;
    xaml_controls::PivotAnimationDirection m_dragDirection;

    DOUBLE m_animateFrom;
    DOUBLE m_animateHeaderOffset;
    DOUBLE m_animateTo;
    DOUBLE m_animateDuration;

    bool m_animateToLeft;
    bool m_animateFromLeft;

    DOUBLE m_parallaxSectionOffset;
    DOUBLE m_parallaxSectionWidth;
    float m_viewportSize;

    BOOLEAN m_snappingBehavior;
    BOOLEAN m_viewportEnabled;
};

} } } } } }
