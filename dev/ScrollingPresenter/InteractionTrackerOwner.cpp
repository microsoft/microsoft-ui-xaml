// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "InteractionTrackerOwner.h"
#include "ScrollingPresenter.h"

InteractionTrackerOwner::InteractionTrackerOwner(const winrt::ScrollingPresenter& scrollingPresenter)
{
    SCROLLER_TRACE_VERBOSE(scrollingPresenter, TRACE_MSG_METH_PTR, METH_NAME, this, scrollingPresenter);

    m_owner = scrollingPresenter;
}

InteractionTrackerOwner::~InteractionTrackerOwner()
{
    SCROLLER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);

    m_owner = nullptr;
}

#pragma region IInteractionTrackerOwner
void InteractionTrackerOwner::ValuesChanged(
    const winrt::InteractionTracker& /*sender*/, const winrt::InteractionTrackerValuesChangedArgs& args)
{
    if (!m_owner)
    {
        return;
    }

    if (auto rawOwner = m_owner.get())
    {
        auto scrollingPresenter = winrt::get_self<ScrollingPresenter>(rawOwner);
        if (scrollingPresenter)
        {
            scrollingPresenter->ValuesChanged(args);
        }
    }
}

void InteractionTrackerOwner::RequestIgnored(
    const winrt::InteractionTracker& /*sender*/, const winrt::InteractionTrackerRequestIgnoredArgs& args)
{
    if (!m_owner)
    {
        return;
    }

    if (auto rawOwner = m_owner.get())
    {
        auto scrollingPresenter = winrt::get_self<ScrollingPresenter>(rawOwner);
        if (scrollingPresenter)
        {
            scrollingPresenter->RequestIgnored(args);
        }
    }
}

void InteractionTrackerOwner::InteractingStateEntered(
    const winrt::InteractionTracker& /*sender*/, const winrt::InteractionTrackerInteractingStateEnteredArgs& args)
{
    if (!m_owner)
    {
        return;
    }

    if (auto rawOwner = m_owner.get())
    {
        auto scrollingPresenter = winrt::get_self<ScrollingPresenter>(rawOwner);
        if (scrollingPresenter)
        {
            scrollingPresenter->InteractingStateEntered(args);
        }
    }
}

void InteractionTrackerOwner::InertiaStateEntered(
    const winrt::InteractionTracker& /*sender*/, const winrt::InteractionTrackerInertiaStateEnteredArgs& args)
{
    if (!m_owner)
    {
        return;
    }

    if (auto rawOwner = m_owner.get())
    {
        auto scrollingPresenter = winrt::get_self<ScrollingPresenter>(rawOwner);
        if (scrollingPresenter)
        {
            scrollingPresenter->InertiaStateEntered(args);
        }
    }
}

void InteractionTrackerOwner::IdleStateEntered(
    const winrt::InteractionTracker& /*sender*/, const winrt::InteractionTrackerIdleStateEnteredArgs& args)
{
    if (!m_owner)
    {
        return;
    }

    if (auto rawOwner = m_owner.get())
    {
        auto scrollingPresenter = winrt::get_self<ScrollingPresenter>(rawOwner);
        if (scrollingPresenter)
        {
            scrollingPresenter->IdleStateEntered(args);
        }
    }
}

void InteractionTrackerOwner::CustomAnimationStateEntered(
    const winrt::InteractionTracker& /*sender*/, const winrt::InteractionTrackerCustomAnimationStateEnteredArgs& args)
{
    if (!m_owner)
    {
        return;
    }

    if (auto rawOwner = m_owner.get())
    {
        auto scrollingPresenter = winrt::get_self<ScrollingPresenter>(rawOwner);
        if (scrollingPresenter)
        {
            scrollingPresenter->CustomAnimationStateEntered(args);
        }
    }
}
#pragma endregion
