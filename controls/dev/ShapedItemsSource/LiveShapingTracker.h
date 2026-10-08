// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <functional>
#include <unordered_map>
#include <unordered_set>

#include <winrt/Microsoft.UI.Xaml.Data.h>
#include <winrt/Windows.Foundation.h>

// Per-item INotifyPropertyChanged subscriptions for live shaping.
//
// Holds each item WEAKLY. The auto-revoker already keeps only a weak reference to its sender, so
// the subscription entry was the sole strong reference; keeping one here would pin every item the
// source has already dropped. The weak reference is not merely absence of ownership -- it is what
// lets Subscribe detect a recycled ABI address (see the .cpp).
class LiveShapingTracker
{
public:
    using ChangeHandler = std::function<void(
        winrt::IInspectable const& item,
        winrt::hstring const& propertyName)>;

    void SetChangeHandler(ChangeHandler handler) { m_changeHandler = std::move(handler); }

    // Idempotent: re-subscribing an already-tracked item is a no-op, so callers can subscribe
    // defensively without paying a revoke/re-add round trip.
    void Subscribe(winrt::IInspectable const& item);
    void Unsubscribe(winrt::IInspectable const& item);
    // Sweep half of a mark-and-sweep reconcile: revoke every subscription whose item is not in
    // `live`. Retained entries keep their existing revoker untouched.
    void RetainOnly(std::unordered_set<void const*> const& live);
    void UnsubscribeAll() noexcept;

private:
    struct Subscription
    {
        winrt::weak_ref<winrt::IInspectable> Item{ nullptr };
        // False when the item does not support IWeakReferenceSource, in which case Item is empty
        // and cannot be used to validate the entry.
        bool CanResolveItem{ false };
        winrt::Microsoft::UI::Xaml::Data::INotifyPropertyChanged::PropertyChanged_revoker Revoker{};
    };

    static void const* Identity(winrt::IInspectable const& item);

    std::unordered_map<void const*, Subscription> m_subscriptions;
    ChangeHandler m_changeHandler;
};
