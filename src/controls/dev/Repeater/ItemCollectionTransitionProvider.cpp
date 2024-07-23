// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include "ItemsRepeater.common.h"
#include "ItemCollectionTransitionProvider.h"
#include "ItemCollectionTransitionCompletedEventArgs.h"
#include "ItemCollectionTransitionProvider.properties.cpp"

#pragma region IItemCollectionTransitionProvider

ItemCollectionTransitionProvider::~ItemCollectionTransitionProvider()
{
    for (auto const& keepAliveTimerBatchPair : m_keepAliveTimersMap)
    {
        if (auto const& keepAliveTimer = keepAliveTimerBatchPair.first)
        {
            keepAliveTimer.Stop();
        }
    }
}

void ItemCollectionTransitionProvider::QueueTransition(winrt::ItemCollectionTransition const& transition)
{
    // When usesNewTransitionsBatch remains 'false', the provided transition is added to the current batch
    // identified by the current m_transitionsBatch value.
    bool usesNewTransitionsBatch = false;

    if (!m_rendering)
    {
        // This marks the beginning of a new batch of transitions.
        usesNewTransitionsBatch = true;
        // The batch gets a new id.
        m_transitionsBatch++;
        m_rendering = winrt::CompositionTarget::Rendering(winrt::auto_revoke, { this, &ItemCollectionTransitionProvider::OnRendering });
    }

    // We'll animate if animations are enabled and the transition provider has indicated we should animate this transition.
    if (SharedHelpers::IsAnimationsEnabled() && ShouldAnimate(transition))
    {
        if (usesNewTransitionsBatch)
        {
            // Allocating a new array of ItemCollectionTransition with animations for this new batch.
            const auto batchTransitionsPair = std::make_pair(m_transitionsBatch, winrt::single_threaded_vector<winrt::ItemCollectionTransition>());

            m_transitionsWithAnimationsMap.insert(batchTransitionsPair);
        }

        const auto transitionsWithAnimations = m_transitionsWithAnimationsMap[m_transitionsBatch];

        transitionsWithAnimations.Append(transition);
    }

    // To ensure proper VirtualizationInfo ordering, we still need to raise TransitionCompleted in a
    // CompositionTarget.Rendering handler, even if we aren't going to animate anything for this transition.
    if (usesNewTransitionsBatch)
    {
        // Allocating a new array of ItemCollectionTransition for this new batch.
        const auto batchTransitionsPair = std::make_pair(m_transitionsBatch, winrt::single_threaded_vector<winrt::ItemCollectionTransition>());

        m_transitionsMap.insert(batchTransitionsPair);
    }

    const auto transitions = m_transitionsMap[m_transitionsBatch];

    transitions.Append(transition);
}

bool ItemCollectionTransitionProvider::ShouldAnimate(winrt::ItemCollectionTransition const& transition)
{
    return overridable().ShouldAnimateCore(transition);
}

#pragma endregion

#pragma region IItemCollectionTransitionProviderOverrides

void ItemCollectionTransitionProvider::StartTransitions(winrt::IVector<winrt::ItemCollectionTransition> const& /* transitions */)
{
    throw winrt::hresult_not_implemented();
}

bool ItemCollectionTransitionProvider::ShouldAnimateCore(winrt::ItemCollectionTransition const& /* transition */)
{
    throw winrt::hresult_not_implemented();
}

#pragma endregion

void ItemCollectionTransitionProvider::NotifyTransitionCompleted(winrt::ItemCollectionTransition const& transition)
{
    m_transitionCompletedEventSource(*this, winrt::make<ItemCollectionTransitionCompletedEventArgs>(transition));
}

void ItemCollectionTransitionProvider::CleanTransitionsBatch()
{
    // Called when none of a batch's transitions required a timer to keep them alive.
    // No completion notification is expected and all transitions can be released.
    m_transitionsWithAnimationsMap.erase(m_transitionsBatch);
    m_transitionsMap.erase(m_transitionsBatch);
}

void ItemCollectionTransitionProvider::OnKeepAliveTimerTick(winrt::IInspectable const& sender, winrt::IInspectable const& /*args*/)
{
    // By the time this timer expires, all transitions associated with it are considered completed and they no longer need to be kept alive for
    // ItemCollectionTransitionProgress::Complete() to successfully trigger a ItemCollectionTransitionProvider::NotifyTransitionCompleted call.
    const auto keepAliveTimer = sender.try_as<winrt::DispatcherTimer>();

    if (keepAliveTimer)
    {
        // The timer is stopped, all its associated transitions are released and so is the timer itself.
        keepAliveTimer.Stop();

        const auto keepAliveTimersIterator = m_keepAliveTimersMap.find(keepAliveTimer);

        if (keepAliveTimersIterator == m_keepAliveTimersMap.end())
        {
            return;
        }

        const auto transitionsBatch = keepAliveTimersIterator->second;

        m_transitionsWithAnimationsMap.erase(transitionsBatch);
        m_transitionsMap.erase(transitionsBatch);

        m_keepAliveTimersMap.erase(keepAliveTimersIterator);
    }
}

void ItemCollectionTransitionProvider::OnRendering(winrt::IInspectable const& /*sender*/, winrt::IInspectable const& /*args*/)
{
    m_rendering.revoke();

    bool keepAliveTimerRequired = false;

    auto scopeGuard = gsl::finally([this, &keepAliveTimerRequired]()
    {
        if (keepAliveTimerRequired)
        {
            // At least one transition in the batch requires a new 'keep alive' timer.
            StartNewKeepAliveTimer();
        }
        else
        {
            // None of the transitions in the batch requires a 'keep alive' timer. Simply discard them all.
            CleanTransitionsBatch();
        }
    });

    const auto transitionsWithAnimations = m_transitionsWithAnimationsMap[m_transitionsBatch];

    if (transitionsWithAnimations)
    {
        overridable().StartTransitions(transitionsWithAnimations);
    }

    // We'll automatically raise TransitionCompleted on all of the transitions that were not actually animated
    // in order to guarantee that every transition queued receives a corresponding TransitionCompleted event.
    const auto transitions = m_transitionsMap[m_transitionsBatch];

    if (transitions)
    {
        for (auto const& transition : transitions)
        {
            if (transition.HasStarted())
            {
                keepAliveTimerRequired = true;
            }
            else
            {
                NotifyTransitionCompleted(transition);
            }
        }
    }
}

void ItemCollectionTransitionProvider::StartNewKeepAliveTimer()
{
    // This timer delays the release of the batch's transitions so that ItemCollectionTransitionProgress::Complete()
    // which uses transition weak references is able to trigger an ItemCollectionTransitionProvider::NotifyTransitionCompleted
    // call for completion. This is important for TransitionManager::OnTransitionProviderTransitionCompleted to be able to
    // update the ItemsRepeater's items ownership.
    MUX_ASSERT(m_transitionsMap[m_transitionsBatch].Size() > 0);

    auto timerDuration = 5s;
    const auto keepAliveTimer = winrt::DispatcherTimer();

    keepAliveTimer.Interval(winrt::TimeSpan::duration(timerDuration));
    keepAliveTimer.Tick({ this, &ItemCollectionTransitionProvider::OnKeepAliveTimerTick });
    keepAliveTimer.Start();

    const auto keepAliveTimerBatchPair = std::make_pair(keepAliveTimer, m_transitionsBatch);

    m_keepAliveTimersMap.insert(keepAliveTimerBatchPair);
}
