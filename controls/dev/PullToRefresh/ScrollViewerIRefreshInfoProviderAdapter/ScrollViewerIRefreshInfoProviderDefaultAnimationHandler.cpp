// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ViewModel.h"
#include "TreeViewNode.h"
#include "VectorChangedEventArgs.h"
#include "ScrollViewerIRefreshInfoProviderAdapter.h"
#include "PTRTracing.h"
#include "ImmediateRenderingHelpers.h"

#define REFRESH_ANIMATION_DURATION 100ms
#define REFRESH_VISUALIZER_OVERPAN_RATIO 0.4

// Implementors of the IAdapterAnimationHandler interface are responsible for implementing the
// 3 well defined component level animations in a PTR scenario. The three animations involved 
// in PTR include the expression animation used to have the RefreshVisualizer and its 
// InfoProvider follow the users finger, the animation used to show the RefreshVisualizer
// when a refresh is requested, and the animation used to hide the refreshVisualizer when the 
// refresh is completed.

// The interaction tracker set up by the Adapter has to be assembled in a very particular way. 
// Factoring out this functionality is a way to expose the animation for
// Alteration without having to expose the "delicate" interaction tracker.

ScrollViewerIRefreshInfoProviderDefaultAnimationHandler::ScrollViewerIRefreshInfoProviderDefaultAnimationHandler(const winrt::UIElement&  container, const winrt::RefreshPullDirection& refreshPullDirection)
{
    PTR_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);
    if (container)
    {
        auto containerVisual = winrt::ElementCompositionPreview::GetElementVisual(container);
        m_compositor = containerVisual.Compositor();
        containerVisual.Clip(m_compositor.CreateInsetClip(0.0f, 0.0f, 0.0f, 0.0f));
    }
    m_refreshPullDirection = refreshPullDirection;
}

ScrollViewerIRefreshInfoProviderDefaultAnimationHandler::~ScrollViewerIRefreshInfoProviderDefaultAnimationHandler()
{
    PTR_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);
    if (m_refreshCompletedScopedBatch)
    {
        m_refreshCompletedScopedBatch.Completed(m_compositionScopedBatchCompletedEventToken);
    }
}

