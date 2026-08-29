// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ItemCollectionTransitionCompletedEventArgs.h"
#include "ItemCollectionTransition.h"

ItemCollectionTransitionCompletedEventArgs::ItemCollectionTransitionCompletedEventArgs(winrt::ItemCollectionTransition const& transition)
{
    m_transition.set(transition);
}

winrt::UIElement ItemCollectionTransitionCompletedEventArgs::Element()
{
    return m_transition ? winrt::get_self<ItemCollectionTransition>(m_transition.safe_get())->Element() : nullptr;
}
