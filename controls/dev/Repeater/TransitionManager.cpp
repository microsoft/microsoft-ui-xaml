// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include "ItemsRepeater.common.h"
#include "TransitionManager.h"
#include "ItemsRepeater.h"
#include "ItemCollectionTransition.h"

TransitionManager::TransitionManager(ItemsRepeater* owner) :
    m_owner(owner),
    m_transitionProvider(owner)
{
    // ItemsRepeater is not fully constructed yet. Don't interact with it.
}

void TransitionManager::OnTransitionProviderChanged(const winrt::ItemCollectionTransitionProvider& newTransitionProvider)
{
    // While an element is hiding, we have ownership of it. We need
    // to know when its animation completes so that we give it back
    // to the view generator.
    if (m_transitionProvider)
    {
        MUX_ASSERT(m_transitionCompleted.value);
        m_transitionProvider.get().TransitionCompleted(m_transitionCompleted);
    }

    m_transitionProvider.set(newTransitionProvider);

    if (newTransitionProvider)
    {
        m_transitionCompleted = newTransitionProvider.TransitionCompleted({ this, &TransitionManager::OnTransitionProviderTransitionCompleted });
    }
}

void TransitionManager::OnLayoutChanging()
{
    m_hasRecordedLayoutTransitions = true;
}

void TransitionManager::OnItemsSourceChanged(const winrt::IInspectable&, const winrt::NotifyCollectionChangedEventArgs& args)
{
    switch (args.Action())
    {
    case winrt::NotifyCollectionChangedAction::Add:
        m_hasRecordedAdds = true;
        break;
    case winrt::NotifyCollectionChangedAction::Remove:
        m_hasRecordedRemoves = true;
        break;
    case winrt::NotifyCollectionChangedAction::Replace:
        m_hasRecordedAdds = true;
        m_hasRecordedRemoves = true;
        break;
    case winrt::NotifyCollectionChangedAction::Reset:
        m_hasRecordedResets = true;
        break;
    }
}

void TransitionManager::OnElementPrepared(const winrt::UIElement& element)
{
    if (m_transitionProvider)
    {
        winrt::ItemCollectionTransitionTriggers triggers = static_cast<winrt::ItemCollectionTransitionTriggers>(0);
        if (m_hasRecordedAdds) triggers |= winrt::ItemCollectionTransitionTriggers::CollectionChangeAdd;
        if (m_hasRecordedResets) triggers |= winrt::ItemCollectionTransitionTriggers::CollectionChangeReset;
        if (m_hasRecordedLayoutTransitions) triggers |= winrt::ItemCollectionTransitionTriggers::LayoutTransition;

        if (triggers != static_cast<winrt::ItemCollectionTransitionTriggers>(0))
        {
            m_transitionProvider.get().QueueTransition(
                winrt::make<ItemCollectionTransition>(
                    m_transitionProvider.get(),
                    element,
                    winrt::ItemCollectionTransitionOperation::Add,
                    triggers));
        }
    }
}

bool TransitionManager::ClearElement(const winrt::UIElement& element)
{
    bool canClear = false;
    
    if (m_transitionProvider)
    {
        auto triggers = static_cast<winrt::ItemCollectionTransitionTriggers>(0);
        if (m_hasRecordedRemoves) triggers |= winrt::ItemCollectionTransitionTriggers::CollectionChangeRemove;
        if (m_hasRecordedResets) triggers |= winrt::ItemCollectionTransitionTriggers::CollectionChangeReset;

        auto transition =
            winrt::make<ItemCollectionTransition>(
                m_transitionProvider.get(),
                element,
                winrt::ItemCollectionTransitionOperation::Remove,
                triggers);

        canClear =
            triggers != static_cast<winrt::ItemCollectionTransitionTriggers>(0) &&
            m_transitionProvider.get().ShouldAnimate(transition);

        if (canClear)
        {
            m_transitionProvider.get().QueueTransition(transition);
        }
    }

    return canClear;
}

void TransitionManager::OnElementBoundsChanged(const winrt::UIElement& element, winrt::Rect oldBounds, winrt::Rect newBounds)
{
    if (m_transitionProvider)
    {
        auto triggers = static_cast<winrt::ItemCollectionTransitionTriggers>(0);
        if (m_hasRecordedAdds) triggers |= winrt::ItemCollectionTransitionTriggers::CollectionChangeAdd;
        if (m_hasRecordedRemoves) triggers |= winrt::ItemCollectionTransitionTriggers::CollectionChangeRemove;
        if (m_hasRecordedResets) triggers |= winrt::ItemCollectionTransitionTriggers::CollectionChangeReset;
        if (m_hasRecordedLayoutTransitions) triggers |= winrt::ItemCollectionTransitionTriggers::LayoutTransition;

        // A bounds change can occur during initial layout or when resizing the owning control,
        // which won't trigger an explicit layout transition but should still be treated as one.
        if (triggers == static_cast<winrt::ItemCollectionTransitionTriggers>(0))
        {
            triggers = winrt::ItemCollectionTransitionTriggers::LayoutTransition;
        }

        m_transitionProvider.get().QueueTransition(
            winrt::make<ItemCollectionTransition>(
                m_transitionProvider.get(),
                element,
                triggers,
                oldBounds,
                newBounds));
    }
}

void TransitionManager::OnOwnerArranged()
{
    m_hasRecordedAdds = false;
    m_hasRecordedRemoves = false;
    m_hasRecordedResets = false;
    m_hasRecordedLayoutTransitions = false;
}

void TransitionManager::OnTransitionProviderTransitionCompleted(const winrt::ItemCollectionTransitionProvider& /*sender*/, const winrt::ItemCollectionTransitionCompletedEventArgs& args)
{
    if (args.Transition().Operation() == winrt::ItemCollectionTransitionOperation::Remove)
    {
        auto element = args.Element();

        if (CachedVisualTreeHelpers::GetParent(element) == static_cast<winrt::DependencyObject>(*m_owner))
        {
            m_owner->ViewManager().ClearElementToElementFactory(element);

            // Invalidate arrange so that repeater can arrange this element off-screen.
            m_owner->InvalidateArrange();
        }
    }
}
