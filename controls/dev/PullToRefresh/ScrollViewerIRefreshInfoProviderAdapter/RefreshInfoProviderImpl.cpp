// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "RefreshInfoProviderImpl.h"
#include "ScrollViewerIRefreshInfoProviderAdapter.h"
#include "RefreshInteractionRatioChangedEventArgs.h"
#include "PTRTracing.h"

// There is not a lot of value in constantly firing the interaction ratio changed event as the
// animations which are based off of it use the published composition property set which is
// updated regularly. Instead we fire the event every 5th change to reduce overhead.
#define RAISE_INTERACTION_RATIO_CHANGED_FREQUENCY 5

// When the user is close to a threshold point we want to make sure that we always raise
// InteractionRatioChanged events so that we don't miss something important.
#define ALWAYS_RAISE_INTERACTION_RATIO_TOLERANCE 0.05

// This is our private implementation of the IRefreshInfoProvider interface. It is contructed by
// the ScrollViewerAdapter's Adapt method and returned as an instance of an IRefreshInfoProvider.
// It is an InteractionTrackerOwner, the corresponding InteractionTracker is maintained in the Adapter.

RefreshInfoProviderImpl::RefreshInfoProviderImpl()
{
    PTR_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);
}

RefreshInfoProviderImpl::~RefreshInfoProviderImpl()
{
    PTR_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);
}

RefreshInfoProviderImpl::RefreshInfoProviderImpl(const winrt::RefreshPullDirection& refreshPullDirection, const winrt::Size& refreshVisualizerSize, const winrt::Compositor& compositor)
{
    PTR_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);
    m_refreshPullDirection = refreshPullDirection;
    m_refreshVisualizerSize = refreshVisualizerSize;
    m_compositionProperties = compositor.CreatePropertySet();
}

void RefreshInfoProviderImpl::UpdateIsInteractingForRefresh(bool value)
{
    PTR_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);
    const bool isInteractingForRefresh = value && !m_peeking;
    if (isInteractingForRefresh != m_isInteractingForRefresh)
    {
        m_isInteractingForRefresh = isInteractingForRefresh;
        RaiseIsInteractingForRefreshChanged();
    }
}

/////////////////////////////////////////////////////
///////   IInteractionTrackerOwnerOverrides  ////////
/////////////////////////////////////////////////////
void RefreshInfoProviderImpl::ValuesChanged(const winrt::InteractionTracker& /*sender*/, const winrt::InteractionTrackerValuesChangedArgs& args)
{
    PTR_TRACE_INFO(nullptr, TRACE_MSG_METH_FLT_FLT_FLT, METH_NAME, this, args.Position().x, args.Position().y, args.Position().z);
    switch (m_refreshPullDirection)
    {
    case winrt::RefreshPullDirection::TopToBottom:
        RaiseInteractionRatioChanged(m_refreshVisualizerSize.Height == 0 ? 1.0 : std::min(1.0, (double)-args.Position().y / m_refreshVisualizerSize.Height));
        break;
    case winrt::RefreshPullDirection::BottomToTop:
        RaiseInteractionRatioChanged(m_refreshVisualizerSize.Height == 0 ? 1.0f : std::min(1.0, (double)args.Position().y / m_refreshVisualizerSize.Height));
        break;
    case winrt::RefreshPullDirection::LeftToRight:
        RaiseInteractionRatioChanged(m_refreshVisualizerSize.Width == 0 ? 1.0f : std::min(1.0, (double)-args.Position().x / m_refreshVisualizerSize.Width));
        break;
    case winrt::RefreshPullDirection::RightToLeft:
        RaiseInteractionRatioChanged(m_refreshVisualizerSize.Width == 0 ? 1.0f : std::min(1.0, (double)args.Position().x / m_refreshVisualizerSize.Width));
        break;
    default:
        MUX_ASSERT(false);
    }
}

void RefreshInfoProviderImpl::RequestIgnored(const winrt::InteractionTracker& /*sender*/, const winrt::InteractionTrackerRequestIgnoredArgs& args)
{
    PTR_TRACE_INFO(nullptr, TRACE_MSG_METH_INT, METH_NAME, this, args.RequestId());
}

void RefreshInfoProviderImpl::InteractingStateEntered(const winrt::InteractionTracker& /*sender*/, const winrt::InteractionTrackerInteractingStateEnteredArgs& args)
{
    PTR_TRACE_INFO(nullptr, TRACE_MSG_METH_INT, METH_NAME, this, args.RequestId()); 
    UpdateIsInteractingForRefresh(true);
}

void RefreshInfoProviderImpl::InertiaStateEntered(const winrt::InteractionTracker& /*sender*/, const winrt::InteractionTrackerInertiaStateEnteredArgs& args)
{
    PTR_TRACE_INFO(nullptr, TRACE_MSG_METH_INT, METH_NAME, this, args.RequestId());
    UpdateIsInteractingForRefresh(false);
}

void RefreshInfoProviderImpl::IdleStateEntered(const winrt::InteractionTracker& /*sender*/, const winrt::InteractionTrackerIdleStateEnteredArgs& args)
{
    PTR_TRACE_INFO(nullptr, TRACE_MSG_METH_INT, METH_NAME, this, args.RequestId());
}

void RefreshInfoProviderImpl::CustomAnimationStateEntered(const winrt::InteractionTracker& /*sender*/, const winrt::InteractionTrackerCustomAnimationStateEnteredArgs& args)
{
    PTR_TRACE_INFO(nullptr, TRACE_MSG_METH_INT, METH_NAME, this, args.RequestId());
}

/////////////////////////////////////////////////////
////////////   IRefreshInfoProvider  ////////////////
/////////////////////////////////////////////////////
void RefreshInfoProviderImpl::OnRefreshStarted()
{
    PTR_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);
    RaiseRefreshStarted();
}