void ScrollViewerIRefreshInfoProviderDefaultAnimationHandler::InteractionTrackerAnimation(const winrt::UIElement& refreshVisualizer, const winrt::UIElement& infoProvider, const winrt::InteractionTracker& interactionTracker)
{
    PTR_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);
    ValidateAndStoreParameters(refreshVisualizer, infoProvider, interactionTracker);

    if (!interactionTracker)
    {
        m_interactionTracker = nullptr;
        return;
    }

    if (ImmediateRendering::IsRequested())
    {
        // The expressions below evaluated for the tracker's current position.
        const winrt::float3 position = interactionTracker.Position();
        const winrt::float2 size = ImmediateRefreshVisualizerSize();
        const float s = IsOrientationVertical() ? size.y : size.x;
        const float p = IsOrientationVertical() ? position.y : position.x;
        const bool isNear = m_refreshPullDirection == winrt::RefreshPullDirection::TopToBottom || m_refreshPullDirection == winrt::RefreshPullDirection::LeftToRight;
        m_immediateRenderingRevoker.revoke();
        SetImmediateTranslation(m_refreshVisualizer.get(), isNear ? -s + std::min(s, -p) : s - std::min(s, p));
        SetImmediateTranslation(m_infoProvider.get(), isNear ? std::min(s, std::max(-p, -s * 0.4f)) : std::max(-s, std::min(-p, s * 0.4f)));
        return;
    }

    if ((!m_refreshVisualizerVisualOffsetAnimation || !m_infoProviderOffsetAnimation || m_interactionAnimationNeedsUpdating) && m_compositor)
    {
        switch (m_refreshPullDirection)
        {
        case winrt::RefreshPullDirection::TopToBottom:
            m_refreshVisualizerVisualOffsetAnimation = m_compositor.CreateExpressionAnimation(
                L"-refreshVisualizerVisual.Size.Y + min(refreshVisualizerVisual.Size.Y, -interactionTracker.Position.Y)");

            m_infoProviderOffsetAnimation = m_compositor.CreateExpressionAnimation(
                L"min(refreshVisualizerVisual.Size.Y, max(-interactionTracker.Position.Y, -refreshVisualizerVisual.Size.Y * 0.4))");

            break;
        case winrt::RefreshPullDirection::BottomToTop:
            m_refreshVisualizerVisualOffsetAnimation = m_compositor.CreateExpressionAnimation(
                L"refreshVisualizerVisual.Size.Y - min(refreshVisualizerVisual.Size.Y, interactionTracker.Position.Y)");

            m_infoProviderOffsetAnimation = m_compositor.CreateExpressionAnimation(
                L"max(-refreshVisualizerVisual.Size.Y, min(-interactionTracker.Position.Y, refreshVisualizerVisual.Size.Y * 0.4))");

            break;
        case winrt::RefreshPullDirection::LeftToRight:
            m_refreshVisualizerVisualOffsetAnimation = m_compositor.CreateExpressionAnimation(
                L"-refreshVisualizerVisual.Size.X + min(refreshVisualizerVisual.Size.X, -interactionTracker.Position.X)");

            m_infoProviderOffsetAnimation = m_compositor.CreateExpressionAnimation(
                L"min(refreshVisualizerVisual.Size.X, max(-interactionTracker.Position.X, -refreshVisualizerVisual.Size.X * 0.4))");

            break;
        case winrt::RefreshPullDirection::RightToLeft:
            m_refreshVisualizerVisualOffsetAnimation = m_compositor.CreateExpressionAnimation(
                L"refreshVisualizerVisual.Size.X - min(refreshVisualizerVisual.Size.X, interactionTracker.Position.X)");

            m_infoProviderOffsetAnimation = m_compositor.CreateExpressionAnimation(
                L"max(-refreshVisualizerVisual.Size.X, min(-interactionTracker.Position.X, refreshVisualizerVisual.Size.X * 0.4))");

            break;
        default:
            MUX_ASSERT(false);
        }

        m_refreshVisualizerVisualOffsetAnimation.SetReferenceParameter(L"refreshVisualizerVisual", m_refreshVisualizerVisual);
        m_refreshVisualizerVisualOffsetAnimation.SetReferenceParameter(L"interactionTracker", m_interactionTracker);

        m_infoProviderOffsetAnimation.SetReferenceParameter(L"refreshVisualizerVisual", m_refreshVisualizerVisual);
        m_infoProviderOffsetAnimation.SetReferenceParameter(L"infoProviderVisual", m_infoProviderVisual);
        m_infoProviderOffsetAnimation.SetReferenceParameter(L"interactionTracker", m_interactionTracker);

        m_interactionAnimationNeedsUpdating = false;
    }

    if (m_refreshVisualizerVisualOffsetAnimation && m_infoProviderOffsetAnimation)
    {
        m_refreshVisualizerVisual.Offset(winrt::float3(0.0f));
        m_infoProviderVisual.Offset(winrt::float3(0.0f));
        m_refreshVisualizerVisual.Properties().InsertVector3(L"Translation", { 0.0f, 0.0f, 0.0f });
        m_infoProviderVisual.Properties().InsertVector3(L"Translation", { 0.0f, 0.0f, 0.0f });

        winrt::hstring animatedProperty = getAnimatedPropertyName();

        m_refreshVisualizerVisual.StartAnimation(animatedProperty, m_refreshVisualizerVisualOffsetAnimation);
        m_infoProviderVisual.StartAnimation(animatedProperty, m_infoProviderOffsetAnimation);
    }
}

