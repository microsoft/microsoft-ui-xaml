// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class ItemsRepeater;

// Internal component that contains all
// the animation related logic for ItemsRepeater.
class TransitionManager final
{
public:
    TransitionManager(ItemsRepeater* owner);

    void OnTransitionProviderChanged(const winrt::ItemCollectionTransitionProvider& newTransitionProvider);
    void OnLayoutChanging();
    void OnItemsSourceChanged(const winrt::IInspectable& source, const winrt::NotifyCollectionChangedEventArgs& args);
    void OnElementPrepared(const winrt::UIElement& element);
    bool ClearElement(const winrt::UIElement& element);
    void OnElementBoundsChanged(const winrt::UIElement& element, winrt::Rect oldBounds, winrt::Rect newBounds);
    void OnOwnerArranged();

private:
    void OnTransitionProviderTransitionCompleted(const winrt::ItemCollectionTransitionProvider& sender, const winrt::ItemCollectionTransitionCompletedEventArgs& args);

    ItemsRepeater* m_owner;
    tracker_ref<winrt::ItemCollectionTransitionProvider> m_transitionProvider;

    // We infer the animation context
    // from heuristics like whether or not
    // we observed a collection change or a
    // layout transition during the current
    // tick.
    bool m_hasRecordedAdds{};
    bool m_hasRecordedRemoves{};
    bool m_hasRecordedResets{};
    bool m_hasRecordedLayoutTransitions{};

    // Event tokens
    winrt::event_token m_transitionCompleted{};
};