void RefreshInfoProviderImpl::OnRefreshCompleted()
{
    PTR_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);
    RaiseRefreshCompleted();
}

winrt::event_token RefreshInfoProviderImpl::InteractionRatioChanged(const winrt::TypedEventHandler<winrt::IRefreshInfoProvider, winrt::RefreshInteractionRatioChangedEventArgs>& handler)
{
    PTR_TRACE_INFO(nullptr, TRACE_MSG_METH_STR, METH_NAME, this, L"Add Handler");
    return m_InteractionRatioChangedEventSource.add(handler);
}

void RefreshInfoProviderImpl::InteractionRatioChanged(const winrt::event_token& token)
{
    PTR_TRACE_INFO(nullptr, TRACE_MSG_METH_STR, METH_NAME, this, L"Remove Handler");
    m_InteractionRatioChangedEventSource.remove(token);
}

winrt::event_token RefreshInfoProviderImpl::IsInteractingForRefreshChanged(const winrt::TypedEventHandler<winrt::IRefreshInfoProvider, winrt::IInspectable>& handler)
{
    PTR_TRACE_INFO(nullptr, TRACE_MSG_METH_STR, METH_NAME, this, L"Add Handler");
    return m_IsInteractingForRefreshChangedEventSource.add(handler);
}

void RefreshInfoProviderImpl::IsInteractingForRefreshChanged(const winrt::event_token& token)
{
    PTR_TRACE_INFO(nullptr, TRACE_MSG_METH_STR, METH_NAME, this, L"Remove Handler");
    m_IsInteractingForRefreshChangedEventSource.remove(token);
}

winrt::event_token RefreshInfoProviderImpl::RefreshStarted(const winrt::TypedEventHandler<winrt::IRefreshInfoProvider, winrt::IInspectable>& handler)
{
    PTR_TRACE_INFO(nullptr, TRACE_MSG_METH_STR, METH_NAME, this, L"Add Handler");
    return m_RefreshStartedEventSource.add(handler);
}

void RefreshInfoProviderImpl::RefreshStarted(const winrt::event_token& token)
{
    PTR_TRACE_INFO(nullptr, TRACE_MSG_METH_STR, METH_NAME, this, L"Remove Handler");
    m_RefreshStartedEventSource.remove(token);
}

winrt::event_token RefreshInfoProviderImpl::RefreshCompleted(const winrt::TypedEventHandler<winrt::IRefreshInfoProvider, winrt::IInspectable>& handler)
{
    PTR_TRACE_INFO(nullptr, TRACE_MSG_METH_STR, METH_NAME, this, L"Add Handler");
    return m_RefreshCompletedEventSource.add(handler);
}

void RefreshInfoProviderImpl::RefreshCompleted(const winrt::event_token& token)
{
    PTR_TRACE_INFO(nullptr, TRACE_MSG_METH_STR, METH_NAME, this, L"Remove Handler");
    m_RefreshCompletedEventSource.remove(token);
}

double RefreshInfoProviderImpl::ExecutionRatio()
{
    return m_executionRatio;
}

winrt::hstring RefreshInfoProviderImpl::InteractionRatioCompositionProperty()
{
    PTR_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);
    return m_interactionRatioCompositionProperty;
}

winrt::CompositionPropertySet RefreshInfoProviderImpl::CompositionProperties()
{
    return m_compositionProperties;
}

bool RefreshInfoProviderImpl::IsInteractingForRefresh()
{
    return m_isInteractingForRefresh;
}

/////////////////////////////////////////////////////
///////////       Private Helpers       /////////////
/////////////////////////////////////////////////////
void RefreshInfoProviderImpl::RaiseInteractionRatioChanged(double interactionRatio)
{
    PTR_TRACE_INFO(nullptr, TRACE_MSG_METH_DBL, METH_NAME, this, interactionRatio);

    m_compositionProperties.InsertScalar(m_interactionRatioCompositionProperty, static_cast<float>(interactionRatio));

    if (m_interactionRatioChangedCount == 0 || AreClose(interactionRatio, 0.0) || AreClose(interactionRatio, m_executionRatio))
    {
        if (m_InteractionRatioChangedEventSource)
        {
            auto interactionRatioChangedArgs = winrt::make<RefreshInteractionRatioChangedEventArgs>(interactionRatio);
            m_InteractionRatioChangedEventSource(*this, interactionRatioChangedArgs);
        }
        m_interactionRatioChangedCount = 1;
    }
    else if (m_interactionRatioChangedCount >= RAISE_INTERACTION_RATIO_CHANGED_FREQUENCY)
    {
        m_interactionRatioChangedCount = 0;
    }
    else
    {
        m_interactionRatioChangedCount++;
    }
}

bool RefreshInfoProviderImpl::AreClose(double interactionRatio, double target)
{
    return abs(interactionRatio - target) < ALWAYS_RAISE_INTERACTION_RATIO_TOLERANCE;
}

void RefreshInfoProviderImpl::RaiseIsInteractingForRefreshChanged()
{
    PTR_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);
    m_IsInteractingForRefreshChangedEventSource(*this, nullptr);
}

void RefreshInfoProviderImpl::RaiseRefreshStarted()
{
    PTR_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);
    m_RefreshStartedEventSource(*this, nullptr);
}

void RefreshInfoProviderImpl::RaiseRefreshCompleted()
{
    PTR_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);
    m_RefreshCompletedEventSource(*this, nullptr);
}

void RefreshInfoProviderImpl::SetPeekingMode(bool peeking)
{
    PTR_TRACE_INFO(nullptr, TRACE_MSG_METH_INT, METH_NAME, this, peeking);
    m_peeking = peeking;
}
