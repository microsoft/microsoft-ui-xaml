// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ItemCollectionTransitionProgress.h"
#include "ItemCollectionTransition.h"
#include "ItemCollectionTransitionProvider.h"

ItemCollectionTransitionProgress::ItemCollectionTransitionProgress(winrt::ItemCollectionTransition transition)
{
    m_transition = transition;
}

winrt::UIElement ItemCollectionTransitionProgress::Element()
{
    if (auto transition = m_transition.get())
    {
        return winrt::get_self<ItemCollectionTransition>(transition)->Element();
    }
    else
    {
        return nullptr;
    }
}

void ItemCollectionTransitionProgress::Complete()
{
    if (auto transition = m_transition.get())
    {
        auto transitionImpl = winrt::get_self<ItemCollectionTransition>(transition);
        auto transitionProvider = winrt::get_self<ItemCollectionTransitionProvider>(transitionImpl->OwningProvider());
        transitionProvider->NotifyTransitionCompleted(transition);
    }
}
