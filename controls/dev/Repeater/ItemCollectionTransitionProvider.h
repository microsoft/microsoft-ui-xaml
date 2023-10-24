// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ItemCollectionTransitionProvider.g.h"
#include "ItemCollectionTransitionProvider.properties.h"

// Given some elements and their animation context, ItemCollectionTransitionProvider
// animates them (Add, Delete and Move) and ensures the timing
// is correct (Delete -> Move -> Add).
// It's possible to customize the animations by inheriting from ItemCollectionTransitionProvider
// and overriding virtual/abstract members.
class ItemCollectionTransitionProvider :
    public ReferenceTracker<ItemCollectionTransitionProvider, winrt::implementation::ItemCollectionTransitionProviderT, winrt::composing, winrt::composable>,
    public ItemCollectionTransitionProviderProperties
{    
public:
#pragma region IItemCollectionTransitionProvider

    void QueueTransition(winrt::ItemCollectionTransition const& transition);
    bool ShouldAnimate(winrt::ItemCollectionTransition const& transition);

#pragma endregion

#pragma region IItemCollectionTransitionProviderOverrides

    virtual void StartTransitions(winrt::IVector<winrt::ItemCollectionTransition> const& transitions);
    virtual bool ShouldAnimateCore(winrt::ItemCollectionTransition const& transition);

#pragma endregion

    void NotifyTransitionCompleted(winrt::ItemCollectionTransition const& transition);

private:
    void OnRendering(const winrt::IInspectable& sender, const winrt::IInspectable& args);
    void ResetState();

    winrt::IVector<winrt::ItemCollectionTransition> m_transitions{ winrt::single_threaded_vector<winrt::ItemCollectionTransition>() };
    winrt::IVector<winrt::ItemCollectionTransition> m_transitionsWithAnimations{ winrt::single_threaded_vector<winrt::ItemCollectionTransition>() };
    winrt::CompositionTarget::Rendering_revoker m_rendering{};
};
