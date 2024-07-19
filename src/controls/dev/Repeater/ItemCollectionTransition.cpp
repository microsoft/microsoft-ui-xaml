// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ItemCollectionTransition.h"
#include "ItemCollectionTransitionProgress.h"

ItemCollectionTransition::ItemCollectionTransition(
    winrt::ItemCollectionTransitionProvider const& owningProvider,
    winrt::UIElement const& element,
    winrt::ItemCollectionTransitionOperation const& operation,
    winrt::ItemCollectionTransitionTriggers const& triggers)
    : ItemCollectionTransition(
        owningProvider,
        element,
        operation,
        triggers,
        winrt::Rect{},
        winrt::Rect{})
{
    MUX_ASSERT(operation != winrt::ItemCollectionTransitionOperation::Move);
}

ItemCollectionTransition::ItemCollectionTransition(
    winrt::ItemCollectionTransitionProvider const& owningProvider,
    winrt::UIElement const& element,
    winrt::ItemCollectionTransitionTriggers const& triggers,
    winrt::Rect const& oldBounds,
    winrt::Rect const& newBounds)
    : ItemCollectionTransition(
        owningProvider,
        element,
        winrt::ItemCollectionTransitionOperation::Move,
        triggers,
        oldBounds,
        newBounds)
{
}

ItemCollectionTransition::ItemCollectionTransition(
    winrt::ItemCollectionTransitionProvider const& owningProvider,
    winrt::UIElement const& element,
    winrt::ItemCollectionTransitionOperation const& operation,
    winrt::ItemCollectionTransitionTriggers const& triggers,
    winrt::Rect const& oldBounds,
    winrt::Rect const& newBounds)
    : m_operation(operation)
    , m_triggers(triggers)
    , m_oldBounds(oldBounds)
    , m_newBounds(newBounds)
{
    m_owningProvider = owningProvider;
    m_element.set(element);
}

winrt::ItemCollectionTransitionProgress ItemCollectionTransition::Start()
{
    if (!m_progress)
    {
        m_progress.set(winrt::make<ItemCollectionTransitionProgress>(*this));
    }

    return m_progress.safe_get();
}