void ScrollViewerIRefreshInfoProviderDefaultAnimationHandler::RefreshRequestedAnimation(const winrt::UIElement& refreshVisualizer, const winrt::UIElement& infoProvider, double executionRatio)
{
    PTR_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);
    ValidateAndStoreParameters(refreshVisualizer, infoProvider, nullptr);

    if (ImmediateRendering::IsRequested())
    {
        const winrt::float2 size = ImmediateRefreshVisualizerSize();
        const float s = IsOrientationVertical() ? size.y : size.x;
        const float ratio = static_cast<float>(executionRatio);
        const bool isNear = m_refreshPullDirection == winrt::RefreshPullDirection::TopToBottom || m_refreshPullDirection == winrt::RefreshPullDirection::LeftToRight;
        StartImmediateAnimation(isNear ? -(s * (1 - ratio)) : s * (1 - ratio), isNear ? s * ratio : -s * ratio, false /*isRefreshCompleted*/);
        return;
    }

    if ((!m_refreshVisualizerRefreshRequestedAnimation || !m_infoProviderRefreshRequestedAnimation || m_refreshRequestedAnimationNeedsUpdating) && m_compositor)
    {
        m_refreshVisualizerRefreshRequestedAnimation = m_compositor.CreateScalarKeyFrameAnimation();
        m_refreshVisualizerRefreshRequestedAnimation.Duration(REFRESH_ANIMATION_DURATION);
        m_infoProviderRefreshRequestedAnimation = m_compositor.CreateScalarKeyFrameAnimation();
        m_infoProviderRefreshRequestedAnimation.Duration(REFRESH_ANIMATION_DURATION);
        switch (m_refreshPullDirection)
        {
        case winrt::RefreshPullDirection::TopToBottom:
            m_refreshVisualizerRefreshRequestedAnimation.InsertKeyFrame(1.0f, -(m_refreshVisualizerVisual.Size().y * (1 - (float)executionRatio)));
            m_infoProviderRefreshRequestedAnimation.InsertKeyFrame(1.0f, m_refreshVisualizerVisual.Size().y * (float)executionRatio);
            break;
        case winrt::RefreshPullDirection::BottomToTop:
            m_refreshVisualizerRefreshRequestedAnimation.InsertKeyFrame(1.0f, m_refreshVisualizerVisual.Size().y * (1 - (float)executionRatio));
            m_infoProviderRefreshRequestedAnimation.InsertKeyFrame(1.0f, -m_refreshVisualizerVisual.Size().y * (float)executionRatio);
            break;
        case winrt::RefreshPullDirection::LeftToRight:
            m_refreshVisualizerRefreshRequestedAnimation.InsertKeyFrame(1.0f, -(m_refreshVisualizerVisual.Size().x * (1 - (float)executionRatio)));
            m_infoProviderRefreshRequestedAnimation.InsertKeyFrame(1.0f, m_refreshVisualizerVisual.Size().x * (float)executionRatio);
            break;
        case winrt::RefreshPullDirection::RightToLeft:
            m_refreshVisualizerRefreshRequestedAnimation.InsertKeyFrame(1.0f, m_refreshVisualizerVisual.Size().x * (1 - (float)executionRatio));
            m_infoProviderRefreshRequestedAnimation.InsertKeyFrame(1.0f, -m_refreshVisualizerVisual.Size().x * (float)executionRatio);
            break;
        default:
            MUX_ASSERT(false);
        }
        m_refreshRequestedAnimationNeedsUpdating = false;
    }

    if (m_refreshVisualizerRefreshRequestedAnimation && m_infoProviderRefreshRequestedAnimation)
    {
        winrt::hstring animatedProperty = getAnimatedPropertyName();

        m_refreshVisualizerVisual.StartAnimation(animatedProperty, m_refreshVisualizerRefreshRequestedAnimation);
        m_infoProviderVisual.StartAnimation(animatedProperty, m_infoProviderRefreshRequestedAnimation);
    }
}

