// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ElementAnimator.g.h"

#include "ElementAnimator.properties.h"

// Given some elements and their animation context, ElementAnimator
// animates them (show, hide and bounds change) and ensures the timing
// is correct (hide -> bounds change -> show).
// It's possible to customize the animations by inheriting from ElementAnimator
// and overriding virtual/abstract members.
class ElementAnimator :
    public ReferenceTracker<ElementAnimator, winrt::implementation::ElementAnimatorT, winrt::composing>,
    public ElementAnimatorProperties
{    
    struct ElementInfo;

public:
#pragma region IElementAnimator

    void OnElementShown(
        winrt::UIElement const& element,
        winrt::AnimationContext const& context);

    void OnElementHidden(
        winrt::UIElement const& element,
        winrt::AnimationContext const& context);

    void OnElementBoundsChanged(
        winrt::UIElement const& element,
        winrt::AnimationContext const& context,
        winrt::Rect const& oldBounds,
        winrt::Rect const& newBounds);

    bool HasShowAnimation(
        winrt::UIElement const& element,
        winrt::AnimationContext const& context);

    bool HasHideAnimation(
        winrt::UIElement const& element,
        winrt::AnimationContext const& context);

    bool HasBoundsChangeAnimation(
        winrt::UIElement const& element,
        winrt::AnimationContext const& context,
        winrt::Rect const& oldBounds,
        winrt::Rect const& newBounds);

#pragma endregion

#pragma region IElementAnimatorOverrides

    bool HasShowAnimationCore(
        winrt::UIElement const& element,
        winrt::AnimationContext const& context);

    bool HasHideAnimationCore(
        winrt::UIElement const& element,
        winrt::AnimationContext const& context);

    bool HasBoundsChangeAnimationCore(
        winrt::UIElement const& element,
        winrt::AnimationContext const& context,
        winrt::Rect const& oldBounds,
        winrt::Rect const& newBounds);

    void StartShowAnimation(
        winrt::UIElement const& element,
        winrt::AnimationContext const& context);

    void StartHideAnimation(
        winrt::UIElement const& element,
        winrt::AnimationContext const& context);

    void StartBoundsChangeAnimation(
        winrt::UIElement const& element,
        winrt::AnimationContext const& context,
        winrt::Rect const& oldBounds,
        winrt::Rect const& newBounds);

#pragma endregion

#pragma region IElementAnimatorProtected

    bool HasShowAnimationsPending();

    bool HasHideAnimationsPending();

    bool HasBoundsChangeAnimationsPending();

    winrt::AnimationContext SharedContext();

    void OnShowAnimationCompleted(winrt::UIElement const& element);

    void OnHideAnimationCompleted(winrt::UIElement const& element);

    void OnBoundsChangeAnimationCompleted(winrt::UIElement const& element);

#pragma endregion

private:
    void QueueElementForAnimation(ElementInfo elementInfo);
    void OnRendering(const winrt::IInspectable& sender, const winrt::IInspectable& args);
    void ResetState();

    enum class AnimationTrigger
    {
        Show,
        Hide,
        BoundsChange
    };

    struct ElementInfo
    {
        ElementInfo(
            const ITrackerHandleManager* owner,
            const winrt::UIElement& element,
            AnimationTrigger trigger,
            winrt::AnimationContext context) :
            m_element(owner, element),
            m_trigger(trigger),
            m_context(context)
        {
            MUX_ASSERT(trigger != AnimationTrigger::BoundsChange);
        }

        ElementInfo(
            const ITrackerHandleManager* owner,
            const winrt::UIElement& element,
            AnimationTrigger trigger,
            winrt::AnimationContext context,
            winrt::Rect oldBounds,
            winrt::Rect newBounds) :
            m_element(owner, element),
            m_trigger(trigger),
            m_context(context),
            m_oldBounds(oldBounds),
            m_newBounds(newBounds)
        {
            MUX_ASSERT(trigger == AnimationTrigger::BoundsChange);
        }

        winrt::UIElement Element() const { return m_element.get(); }
        AnimationTrigger Trigger() const { return m_trigger; }
        winrt::AnimationContext Context() const { return m_context; }
        winrt::Rect OldBounds() const { return m_oldBounds; }
        winrt::Rect NewBounds() const { return m_newBounds; }

    private:
        tracker_ref<winrt::UIElement> m_element;
        AnimationTrigger m_trigger{};
        winrt::AnimationContext m_context{};
        // Valid for Trigger == BoundsChange
        winrt::Rect m_oldBounds{};
        winrt::Rect m_newBounds{};
    };

    std::vector<ElementInfo> m_animatingElements;
    bool m_hasShowAnimationsPending{};
    bool m_hasHideAnimationsPending{};
    bool m_hasBoundsChangeAnimationsPending{};
    winrt::AnimationContext m_sharedContext{};

    // Event tokens.
    winrt::Windows::UI::Xaml::Media::CompositionTarget::Rendering_revoker m_rendering{};
};
