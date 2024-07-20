// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "ItemCollectionTransitionProgress.g.h"

class ItemCollectionTransitionProgress :
    public ReferenceTracker<ItemCollectionTransitionProgress, winrt::implementation::ItemCollectionTransitionProgressT, winrt::composing, winrt::composable>
{
public:
    ItemCollectionTransitionProgress(winrt::ItemCollectionTransition transition);

    winrt::ItemCollectionTransition Transition() { return m_transition.get(); }
    winrt::UIElement Element();

    void Complete();

private:
    winrt::weak_ref<winrt::ItemCollectionTransition> m_transition{ nullptr };
};
