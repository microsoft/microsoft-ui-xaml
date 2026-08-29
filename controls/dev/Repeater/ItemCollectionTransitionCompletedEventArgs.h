// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "ItemCollectionTransitionCompletedEventArgs.g.h"

class ItemCollectionTransitionCompletedEventArgs :
    public ReferenceTracker<ItemCollectionTransitionCompletedEventArgs, winrt::implementation::ItemCollectionTransitionCompletedEventArgsT, winrt::composing, winrt::composable>
{
public:
    ItemCollectionTransitionCompletedEventArgs(winrt::ItemCollectionTransition const& transition);

    winrt::ItemCollectionTransition Transition() { return m_transition.safe_get(); }
    winrt::UIElement Element();

private:
    tracker_ref<winrt::ItemCollectionTransition> m_transition{ this };
};
