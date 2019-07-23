// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "AnimationManager.h"
#include "ItemsRepeater.common.h"
#include "ItemsRepeater.h"
#include <common.h>
#include <pch.h>

AnimationManager::AnimationManager(ItemsRepeater* owner) :
    m_owner(owner),
    m_animator(owner)
{
    // ItemsRepeater is not fully constructed yet. Don't interact with it.
}

void AnimationManager::OnAnimatorChanged(const winrt::ElementAnimator&  /*newAnimator*/)
{
    // While an element is hiding, we have ownership of it. We need
    // to know when its animation completes so that we give it back
    // to the view generator.
    if (m_animator)
    {
        MUX_ASSERT(m_hideAnimationCompleted.value);
        m_animator.get().HideAnimationCompleted(m_hideAnimationCompleted);
    }

    m_animator.set(newAnimator);

    if (newAnimator)
    {
        m_hideAnimationCompleted = newAnimator.HideAnimationCompleted({ this, &AnimationManager::OnHideAnimationCompleted });
    }
}

void AnimationManager::OnLayoutChanging()
{
    m_hasRecordedLayoutTransitions = true;
}

void AnimationManager::OnItemsSourceChanged(const winrt::IInspectable& /*unused*/, const winrt::NotifyCollectionChangedEventArgs& args)
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

void AnimationManager::OnElementPrepared(const winrt::UIElement& element)
{
    if (m_animator)
    {
        auto context = winrt::AnimationContext::None;
        if (m_hasRecordedAdds) context |= winrt::AnimationContext::CollectionChangeAdd;
        if (m_hasRecordedResets) context |= winrt::AnimationContext::CollectionChangeReset;
        if (m_hasRecordedLayoutTransitions) context |= winrt::AnimationContext::LayoutTransition;

        if (context != winrt::AnimationContext::None)
        {
            m_animator.get().OnElementShown(element, context);
        }
    }
}

bool AnimationManager::ClearElement(const winrt::UIElement& element)
{
    bool canClear = false;
    
    if (m_animator)
    {
        auto context = winrt::AnimationContext::None;
        if (m_hasRecordedRemoves) context |= winrt::AnimationContext::CollectionChangeRemove;
        if (m_hasRecordedResets) context |= winrt::AnimationContext::CollectionChangeReset;

        canClear =
            context != winrt::AnimationContext::None &&
            m_animator.get().HasHideAnimation(element, context);

        if (canClear)
        {
            m_animator.get().OnElementHidden(element, context);
        }
    }

    return canClear;
}

void AnimationManager::OnElementBoundsChanged(const winrt::UIElement& element, winrt::Rect oldBounds, winrt::Rect newBounds)
{
    if (m_animator)
    {
        auto context = winrt::AnimationContext::None;
        if (m_hasRecordedAdds) context |= winrt::AnimationContext::CollectionChangeAdd;
        if (m_hasRecordedRemoves) context |= winrt::AnimationContext::CollectionChangeRemove;
        if (m_hasRecordedResets) context |= winrt::AnimationContext::CollectionChangeReset;
        if (m_hasRecordedLayoutTransitions) context |= winrt::AnimationContext::LayoutTransition;

        m_animator.get().OnElementBoundsChanged(element, context, oldBounds, newBounds);
    }
}

void AnimationManager::OnOwnerArranged()
{
    m_hasRecordedAdds = false;
    m_hasRecordedRemoves = false;
    m_hasRecordedResets = false;
    m_hasRecordedLayoutTransitions = false;
}

void AnimationManager::OnHideAnimationCompleted(const winrt::ElementAnimator& /*sender*/, const winrt::UIElement& element)
{
    if (CachedVisualTreeHelpers::GetParent(element) == static_cast<winrt::DependencyObject>(*m_owner))
    {
        m_owner->ViewManager().ClearElementToElementFactory(element);

        // Invalidate arrange so that repeater can arrange this element off-screen.
        m_owner->InvalidateArrange();
    }
}