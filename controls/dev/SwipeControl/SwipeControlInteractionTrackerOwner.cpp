// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "SwipeControlInteractionTrackerOwner.h"
#include "SwipeControl.h"

SwipeControlInteractionTrackerOwner::SwipeControlInteractionTrackerOwner(const winrt::SwipeControl& swipeControl)
{
    SWIPECONTROL_TRACE_VERBOSE(swipeControl, TRACE_MSG_METH_PTR, METH_NAME, this, swipeControl);

    m_owner = swipeControl;
}

SwipeControlInteractionTrackerOwner::~SwipeControlInteractionTrackerOwner()
{
    SWIPECONTROL_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);

    m_owner = nullptr;
}

#pragma region IInteractionTrackerOwner
void SwipeControlInteractionTrackerOwner::ValuesChanged(
    const winrt::InteractionTracker& /*sender*/, const winrt::InteractionTrackerValuesChangedArgs& args)
{
    if (!m_owner)
    {
        return;
    }

    if (auto rawOwner = m_owner.get())
    {
        auto swipeControl = winrt::get_self<SwipeControl>(rawOwner);
        if (swipeControl)
        {
            swipeControl->ValuesChanged(args);
        }
    }
}

void SwipeControlInteractionTrackerOwner::RequestIgnored(
    const winrt::InteractionTracker& /*sender*/, const winrt::InteractionTrackerRequestIgnoredArgs& args)
{
    if (!m_owner)
    {
        return;
    }

    if (auto rawOwner = m_owner.get())
    {
        auto swipeControl = winrt::get_self<SwipeControl>(rawOwner);
        if (swipeControl)
        {
            swipeControl->RequestIgnored(args);
        }
    }
}

void SwipeControlInteractionTrackerOwner::InteractingStateEntered(
    const winrt::InteractionTracker& /*sender*/, const winrt::InteractionTrackerInteractingStateEnteredArgs& args)
{
    if (!m_owner)
    {
        return;
    }

    if (auto rawOwner = m_owner.get())
    {
        auto swipeControl = winrt::get_self<SwipeControl>(rawOwner);
        if (swipeControl)
        {
            swipeControl->InteractingStateEntered(args);
        }
    }
}

void SwipeControlInteractionTrackerOwner::InertiaStateEntered(
    const winrt::InteractionTracker& /*sender*/, const winrt::InteractionTrackerInertiaStateEnteredArgs& args)
{
    if (!m_owner)
    {
        return;
    }

    if (auto rawOwner = m_owner.get())
    {
        auto swipeControl = winrt::get_self<SwipeControl>(rawOwner);
        if (swipeControl)
        {
            swipeControl->InertiaStateEntered(args);
        }
    }
}

void SwipeControlInteractionTrackerOwner::IdleStateEntered(
    const winrt::InteractionTracker& /*sender*/, const winrt::InteractionTrackerIdleStateEnteredArgs& args)
{
    if (!m_owner)
    {
        return;
    }

    if (auto rawOwner = m_owner.get())
    {
        auto swipeControl = winrt::get_self<SwipeControl>(rawOwner);
        if (swipeControl)
        {
            swipeControl->IdleStateEntered(args);
        }
    }
}

void SwipeControlInteractionTrackerOwner::CustomAnimationStateEntered(
    const winrt::InteractionTracker& /*sender*/, const winrt::InteractionTrackerCustomAnimationStateEnteredArgs& args)
{
    if (!m_owner)
    {
        return;
    }

    if (auto rawOwner = m_owner.get())
    {
        auto swipeControl = winrt::get_self<SwipeControl>(rawOwner);
        if (swipeControl)
        {
            swipeControl->CustomAnimationStateEntered(args);
        }
    }
}
#pragma endregion
