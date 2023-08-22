// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ScrollViewerIRefreshInfoProviderAdapter.h"
#include "ScrollViewerIRefreshInfoProviderAdapterFactory.h"
#include "RefreshInteractionRatioChangedEventArgs.h"
#include "RefreshVisualizer.h"
#include "ResourceAccessor.h"
#include "PTRTracing.h"

// The maximum initial scroll viewer offset allowed before PTR is disabled, and we enter the peeking state
#define INITIAL_OFFSET_THRESHOLD 1.0f

// The farthest down the tree we are willing to search for a SV in the adapt from tree method
#define MAX_BFS_DEPTH 10

// In the event that we call Refresh before having an implementation of IRefreshInfoProvider to tell us
// what value to use as the execution ratio, we use this value instead.
#define FALLBACK_EXECUTION_RATIO 0.8

// The ScrollViewerAdapter is responsible for creating an implementation of the IRefreshInfoProvider interface from a ScrollViewer. This
// Is accomplished by attaching an interaction tracker to the scrollViewer's content presenter and wiring the PTR functionality up to that.
ScrollViewerIRefreshInfoProviderAdapter::~ScrollViewerIRefreshInfoProviderAdapter()
{
    PTR_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);
    if (m_scrollViewer.safe_get())
    {
        CleanupScrollViewer();
    }

    if (m_infoProvider.safe_get<winrt::IRefreshInfoProvider>())
    {
        CleanupIRefreshInfoProvider();
    }
}

ScrollViewerIRefreshInfoProviderAdapter::ScrollViewerIRefreshInfoProviderAdapter(
    winrt::RefreshPullDirection const& refreshPullDirection, winrt::IAdapterAnimationHandler const& animationHandler)
{
    PTR_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);

    m_refreshPullDirection = refreshPullDirection;
    if (animationHandler)
    {
        m_animationHandler.set(animationHandler);
    }
    else
    {
        m_animationHandler.set(winrt::make_self<ScrollViewerIRefreshInfoProviderDefaultAnimationHandler>(nullptr, m_refreshPullDirection).as<winrt::IAdapterAnimationHandler>());
    }
}

winrt::IRefreshInfoProvider ScrollViewerIRefreshInfoProviderAdapter::AdaptFromTree(winrt::UIElement const& root, winrt::Size const& refreshVisualizerSize)
{
    PTR_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);

    const winrt::UIElement& winrtRoot = root;
    winrt::FxScrollViewer rootAsSV = winrtRoot.try_as<winrt::FxScrollViewer>();
    int depth = 0;
    if (rootAsSV)
    {
        return Adapt(winrtRoot.as<winrt::FxScrollViewer>(), refreshVisualizerSize);
    }
    else
    {
        while (depth < MAX_BFS_DEPTH)
        {
            winrt::FxScrollViewer helperResult = AdaptFromTreeRecursiveHelper(winrtRoot, depth);
            if (helperResult)
            {
                return Adapt(helperResult, refreshVisualizerSize);
                break;
            }
            depth++;
        }
    }

    return nullptr;
}

