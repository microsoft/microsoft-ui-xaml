// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "RatingControl.h"

#include "RatingControlAutomationPeer.g.h"

// Needed because IValueProvider.Value and IRangeValueProvider.Value conflict on their return type, this level of indirection enables it to work.
template <typename Owner>
struct ValueProviderThunk : winrt::implements<ValueProviderThunk<Owner>, winrt::IValueProvider, winrt::composable, winrt::composing>
{
    bool IsReadOnly() { return static_cast<Owner*>(this)->IsReadOnly(); }
    hstring Value() { return static_cast<Owner*>(this)->IValueProvider_Value(); }
    void SetValue(hstring const& value) { static_cast<Owner*>(this)->SetValue(value); }
};

class RatingControlAutomationPeer : 
    public ReferenceTracker<
        RatingControlAutomationPeer,
        winrt::implementation::RatingControlAutomationPeerT,
        ValueProviderThunk<RatingControlAutomationPeer>,
        winrt::IRangeValueProvider>
{

public:
    RatingControlAutomationPeer(winrt::FrameworkElement const& owner);

    // IAutomationPeerOverrides
    hstring GetLocalizedControlTypeCore();
    
    // IValueProvider properties
    bool IsReadOnly();
    hstring IValueProvider_Value();
    // Methods
    void SetValue(hstring const& value);

    // IRangeValueProvider properties
    // IsReadOnly shared with IValueProvider
    double SmallChange();
    double LargeChange();
    double Maximum();
    double Minimum();
    double Value();
    // Methods
    void SetValue(double value);

    // IAutomationPeerOverrides 
    winrt::IInspectable GetPatternCore(winrt::PatternInterface const& patternInterface);
    winrt::AutomationControlType GetAutomationControlTypeCore();

    void RaisePropertyChangedEvent(double newValue);

private:
    winrt::RatingControl GetRatingControl();

    int DetermineFractionDigits(double value);
    int DetermineSignificantDigits(double value, int fractionDigits);

    winrt::hstring GenerateValue_ValueString(const winrt::hstring& resourceString, double ratingValue);
};

