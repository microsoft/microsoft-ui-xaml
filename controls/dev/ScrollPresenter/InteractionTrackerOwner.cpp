// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "InteractionTrackerOwner.h"
#include "ScrollPresenter.h"

InteractionTrackerOwner::InteractionTrackerOwner(const winrt::ScrollPresenter& scrollPresenter)
{
    SCROLLPRESENTER_TRACE_VERBOSE(scrollPresenter, TRACE_MSG_METH_PTR, METH_NAME, this, scrollPresenter);

    m_owner = scrollPresenter;
}

InteractionTrackerOwner::~InteractionTrackerOwner()
{
    SCROLLPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);

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
        auto scrollPresenter = winrt::get_self<ScrollPresenter>(rawOwner);
        if (scrollPresenter)
        {
            scrollPresenter->ValuesChanged(args);
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
        auto scrollPresenter = winrt::get_self<ScrollPresenter>(rawOwner);
        if (scrollPresenter)
        {
            scrollPresenter->RequestIgnored(args);
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
        auto scrollPresenter = winrt::get_self<ScrollPresenter>(rawOwner);
        if (scrollPresenter)
        {
            scrollPresenter->InteractingStateEntered(args);
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
        auto scrollPresenter = winrt::get_self<ScrollPresenter>(rawOwner);
        if (scrollPresenter)
        {
            scrollPresenter->InertiaStateEntered(args);
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
        auto scrollPresenter = winrt::get_self<ScrollPresenter>(rawOwner);
        if (scrollPresenter)
        {
            scrollPresenter->IdleStateEntered(args);
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
        auto scrollPresenter = winrt::get_self<ScrollPresenter>(rawOwner);
        if (scrollPresenter)
        {
            scrollPresenter->CustomAnimationStateEntered(args);
        }
    }
}
#pragma endregion