winrt::IRefreshInfoProvider ScrollViewerIRefreshInfoProviderAdapter::Adapt(winrt::FxScrollViewer const& adaptee, winrt::Size const& refreshVisualizerSize)
{
    PTR_TRACE_INFO(nullptr, TRACE_MSG_METH_PTR, METH_NAME, this, adaptee);
    if (!adaptee)
    {
        throw winrt::hresult_invalid_argument(L"Adaptee cannot be null.");
    }

    if (m_scrollViewer)
    {
        CleanupScrollViewer();
    }

    if (m_infoProvider)
    {
        CleanupIRefreshInfoProvider();
    }

    m_infoProvider.set(nullptr);
    m_interactionTracker.set(nullptr);
    m_visualInteractionSource.set(nullptr);
    m_scrollViewer.set(adaptee);


    if (!m_scrollViewer.get().Content())
    {
        throw winrt::hresult_invalid_argument(L"Adaptee's content property cannot be null.");
    }

    winrt::UIElement content = GetScrollContent();
    if (!content)
    {
        throw winrt::hresult_invalid_argument(L"Adaptee's content property must be a UIElement.");
    }

    auto contentParent = winrt::VisualTreeHelper::GetParent(content);
    if (!contentParent)
    {
        //If the Content property does not have a parent this likely means the OnLoaded event of the SV has not fired yet.
        //Attach to this event to finish the adaption.
        m_scrollViewer_LoadedToken = m_scrollViewer.get().Loaded({ this, &ScrollViewerIRefreshInfoProviderAdapter::OnScrollViewerLoaded });
    }
    else
    {
        OnScrollViewerLoaded(nullptr, nullptr);
        winrt::UIElement contentParentAsUIElement = contentParent.try_as<winrt::UIElement>();
        if (!contentParentAsUIElement)
        {
            throw winrt::hresult_invalid_argument(L"Adaptee's content's parent must be a UIElement.");
        }
    }
    winrt::Visual contentVisual = winrt::ElementCompositionPreview::GetElementVisual(content);
    winrt::Compositor compositor = contentVisual.Compositor();

    m_infoProvider.set(winrt::make_self<RefreshInfoProviderImpl>(m_refreshPullDirection, refreshVisualizerSize, compositor));

    m_infoProvider_RefreshStartedToken = m_infoProvider.get()->RefreshStarted({ this, &ScrollViewerIRefreshInfoProviderAdapter::OnRefreshStarted });
    m_infoProvider_RefreshCompletedToken = m_infoProvider.get()->RefreshCompleted({ this, &ScrollViewerIRefreshInfoProviderAdapter::OnRefreshCompleted });

    m_interactionTracker.set(winrt::InteractionTracker::CreateWithOwner(compositor, m_infoProvider.as<winrt::IInteractionTrackerOwner>()));

    m_interactionTracker.get().MinPosition(winrt::float3(0.0f));
    m_interactionTracker.get().MaxPosition(winrt::float3(0.0f));
    m_interactionTracker.get().MinScale(1.0f);
    m_interactionTracker.get().MaxScale(1.0f);

    if (m_visualInteractionSource.get())
    {
        m_interactionTracker.get().InteractionSources().Add(m_visualInteractionSource.get());
        m_visualInteractionSourceIsAttached = true;
    }

    winrt::PointerEventHandler myEventHandler = 
    [=](auto sender, auto args)
    {
        PTR_TRACE_INFO(nullptr, TRACE_MSG_METH, L"ScrollViewer::PointerPressedHandler", this);
        if (args.Pointer().PointerDeviceType() == winrt::PointerDeviceType::Touch && m_visualInteractionSource.get())
        {
            if (m_visualInteractionSourceIsAttached)
            {
                winrt::PointerPoint pp = args.GetCurrentPoint(nullptr);

                if (pp)
                {
                    bool tryRedirectForManipulationSuccessful = true;

                    try
                    {
                        PTR_TRACE_INFO(nullptr, TRACE_MSG_METH_METH, L"ScrollViewer::PointerPressedHandler", this, L"TryRedirectForManipulation");
                        m_visualInteractionSource.get().TryRedirectForManipulation(pp);
                    }
                    catch (const winrt::hresult_error& e)
                    {
                        // Swallowing Access Denied error because of InteractionTracker bug 17434718 which has been causing crashes at least in RS3, RS4 and RS5.
                        if (e.to_abi() != E_ACCESSDENIED)
                        {
                            throw;
                        }

                        tryRedirectForManipulationSuccessful = false;
                    }

                    if (tryRedirectForManipulationSuccessful)
                    {
                        m_infoProvider.get()->SetPeekingMode(!IsWithinOffsetThreshold());
                    }
                }
            }
            else
            {
                throw winrt::hresult_invalid_argument(L"Invalid IRefreshInfoProvider adaptation of scroll viewer, this can occur when calling TryRedirectForManipulation to an unattached visual interaction source.");
            }
        }
    };

    m_boxedPointerPressedEventHandler.set(winrt::box_value<winrt::PointerEventHandler>(myEventHandler));
    m_scrollViewer.get().AddHandler(winrt::UIElement::PointerPressedEvent(), m_boxedPointerPressedEventHandler.get(), true /* handledEventsToo */);
    m_scrollViewer_DirectManipulationCompletedToken = m_scrollViewer.get().DirectManipulationCompleted({ this, &ScrollViewerIRefreshInfoProviderAdapter::OnScrollViewerDirectManipulationCompleted });
    m_scrollViewer_ViewChangingToken = m_scrollViewer.get().ViewChanging({ this, &ScrollViewerIRefreshInfoProviderAdapter::OnScrollViewerViewChanging });

    return m_infoProvider.as<winrt::IRefreshInfoProvider>();
}

void ScrollViewerIRefreshInfoProviderAdapter::SetAnimations(winrt::UIElement const& refreshVisualizerContainer)
{
    PTR_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);
    if (!refreshVisualizerContainer)
    {
        throw winrt::hresult_invalid_argument(L"The refreshVisualizerContainer cannot be null.");
    }

    m_animationHandler.get().InteractionTrackerAnimation(refreshVisualizerContainer, GetScrollContent(), m_interactionTracker.get());
}