void ScrollViewerIRefreshInfoProviderDefaultAnimationHandler::RefreshCompletedAnimation(const winrt::UIElement& refreshVisualizer, const winrt::UIElement& infoProvider)
{
    PTR_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);
    ValidateAndStoreParameters(refreshVisualizer, infoProvider, nullptr);

    if (ImmediateRendering::IsRequested())
    {
        const winrt::float2 size = ImmediateRefreshVisualizerSize();
        const float s = IsOrientationVertical() ? size.y : size.x;
        const bool isNear = m_refreshPullDirection == winrt::RefreshPullDirection::TopToBottom || m_refreshPullDirection == winrt::RefreshPullDirection::LeftToRight;
        StartImmediateAnimation(isNear ? -s : s, 0.0f, true /*isRefreshCompleted*/);
        return;
    }

    if ((!m_refreshVisualizerRefreshCompletedAnimation || !m_infoProviderRefreshCompletedAnimation || m_refreshCompletedAnimationNeedsUpdating) && m_compositor)
    {
        m_refreshVisualizerRefreshCompletedAnimation = m_compositor.CreateScalarKeyFrameAnimation();
        m_refreshVisualizerRefreshCompletedAnimation.Duration(REFRESH_ANIMATION_DURATION);
        m_infoProviderRefreshCompletedAnimation = m_compositor.CreateScalarKeyFrameAnimation();
        m_infoProviderRefreshCompletedAnimation.Duration(REFRESH_ANIMATION_DURATION);
        m_infoProviderRefreshCompletedAnimation.InsertKeyFrame(1.0f, 0.0f);

        switch (m_refreshPullDirection)
        {
        case winrt::RefreshPullDirection::TopToBottom:
            m_refreshVisualizerRefreshCompletedAnimation.InsertKeyFrame(1.0f, -(m_refreshVisualizerVisual.Size().y));
            break;
        case winrt::RefreshPullDirection::BottomToTop:
            m_refreshVisualizerRefreshCompletedAnimation.InsertKeyFrame(1.0f, m_refreshVisualizerVisual.Size().y);
            break;
        case winrt::RefreshPullDirection::LeftToRight:
            m_refreshVisualizerRefreshCompletedAnimation.InsertKeyFrame(1.0f, -(m_refreshVisualizerVisual.Size().x));
            break;
        case winrt::RefreshPullDirection::RightToLeft:
            m_refreshVisualizerRefreshCompletedAnimation.InsertKeyFrame(1.0f, m_refreshVisualizerVisual.Size().x);
            break;
        default:
            MUX_ASSERT(false);
        }
        m_refreshCompletedAnimationNeedsUpdating = false;
    }

    if (m_compositor)
    {
        m_refreshCompletedScopedBatch = m_compositor.CreateScopedBatch(winrt::CompositionBatchTypes::Animation);
        m_compositionScopedBatchCompletedEventToken = m_refreshCompletedScopedBatch.Completed({ this, &ScrollViewerIRefreshInfoProviderDefaultAnimationHandler::RefreshCompletedBatchCompleted });
    }

    if (m_refreshVisualizerRefreshCompletedAnimation && m_infoProviderRefreshCompletedAnimation)
    {
        winrt::hstring animatedProperty = getAnimatedPropertyName();

        m_refreshVisualizerVisual.StartAnimation(animatedProperty, m_refreshVisualizerRefreshCompletedAnimation);
        m_infoProviderVisual.StartAnimation(animatedProperty, m_infoProviderRefreshCompletedAnimation);
    }

    if (m_refreshCompletedScopedBatch)
    {
        m_refreshCompletedScopedBatch.End();
    }
}

//PrivateHelpers
void ScrollViewerIRefreshInfoProviderDefaultAnimationHandler::ValidateAndStoreParameters(const winrt::UIElement& refreshVisualizer, const winrt::UIElement& infoProvider, const winrt::InteractionTracker& interactionTracker)
{
    if (refreshVisualizer && m_refreshVisualizer.get() != refreshVisualizer)
    {

        m_refreshVisualizerVisual = winrt::ElementCompositionPreview::GetElementVisual(refreshVisualizer);
        m_refreshVisualizer.set(refreshVisualizer);
        m_interactionAnimationNeedsUpdating = true;
        m_refreshRequestedAnimationNeedsUpdating = true;
        m_refreshCompletedAnimationNeedsUpdating = true;

        if (!ImmediateRendering::IsRequested())
        {
            winrt::ElementCompositionPreview::SetIsTranslationEnabled(refreshVisualizer, true);
            m_refreshVisualizerVisual.Properties().InsertVector3(L"Translation", { 0.0f, 0.0f, 0.0f });
        }

        if (!m_compositor)
        {
            m_compositor = m_refreshVisualizerVisual.Compositor();
        }
    }

    if (infoProvider && m_infoProvider.get() != infoProvider)
    {
        m_infoProviderVisual = winrt::ElementCompositionPreview::GetElementVisual(infoProvider);
        m_infoProvider.set(infoProvider);
        m_interactionAnimationNeedsUpdating = true;
        m_refreshRequestedAnimationNeedsUpdating = true;
        m_refreshCompletedAnimationNeedsUpdating = true;

        if (!ImmediateRendering::IsRequested())
        {
            winrt::ElementCompositionPreview::SetIsTranslationEnabled(infoProvider, true);
            m_infoProviderVisual.Properties().InsertVector3(L"Translation", { 0.0f, 0.0f, 0.0f });
        }

        if (!m_compositor)
        {
            m_compositor = m_infoProviderVisual.Compositor();
        }
    }

    if (interactionTracker && m_interactionTracker != interactionTracker)
    {
        m_interactionTracker = interactionTracker;
        m_interactionAnimationNeedsUpdating = true;
        m_refreshRequestedAnimationNeedsUpdating = true;
        m_refreshCompletedAnimationNeedsUpdating = true;
    }
}

