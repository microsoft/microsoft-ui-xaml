// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ResourceAccessor.h"
#include "RatingControlAutomationPeer.h"
#include "Utils.h"

#include "RatingControlAutomationPeer.properties.cpp"

RatingControlAutomationPeer::RatingControlAutomationPeer(winrt::FrameworkElement const& owner)
    : ReferenceTracker(owner)
{
}

hstring RatingControlAutomationPeer::GetLocalizedControlTypeCore()
{
    return ResourceAccessor::GetLocalizedStringResource(SR_RatingLocalizedControlType);
}

// Properties.
bool RatingControlAutomationPeer::IsReadOnly()
{
    return GetRatingControl().IsReadOnly();
}

hstring RatingControlAutomationPeer::IValueProvider_Value()
{
    double ratingValue = GetRatingControl().Value();
    winrt::hstring valueString;

    winrt::hstring ratingString;

    if (ratingValue == -1)
    {
        double placeholderValue = GetRatingControl().PlaceholderValue();
        if (placeholderValue == -1)
        {
            valueString = ResourceAccessor::GetLocalizedStringResource(SR_RatingUnset);
        }
        else
        {
            valueString = GenerateValue_ValueString(ResourceAccessor::GetLocalizedStringResource(SR_CommunityRatingString), placeholderValue);
        }
    }
    else
    {
        valueString = GenerateValue_ValueString(ResourceAccessor::GetLocalizedStringResource(SR_BasicRatingString), ratingValue);
    }

    return valueString;
}

void RatingControlAutomationPeer::SetValue(hstring const& value)
{
    winrt::DecimalFormatter formatter;
    auto potentialRating = formatter.ParseDouble(value);
    if (potentialRating)
    {
        GetRatingControl().Value(potentialRating.Value());
    }
}

// IRangeValueProvider overrides
double RatingControlAutomationPeer::SmallChange()
{
    return 1.0;
}

double RatingControlAutomationPeer::LargeChange()
{
    return 1.0;
}

double RatingControlAutomationPeer::Maximum()
{
    return GetRatingControl().MaxRating();
}

double RatingControlAutomationPeer::Minimum()
{
    return 0;
}

double RatingControlAutomationPeer::Value()
{
    // Change this to provide a placeholder value too.
    double value = GetRatingControl().Value();
    if (value == -1)
    {
        return 0;
    }
    else
    {
        return value;
    }
}

void RatingControlAutomationPeer::SetValue(double value)
{
    GetRatingControl().Value(value);
}

//IAutomationPeerOverrides

winrt::IInspectable RatingControlAutomationPeer::GetPatternCore(winrt::PatternInterface const& patternInterface)
{
    if (patternInterface == winrt::PatternInterface::Value || patternInterface == winrt::PatternInterface::RangeValue)
    {
        return *this;
    }

    return __super::GetPatternCore(patternInterface);
}

winrt::AutomationControlType RatingControlAutomationPeer::GetAutomationControlTypeCore()
{
    return winrt::AutomationControlType::Slider;
}

// Protected methods
void RatingControlAutomationPeer::RaisePropertyChangedEvent(double newValue)
{
    // UIA doesn't tolerate a null doubles, so we convert them to zeroes.
    double oldValue = GetRatingControl().Value();
    auto oldValueProp = winrt::PropertyValue::CreateDouble(oldValue);

    if (newValue == -1)
    {
        auto newValueProp = winrt::PropertyValue::CreateDouble(0.0);
        __super::RaisePropertyChangedEvent(winrt::ValuePatternIdentifiers::ValueProperty(), oldValueProp, newValueProp);
        __super::RaisePropertyChangedEvent(winrt::RangeValuePatternIdentifiers::ValueProperty(), oldValueProp, newValueProp);
    }
    else
    {
        auto newValueProp = winrt::PropertyValue::CreateDouble(newValue);
        __super::RaisePropertyChangedEvent(winrt::ValuePatternIdentifiers::ValueProperty(), oldValueProp, newValueProp); // make these strings
        __super::RaisePropertyChangedEvent(winrt::RangeValuePatternIdentifiers::ValueProperty(), oldValueProp, newValueProp);
    }
}

// private methods

winrt::RatingControl RatingControlAutomationPeer::GetRatingControl()
{
    winrt::UIElement owner = Owner();
    return owner.as<winrt::RatingControl>();
}

int RatingControlAutomationPeer::DetermineFractionDigits(double value)
{
    value = value * 100;
    const int intValue = (int)value;

    // When reading out the Value_Value, we want clients to read out the least number of digits
    // possible. We don't want a 3 (represented as a double) to be read out as 3.00...
    // Here we determine the number of digits past the decimal point we care about,
    // and this number is used by the caller to truncate the Value_Value string.

    if (intValue % 100 == 0)
    {
        return 0;   
    }
    else if (intValue % 10 == 0)
    {
        return 1;
    }
    else
    {
        return 2;
    }
}

int RatingControlAutomationPeer::DetermineSignificantDigits(double value, int fractionDigits)
{
    int sigFigsInt = (int)value;
    int length = 0;

    while (sigFigsInt > 0)
    {
        sigFigsInt /= 10;
        length++;
    }

    return length + fractionDigits;
}

winrt::hstring RatingControlAutomationPeer::GenerateValue_ValueString(const winrt::hstring& resourceString, double ratingValue)
{
    winrt::DecimalFormatter formatter;
    winrt::SignificantDigitsNumberRounder rounder;
    formatter.NumberRounder(rounder);

    std::wstring maxRatingString = std::to_wstring(GetRatingControl().MaxRating());

    const int fractionDigits = DetermineFractionDigits(ratingValue);
    const int sigDigits = DetermineSignificantDigits(ratingValue, fractionDigits);
    formatter.FractionDigits(fractionDigits);
    rounder.SignificantDigits(sigDigits);
    winrt::hstring ratingString = formatter.Format(ratingValue);

    return StringUtil::FormatString(resourceString, ratingString.c_str(), maxRatingString.c_str());
}