void ScrollViewerIRefreshInfoProviderAdapter::OnRefreshStarted(const winrt::IInspectable& /*sender*/, const winrt::IInspectable& /*args*/)
{
    PTR_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);
    auto content = GetScrollContent();
    if (content)
    {
        content.CancelDirectManipulations();
    }

    double executionRatio = FALLBACK_EXECUTION_RATIO;
    if (m_infoProvider.get())
    {
        executionRatio = m_infoProvider.get()->ExecutionRatio();
    }
    
    m_animationHandler.get().RefreshRequestedAnimation(nullptr, content, executionRatio);
}

void ScrollViewerIRefreshInfoProviderAdapter::OnRefreshCompleted(const winrt::IInspectable& /*sender*/, const winrt::IInspectable& /*args*/)
{
    PTR_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);
    m_animationHandler.get().RefreshCompletedAnimation(nullptr, GetScrollContent());
}

void ScrollViewerIRefreshInfoProviderAdapter::OnScrollViewerLoaded(const winrt::IInspectable& /*sender*/, const winrt::IInspectable& /*args*/)
{
    PTR_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);
    winrt::UIElement content = GetScrollContent();
    if (!content)
    {
        throw winrt::hresult_invalid_argument(L"Adaptee's content property must be a UIElement.");
    }

    auto contentParent = winrt::VisualTreeHelper::GetParent(content);
    if (!contentParent)
    {
        throw winrt::hresult_invalid_argument(L"Adaptee cannot be null.");
    }

    winrt::UIElement contentParentAsUIElement = contentParent.try_as<winrt::UIElement>();
    if (!contentParentAsUIElement)
    {
        throw winrt::hresult_invalid_argument(L"Adaptee's content's parent must be a UIElement.");
    }

    MakeInteractionSource(contentParentAsUIElement);

    m_scrollViewer.get().Loaded(m_scrollViewer_LoadedToken);
}

void ScrollViewerIRefreshInfoProviderAdapter::OnScrollViewerDirectManipulationCompleted(const winrt::IInspectable& /*sender*/, const winrt::IInspectable& /*args*/)
{
    PTR_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);
    if (m_infoProvider.get())
    {
        m_infoProvider.get()->UpdateIsInteractingForRefresh(false);
    }
}

void ScrollViewerIRefreshInfoProviderAdapter::OnScrollViewerViewChanging(const winrt::IInspectable& /*sender*/, const winrt::Windows::UI::Xaml::Controls::ScrollViewerViewChangingEventArgs& args)
{
    if (m_infoProvider.get() && m_infoProvider.get()->IsInteractingForRefresh())
    {
        PTR_TRACE_INFO(nullptr, TRACE_MSG_METH_DBL_DBL, METH_NAME, this, args.FinalView().HorizontalOffset(), args.FinalView().VerticalOffset());
        if (!IsWithinOffsetThreshold())
        {
            PTR_TRACE_INFO(nullptr, TRACE_MSG_METH_STR, METH_NAME, this, L"No longer interacting for refresh due to ScrollViewer view change.");
            m_infoProvider.get()->UpdateIsInteractingForRefresh(false);
        }
    }
}

bool ScrollViewerIRefreshInfoProviderAdapter::IsOrientationVertical()
{
    return (m_refreshPullDirection == winrt::RefreshPullDirection::TopToBottom || m_refreshPullDirection == winrt::RefreshPullDirection::BottomToTop);
}

winrt::UIElement ScrollViewerIRefreshInfoProviderAdapter::GetScrollContent()
{
    if (m_scrollViewer)
    {
        auto content = m_scrollViewer.get().Content();
        return content.try_as<winrt::UIElement>();
    }
    return nullptr;
}

void ScrollViewerIRefreshInfoProviderAdapter::MakeInteractionSource(const winrt::UIElement& contentParent)
{
    PTR_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);
    winrt::Visual contentParentVisual = winrt::ElementCompositionPreview::GetElementVisual(contentParent);

    m_visualInteractionSourceIsAttached = false;
    m_visualInteractionSource.set(winrt::VisualInteractionSource::Create(contentParentVisual));
    m_visualInteractionSource.get().ManipulationRedirectionMode(winrt::VisualInteractionSourceRedirectionMode::CapableTouchpadOnly);
    m_visualInteractionSource.get().ScaleSourceMode(winrt::InteractionSourceMode::Disabled);
    m_visualInteractionSource.get().PositionXSourceMode(IsOrientationVertical() ? winrt::InteractionSourceMode::Disabled : winrt::InteractionSourceMode::EnabledWithInertia);
    m_visualInteractionSource.get().PositionXChainingMode(IsOrientationVertical() ? winrt::InteractionChainingMode::Auto : winrt::InteractionChainingMode::Never);
    m_visualInteractionSource.get().PositionYSourceMode(IsOrientationVertical() ? winrt::InteractionSourceMode::EnabledWithInertia : winrt::InteractionSourceMode::Disabled);
    m_visualInteractionSource.get().PositionYChainingMode(IsOrientationVertical() ? winrt::InteractionChainingMode::Never : winrt::InteractionChainingMode::Auto);

    if (m_interactionTracker)
    {
        m_interactionTracker.get().InteractionSources().Add(m_visualInteractionSource.get());
        m_visualInteractionSourceIsAttached = true;
    }
}

