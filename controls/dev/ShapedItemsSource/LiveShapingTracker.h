// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <winrt/Microsoft.UI.Xaml.Data.h>
#include <winrt/Windows.Foundation.h>

// Per-item INotifyPropertyChanged subscriptions for live shaping. Items are held weakly so dropped
// items are not pinned; the weak ref also lets Subscribe detect a recycled ABI address. Items
// without weak ref support are held strongly and revoked by token.
class LiveShapingTracker
{
public:
    using ChangeHandler = std::function<void(
        winrt::IInspectable const& item,
        winrt::hstring const& propertyName)>;

    void SetChangeHandler(ChangeHandler handler) { m_changeHandler = std::move(handler); }

    // Idempotent: re-subscribing an already-tracked item is a no-op.
    void Subscribe(winrt::IInspectable const& item);
    void Unsubscribe(winrt::IInspectable const& item);
    // Revokes every subscription whose item is not in `live`.
    void RetainOnly(std::unordered_set<void const*> const& live);
    void UnsubscribeAll() noexcept;
    bool Empty() const noexcept { return m_subscriptions.empty(); }
    static void const* Identity(winrt::IInspectable const& item);

private:
    struct Subscription
    {
        winrt::weak_ref<winrt::IInspectable> Item{ nullptr };
        // False when the item has no IWeakReferenceSource (Item is empty).
        bool CanResolveItem{ false };
        winrt::Microsoft::UI::Xaml::Data::INotifyPropertyChanged::PropertyChanged_revoker Revoker{};
        // Only for an item without weak reference support (Revoker is empty then).
        winrt::Microsoft::UI::Xaml::Data::INotifyPropertyChanged StrongSource{ nullptr };
        winrt::event_token Token{};

        Subscription() = default;
        Subscription(Subscription const&) = delete;
        Subscription& operator=(Subscription const&) = delete;
        Subscription(Subscription&&) noexcept = default;
        // Entries live in a node-based map and are never reassigned.
        Subscription& operator=(Subscription&&) = delete;
        // Backstop for the strong path, which has no auto-revoker: a dropped entry must not leave
        // its handler attached. A moved-from or revoked entry has nothing left to revoke.
        ~Subscription() { Revoke(); }

        void Revoke() noexcept;
    };

    std::unordered_map<void const*, Subscription> m_subscriptions;
    ChangeHandler m_changeHandler;
};
