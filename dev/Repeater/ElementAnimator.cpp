// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include "ItemsRepeater.common.h"
#include "ElementAnimator.h"

#include "ElementAnimator.properties.cpp"

#pragma region IElementAnimator

void ElementAnimator::OnElementShown(
    winrt::UIElement const& element,
    winrt::AnimationContext const& context)
{
    if (HasShowAnimation(element, static_cast<winrt::AnimationContext>(context)))
    {
        m_hasShowAnimationsPending = true;
        m_sharedContext |= context;
        QueueElementForAnimation(ElementInfo(
            this /* owner */,
            element,
            AnimationTrigger::Show,
            context));
    }
}

void ElementAnimator::OnElementHidden(
    winrt::UIElement const& element,
    winrt::AnimationContext const& context)
{
    if (HasHideAnimation(element, static_cast<winrt::AnimationContext>(context)))
    {
        m_hasHideAnimationsPending = true;
        m_sharedContext |= context;
        QueueElementForAnimation(ElementInfo(
            this /* owner */,
            element,
            AnimationTrigger::Hide,
            context));
    }
}

void ElementAnimator::OnElementBoundsChanged(
    winrt::UIElement const& element,
    winrt::AnimationContext const& context,
    winrt::Rect const& oldBounds,
    winrt::Rect const& newBounds)
{
    if (HasBoundsChangeAnimation(element, static_cast<winrt::AnimationContext>(context), oldBounds, newBounds))
    {
        m_hasBoundsChangeAnimationsPending = true;
        m_sharedContext |= context;
        QueueElementForAnimation(ElementInfo(
            this /* owner */,
            element,
            AnimationTrigger::BoundsChange,
            context,
            oldBounds,
            newBounds));
    }
}

bool ElementAnimator::HasShowAnimation(
    winrt::UIElement const& element,
    winrt::AnimationContext const& context)
{
    return overridable().HasShowAnimationCore(element, context);
}

bool ElementAnimator::HasHideAnimation(
    winrt::UIElement const& element,
    winrt::AnimationContext const& context)
{
    return overridable().HasHideAnimationCore(element, context);
}

bool ElementAnimator::HasBoundsChangeAnimation(
    winrt::UIElement const& element,
    winrt::AnimationContext const& context,
    winrt::Rect const& oldBounds,
    winrt::Rect const& newBounds)
{
    return overridable().HasBoundsChangeAnimationCore(element, context, oldBounds, newBounds);
}

#pragma endregion

#pragma region IElementAnimatorOverrides

bool ElementAnimator::HasShowAnimationCore(
    winrt::UIElement const& /*element*/,
    winrt::AnimationContext const& /*context*/)
{
    throw winrt::hresult_not_implemented();
}

bool ElementAnimator::HasHideAnimationCore(
    winrt::UIElement const& /*element*/,
    winrt::AnimationContext const& /*context*/)
{
    throw winrt::hresult_not_implemented();
}

bool ElementAnimator::HasBoundsChangeAnimationCore(
    winrt::UIElement const& /*element*/,
    winrt::AnimationContext const& /*context*/,
    winrt::Rect const& /*oldBounds*/,
    winrt::Rect const& /*newBounds*/)
{
    throw winrt::hresult_not_implemented();
}

void ElementAnimator::StartShowAnimation(
    winrt::UIElement const& /*element*/,
    winrt::AnimationContext const& /*context*/)
{
    throw winrt::hresult_not_implemented();
}

void ElementAnimator::StartHideAnimation(
    winrt::UIElement const& /*element*/,
    winrt::AnimationContext const& /*context*/)
{
    throw winrt::hresult_not_implemented();
}

void ElementAnimator::StartBoundsChangeAnimation(
    winrt::UIElement const& /*element*/,
    winrt::AnimationContext const& /*context*/,
    winrt::Rect const& /*oldBounds*/,
    winrt::Rect const& /*newBounds*/)
{
    throw winrt::hresult_not_implemented();
}

#pragma endregion

#pragma region IElementAnimatorProtected

bool ElementAnimator::HasShowAnimationsPending()
{
    return m_hasShowAnimationsPending;
}

bool ElementAnimator::HasHideAnimationsPending()
{
    return m_hasHideAnimationsPending;
}

bool ElementAnimator::HasBoundsChangeAnimationsPending()
{
    return m_hasBoundsChangeAnimationsPending;
}

winrt::AnimationContext ElementAnimator::SharedContext()
{
    return m_sharedContext;
}

void ElementAnimator::OnShowAnimationCompleted(winrt::UIElement const& element)
{
    m_showAnimationCompletedEventSource(*this, element);
}

void ElementAnimator::OnHideAnimationCompleted(winrt::UIElement const& element)
{
    m_hideAnimationCompletedEventSource(*this, element);
}

void ElementAnimator::OnBoundsChangeAnimationCompleted(winrt::UIElement const& element)
{
    m_boundsChangeAnimationCompletedEventSource(*this, element);
}

#pragma endregion

void ElementAnimator::QueueElementForAnimation(ElementInfo elementInfo)
{
    m_animatingElements.push_back(std::move(elementInfo));
    if (m_animatingElements.size() == 1)
    {
        m_rendering = winrt::Windows::UI::Xaml::Media::CompositionTarget::Rendering(winrt::auto_revoke, { this, &ElementAnimator::OnRendering });
    }
}

void ElementAnimator::OnRendering(winrt::IInspectable const& /*sender*/, winrt::IInspectable const& /*args*/)
{
    m_rendering.revoke();

    auto resetState = gsl::finally([this]()
    {
        ResetState();
    });

    for (const auto& elementInfo : m_animatingElements)
    {
        switch (elementInfo.Trigger())
        {
        case AnimationTrigger::Show:
            // Call into the derivied class's StartShowAnimation override
            overridable().StartShowAnimation(elementInfo.Element(), elementInfo.Context());
            break;
        case AnimationTrigger::Hide:
            // Call into the derivied class's StartHideAnimation override
            overridable().StartHideAnimation(elementInfo.Element(), elementInfo.Context());
            break;
        case AnimationTrigger::BoundsChange:
            // Call into the derivied class's StartBoundsChangeAnimation override
            overridable().StartBoundsChangeAnimation(
                elementInfo.Element(),
                elementInfo.Context(),
                elementInfo.OldBounds(),
                elementInfo.NewBounds());
            break;
        }
    }
}

void ElementAnimator::ResetState()
{
    m_animatingElements.clear();
    m_hasShowAnimationsPending = m_hasHideAnimationsPending = m_hasBoundsChangeAnimationsPending = false;
    m_sharedContext = winrt::AnimationContext::None;
}