winrt::FxScrollViewer ScrollViewerIRefreshInfoProviderAdapter::AdaptFromTreeRecursiveHelper(const winrt::DependencyObject& root, int depth)
{
    PTR_TRACE_INFO(nullptr, TRACE_MSG_METH_INT, METH_NAME, this, depth);
    const int numChildren = winrt::VisualTreeHelper::GetChildrenCount(root);
    if (depth == 0)
    {
        for (int i = 0; i < numChildren; i++)
        {
            const winrt::DependencyObject childObject = winrt::VisualTreeHelper::GetChild(root, i);
            const winrt::FxScrollViewer childObjectAsSV = childObject.try_as<winrt::FxScrollViewer>();
            if (childObjectAsSV)
            {
                return childObjectAsSV;
            }
        }
        return nullptr;
    }
    else
    {
        for (int i = 0; i < numChildren; i++)
        {
            const winrt::DependencyObject childObject = winrt::VisualTreeHelper::GetChild(root, i);
            const winrt::FxScrollViewer recursiveResult = AdaptFromTreeRecursiveHelper(childObject, depth - 1);
            if (recursiveResult)
            {
                return recursiveResult;
            }
        }
        return nullptr;
    }
}

void ScrollViewerIRefreshInfoProviderAdapter::CleanupScrollViewer()
{
    auto&& sv = m_scrollViewer.get();
    if (m_boxedPointerPressedEventHandler)
    {
        sv.RemoveHandler(winrt::UIElement::PointerPressedEvent(), m_boxedPointerPressedEventHandler.get());
        m_boxedPointerPressedEventHandler.set(nullptr);
    }
    if (m_scrollViewer_DirectManipulationCompletedToken.value)
    {
        sv.DirectManipulationCompleted(m_scrollViewer_DirectManipulationCompletedToken);
        m_scrollViewer_DirectManipulationCompletedToken.value = 0;
    }
    if (m_scrollViewer_ViewChangingToken.value)
    {
        sv.ViewChanging(m_scrollViewer_ViewChangingToken);
        m_scrollViewer_ViewChangingToken.value = 0;
    }
    if (m_scrollViewer_LoadedToken.value)
    {
        sv.Loaded(m_scrollViewer_LoadedToken);
        m_scrollViewer_LoadedToken.value = 0;
    }
}

void ScrollViewerIRefreshInfoProviderAdapter::CleanupIRefreshInfoProvider()
{
    auto&& provider = m_infoProvider.get();
    if (m_infoProvider_RefreshStartedToken.value)
    {
        provider->RefreshStarted(m_infoProvider_RefreshStartedToken);
        m_infoProvider_RefreshStartedToken.value = 0;
    }
    if (m_infoProvider_RefreshCompletedToken.value)
    {
        provider->RefreshCompleted(m_infoProvider_RefreshCompletedToken);
        m_infoProvider_RefreshCompletedToken.value = 0;
    }
}

bool ScrollViewerIRefreshInfoProviderAdapter::IsWithinOffsetThreshold()
{
    switch (m_refreshPullDirection)
    {
    case winrt::RefreshPullDirection::TopToBottom:
        return m_scrollViewer.get().VerticalOffset() < INITIAL_OFFSET_THRESHOLD;
    case winrt::RefreshPullDirection::BottomToTop:
        return m_scrollViewer.get().VerticalOffset() > m_scrollViewer.get().ScrollableHeight() - INITIAL_OFFSET_THRESHOLD;
    case winrt::RefreshPullDirection::LeftToRight:
        return m_scrollViewer.get().HorizontalOffset() < INITIAL_OFFSET_THRESHOLD;
    case winrt::RefreshPullDirection::RightToLeft:
        return m_scrollViewer.get().HorizontalOffset() > m_scrollViewer.get().ScrollableWidth() - INITIAL_OFFSET_THRESHOLD;
    default:
        MUX_ASSERT(false);
        return false;
    }
}