void ScrollViewerIRefreshInfoProviderDefaultAnimationHandler::RefreshCompletedBatchCompleted(const winrt::IInspectable& /*sender*/, const winrt::CompositionBatchCompletedEventArgs& /*args*/)
{
    PTR_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);
    m_refreshCompletedScopedBatch.Completed(m_compositionScopedBatchCompletedEventToken);
    InteractionTrackerAnimation(m_refreshVisualizer.get(), m_infoProvider.get(), m_interactionTracker);
}

bool ScrollViewerIRefreshInfoProviderDefaultAnimationHandler::IsOrientationVertical()
{
    return (m_refreshPullDirection == winrt::RefreshPullDirection::TopToBottom || m_refreshPullDirection == winrt::RefreshPullDirection::BottomToTop);
}

winrt::hstring ScrollViewerIRefreshInfoProviderDefaultAnimationHandler::getAnimatedPropertyName()
{
    winrt::hstring animatedProperty = L"";
    animatedProperty = static_cast<std::wstring>(animatedProperty) + L"Translation";

    if (IsOrientationVertical())
    {
        animatedProperty = static_cast<std::wstring>(animatedProperty) + L".Y";
    }
    else
    {
        animatedProperty = static_cast<std::wstring>(animatedProperty) + L".X";
    }

    return animatedProperty;
}

winrt::float2 ScrollViewerIRefreshInfoProviderDefaultAnimationHandler::ImmediateRefreshVisualizerSize()
{
    // refreshVisualizerVisual.Size: no Composition node sizes hand-off visuals under immediate rendering.
    if (auto refreshVisualizer = m_refreshVisualizer.get())
    {
        return refreshVisualizer.ActualSize();
    }
    return {};
}

void ScrollViewerIRefreshInfoProviderDefaultAnimationHandler::SetImmediateTranslation(const winrt::UIElement& element, float value)
{
    if (element)
    {
        winrt::float3 translation = element.Translation();
        (IsOrientationVertical() ? translation.y : translation.x) = value;
        element.Translation(translation);
    }
}

float ScrollViewerIRefreshInfoProviderDefaultAnimationHandler::GetImmediateTranslation(const winrt::UIElement& element)
{
    if (element)
    {
        const winrt::float3 translation = element.Translation();
        return IsOrientationVertical() ? translation.y : translation.x;
    }
    return 0.0f;
}

void ScrollViewerIRefreshInfoProviderDefaultAnimationHandler::StartImmediateAnimation(float refreshVisualizerTo, float infoProviderTo, bool isRefreshCompleted)
{
    m_immediateRefreshVisualizerFrom = GetImmediateTranslation(m_refreshVisualizer.get());
    m_immediateInfoProviderFrom = GetImmediateTranslation(m_infoProvider.get());
    m_immediateRefreshVisualizerTo = refreshVisualizerTo;
    m_immediateInfoProviderTo = infoProviderTo;
    m_immediateIsRefreshCompleted = isRefreshCompleted;
    m_immediateAnimationStart = std::chrono::steady_clock::now();
    if (!m_immediateRenderingRevoker)
    {
        m_immediateRenderingRevoker = winrt::Microsoft::UI::Xaml::Media::CompositionTarget::Rendering(winrt::auto_revoke, { this, &ScrollViewerIRefreshInfoProviderDefaultAnimationHandler::OnImmediateRendering });
    }
}

void ScrollViewerIRefreshInfoProviderDefaultAnimationHandler::OnImmediateRendering(const winrt::IInspectable& /*sender*/, const winrt::IInspectable& /*args*/)
{
    const float progress = std::min(1.0f, std::chrono::duration<float, std::milli>(std::chrono::steady_clock::now() - m_immediateAnimationStart).count()
        / std::chrono::duration<float, std::milli>(REFRESH_ANIMATION_DURATION).count());
    SetImmediateTranslation(m_refreshVisualizer.get(), m_immediateRefreshVisualizerFrom + (m_immediateRefreshVisualizerTo - m_immediateRefreshVisualizerFrom) * progress);
    SetImmediateTranslation(m_infoProvider.get(), m_immediateInfoProviderFrom + (m_immediateInfoProviderTo - m_immediateInfoProviderFrom) * progress);
    if (progress >= 1.0f)
    {
        m_immediateRenderingRevoker.revoke();
        if (m_immediateIsRefreshCompleted)
        {
            // RefreshCompletedBatchCompleted
            InteractionTrackerAnimation(m_refreshVisualizer.get(), m_infoProvider.get(), m_interactionTracker);
        }
    }
}