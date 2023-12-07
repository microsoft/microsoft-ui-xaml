// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "ItemCollectionTransitionProvider.h"
#include "LinedFlowLayoutItemCollectionTransitionProvider.g.h"

const uint32_t DebugSlowDownFactor = 1;

#ifndef IS_LAB_BUILD
static_assert(DebugSlowDownFactor >= 1, "DebugSlowDownFactor must be at least 1.");
#else
static_assert(DebugSlowDownFactor == 1, "DebugSlowDownFactor must be set back to 1 before a pull request can be completed.");
#endif

class LinedFlowLayoutItemCollectionTransitionProvider :
    public winrt::implementation::LinedFlowLayoutItemCollectionTransitionProviderT<LinedFlowLayoutItemCollectionTransitionProvider, ItemCollectionTransitionProvider>
{
public:
    ForwardRefToBaseReferenceTracker(ItemCollectionTransitionProvider)

#pragma region IItemCollectionTransitionProviderOverrides

    void StartTransitions(winrt::IVector<winrt::ItemCollectionTransition> const& transitions);

    bool ShouldAnimateCore(winrt::ItemCollectionTransition const& transition);

#pragma endregion

private:
    void StartAddTransitions(std::vector<winrt::ItemCollectionTransition> const& transitions, bool hasRemoveTransitions, bool hasMoveTransitions);
    void StartRemoveTransitions(std::vector<winrt::ItemCollectionTransition> const& transitions, bool hasAddAnimations);
    void StartMoveTransitions(std::vector<winrt::ItemCollectionTransition> const& transitions, bool hasRemoveTransitions);

    void SlideIn(winrt::CompositionAnimationGroup const& group, winrt::Compositor const& compositor, std::chrono::milliseconds const& delay);
    void ScaleIn(winrt::CompositionAnimationGroup const& group, winrt::Compositor const& compositor, std::chrono::milliseconds const& delay, bool presetInitialValue = false);
    void ScaleOut(winrt::CompositionAnimationGroup const& group, winrt::Compositor const& compositor, std::chrono::milliseconds const& delay, bool presetInitialValue = false);
    void Translate(winrt::CompositionAnimationGroup const& group, winrt::Compositor const& compositor, winrt::Rect const& from, winrt::Rect const& to, std::chrono::milliseconds const& delay, bool presetInitialValue = false);

    static winrt::float3 GetCenterPoint(winrt::UIElement const& element);
    winrt::CubicBezierEasingFunction GetEaseInFunction(winrt::Compositor const& compositor);
    winrt::CubicBezierEasingFunction GetEaseOutFunction(winrt::Compositor const& compositor);
    void StartAnimationGroup(winrt::Visual const& visual, winrt::CompositionAnimationGroup const& animationGroup);

    const float Epsilon = 0.001f;

    const std::chrono::milliseconds QuickAnimationDuration = DebugSlowDownFactor * 167ms;
    const std::chrono::milliseconds DefaultAnimationDuration = DebugSlowDownFactor * 250ms;

    const float SlideInDistance = 100.0f;

    const winrt::float3 ScaleVisible{ 1.0f, 1.0f, 1.0f };
    const winrt::float3 ScaleCollapsed{ 0.0f, 0.0f, 1.0f };
        
    winrt::IMap<winrt::Visual, winrt::CompositionAnimationGroup> m_currentAnimationGroupByVisual{ winrt::single_threaded_map<winrt::Visual, winrt::CompositionAnimationGroup>() };

    winrt::CubicBezierEasingFunction m_easeInFunction{ nullptr };
    winrt::CubicBezierEasingFunction m_easeOutFunction{ nullptr };
};
