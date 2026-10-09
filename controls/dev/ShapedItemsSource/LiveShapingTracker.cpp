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
        // stale and would otherwise leave the new item silently unsubscribed. An entry for an item
        // without weak references holds it strongly, so its address cannot have been recycled.
        if (!existing->second.CanResolveItem || existing->second.Item.get() == item)
        {
            return;
        }
        // Out of the map before revoking: a revoke calls into the item, which is app code.
        m_subscriptions.extract(existing).mapped().Revoke();
    }

    auto const handler = m_changeHandler;
    auto onChanged = [handler](winrt::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventArgs const& args)
    {
        handler(sender, args.PropertyName());
    };

    Subscription subscription{};
    if (item.try_as<::IWeakReferenceSource>())
    {
        subscription.Item = winrt::make_weak(item);
        subscription.CanResolveItem = true;
        subscription.Revoker = observable.PropertyChanged(winrt::auto_revoke, std::move(onChanged));
    }
    else
    {
        subscription.Token = observable.PropertyChanged(std::move(onChanged));
        subscription.StrongSource = observable;
    }
    m_subscriptions.emplace(identity, std::move(subscription));
}

void LiveShapingTracker::Subscription::Revoke() noexcept
{
    Revoker.revoke();
    if (auto const source = std::exchange(StrongSource, nullptr))
    {
        try
        {
            source.PropertyChanged(Token);
        }
        catch (...)
        {
            // Same as the auto-revoker: a source that fails to remove the handler is past caring.
        }
    }
}

void LiveShapingTracker::Unsubscribe(winrt::IInspectable const& item)
{
    if (auto const it = m_subscriptions.find(Identity(item)); it != m_subscriptions.end())
    {
        m_subscriptions.extract(it).mapped().Revoke();
    }
}

// Revoking calls the item's remove_PropertyChanged, which is app code and may re-enter this
// tracker. Every path below therefore takes entries out of the map first and revokes afterwards,
// so no iterator into m_subscriptions is live while app code runs.
void LiveShapingTracker::RetainOnly(std::unordered_set<void const*> const& live)
{
    std::vector<Subscription> departed;
    for (auto it = m_subscriptions.begin(); it != m_subscriptions.end();)
    {
        if (live.count(it->first) != 0)
        {
            ++it;
            continue;
        }
        departed.push_back(std::move(m_subscriptions.extract(it++).mapped()));
    }

    for (auto& subscription : departed)
    {
        subscription.Revoke();
    }
}

void LiveShapingTracker::UnsubscribeAll() noexcept
{
    auto subscriptions = std::exchange(m_subscriptions, {});
    for (auto& [identity, subscription] : subscriptions)
    {
        UNREFERENCED_PARAMETER(identity);
        subscription.Revoke();
    }
}
