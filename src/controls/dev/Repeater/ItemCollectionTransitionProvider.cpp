// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include "ItemsRepeater.common.h"
#include "ItemCollectionTransitionProvider.h"
#include "ItemCollectionTransitionCompletedEventArgs.h"
#include "ItemCollectionTransitionProvider.properties.cpp"

#pragma region IItemCollectionTransitionProvider

void ItemCollectionTransitionProvider::QueueTransition(winrt::ItemCollectionTransition const& transition)
{
    // We'll animate if animations are enabled and the transition provider has indicated we should animate this transition.
    if (SharedHelpers::IsAnimationsEnabled() && ShouldAnimate(transition))
    {
        m_transitionsWithAnimations.Append(transition);
    }

    // To ensure proper VirtualizationInfo ordering, we still need to raise TransitionCompleted in a
    // CompositionTarget.Rendering handler, even if we aren't going to animate anything for this transition.
    m_transitions.Append(transition);

    if (!m_rendering)
    {
        m_rendering = winrt::CompositionTarget::Rendering(winrt::auto_revoke, { this, &ItemCollectionTransitionProvider::OnRendering });
    }
}

bool ItemCollectionTransitionProvider::ShouldAnimate(winrt::ItemCollectionTransition const& transition)
{
    return overridable().ShouldAnimateCore(transition);
}

#pragma endregion

#pragma region IItemCollectionTransitionProviderOverrides

void ItemCollectionTransitionProvider::StartTransitions(winrt::IVector<winrt::ItemCollectionTransition> const& /* transitions */)
{
    throw winrt::hresult_not_implemented();
}

bool ItemCollectionTransitionProvider::ShouldAnimateCore(winrt::ItemCollectionTransition const& /* transition */)
{
    throw winrt::hresult_not_implemented();
}

#pragma endregion

void ItemCollectionTransitionProvider::NotifyTransitionCompleted(winrt::ItemCollectionTransition const& transition)
{
    m_transitionCompletedEventSource(*this, winrt::make<ItemCollectionTransitionCompletedEventArgs>(transition));
}

void ItemCollectionTransitionProvider::OnRendering(winrt::IInspectable const& /*sender*/, winrt::IInspectable const& /*args*/)
{
    m_rendering.revoke();

    auto resetState = gsl::finally([this]()
    {
        ResetState();
    });

    overridable().StartTransitions(m_transitionsWithAnimations);

    // We'll automatically raise TransitionCompleted on all of the transitions that were not actually animated
    // in order to guarantee that every transition queued receives a corresponding TransitionCompleted event.
    for (auto const& transition : m_transitions)
    {
        if (!transition.HasStarted())
        {
            NotifyTransitionCompleted(transition);
        }
    }
}

void ItemCollectionTransitionProvider::ResetState()
{
    m_transitionsWithAnimations.Clear();
    m_transitions.Clear();
}
