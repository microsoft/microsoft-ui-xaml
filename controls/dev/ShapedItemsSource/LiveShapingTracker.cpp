// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "LiveShapingTracker.h"

void const* LiveShapingTracker::Identity(winrt::IInspectable const& item)
{
    if (!item)
    {
        return nullptr;
    }

    auto const unknown = item.as<winrt::Windows::Foundation::IUnknown>();
    return winrt::get_abi(unknown);
}

void LiveShapingTracker::Subscribe(winrt::IInspectable const& item)
{
    if (!item || !m_changeHandler)
    {
        return;
    }

    auto const observable = item.try_as<winrt::Microsoft::UI::Xaml::Data::INotifyPropertyChanged>();
    if (!observable)
    {
        return;
    }

    auto const identity = Identity(item);
    if (!identity)
    {
        return;
    }

    if (auto const existing = m_subscriptions.find(identity); existing != m_subscriptions.end())
    {
        // Already tracking this address. The key is a raw ABI pointer, so an address freed by a
        // dead item can be handed to a new one -- an entry that resolves to a DIFFERENT object is
        // stale and would otherwise leave the new item silently unsubscribed. When the item does
        // not support weak references there is nothing to compare against, so the entry stands.
        if (!existing->second.CanResolveItem || existing->second.Item.get() == item)
        {
            return;
        }
        existing->second.Revoker.revoke();
        m_subscriptions.erase(existing);
    }

    auto const handler = m_changeHandler;
    Subscription subscription{};
    if (item.try_as<::IWeakReferenceSource>())
    {
        subscription.Item = winrt::make_weak(item);
        subscription.CanResolveItem = true;
    }
    subscription.Revoker = observable.PropertyChanged(
        winrt::auto_revoke,
        [handler](winrt::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventArgs const& args)
        {
            handler(sender, args.PropertyName());
        });
    m_subscriptions.emplace(identity, std::move(subscription));
}

void LiveShapingTracker::Unsubscribe(winrt::IInspectable const& item)
{
    if (auto const it = m_subscriptions.find(Identity(item)); it != m_subscriptions.end())
    {
        it->second.Revoker.revoke();
        m_subscriptions.erase(it);
    }
}

void LiveShapingTracker::RetainOnly(std::unordered_set<void const*> const& live)
{
    for (auto it = m_subscriptions.begin(); it != m_subscriptions.end();)
    {
        if (live.count(it->first) != 0)
        {
            ++it;
            continue;
        }
        it->second.Revoker.revoke();
        it = m_subscriptions.erase(it);
    }
}

void LiveShapingTracker::UnsubscribeAll() noexcept
{
    for (auto& [identity, subscription] : m_subscriptions)
    {
        UNREFERENCED_PARAMETER(identity);
        subscription.Revoker.revoke();
    }
    m_subscriptions.clear();
}
