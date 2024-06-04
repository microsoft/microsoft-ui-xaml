// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "ItemCollectionTransition.g.h"

class ItemCollectionTransition :
    public ReferenceTracker<ItemCollectionTransition, winrt::implementation::ItemCollectionTransitionT, winrt::composing, winrt::composable>
{
public:
    ItemCollectionTransition(
        winrt::ItemCollectionTransitionProvider const& owningProvider,
        winrt::UIElement const& element,
        winrt::ItemCollectionTransitionOperation const& operation,
        winrt::ItemCollectionTransitionTriggers const& triggers);

    ItemCollectionTransition(
        winrt::ItemCollectionTransitionProvider const& owningProvider,
        winrt::UIElement const& element,
        winrt::ItemCollectionTransitionTriggers const& triggers,
        winrt::Rect const& oldBounds,
        winrt::Rect const& newBounds);

    winrt::ItemCollectionTransitionProvider OwningProvider() { return m_owningProvider.get(); }
    winrt::UIElement Element() { return m_element.safe_get(); }
    winrt::ItemCollectionTransitionOperation Operation() { return m_operation; }
    winrt::ItemCollectionTransitionTriggers Triggers() { return m_triggers; }
    winrt::Rect OldBounds() { return m_oldBounds; }
    winrt::Rect NewBounds() { return m_newBounds; }
    bool HasStarted() { return static_cast<bool>(m_progress); }

    winrt::ItemCollectionTransitionProgress Start();

private:
    ItemCollectionTransition(
        winrt::ItemCollectionTransitionProvider const& owningProvider,
        winrt::UIElement const& element,
        winrt::ItemCollectionTransitionOperation const& operation,
        winrt::ItemCollectionTransitionTriggers const& triggers,
        winrt::Rect const& oldBounds,
        winrt::Rect const& newBounds);

    winrt::weak_ref<winrt::ItemCollectionTransitionProvider> m_owningProvider{ nullptr };
    tracker_ref<winrt::UIElement> m_element{ this };
    winrt::ItemCollectionTransitionOperation m_operation{};
    winrt::ItemCollectionTransitionTriggers m_triggers{};
    winrt::Rect m_oldBounds{};
    winrt::Rect m_newBounds{};
    tracker_ref<winrt::ItemCollectionTransitionProgress> m_progress{ this };
};
