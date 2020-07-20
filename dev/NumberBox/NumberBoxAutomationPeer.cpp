// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ResourceAccessor.h"
#include "NumberBox.h"
#include "NumberBoxAutomationPeer.h"
#include "Utils.h"

#include "NumberBoxAutomationPeer.properties.cpp"

NumberBoxAutomationPeer::NumberBoxAutomationPeer(winrt::NumberBoxTextBox const& owner)
    : ReferenceTracker(owner)
{
    // ### get NumberBox from owner.
}

// IAutomationPeerOverrides
winrt::IInspectable NumberBoxAutomationPeer::GetPatternCore(winrt::PatternInterface const& patternInterface)
{
    if (patternInterface == winrt::PatternInterface::RangeValue)
    {
        return *this;
    }

    return __super::GetPatternCore(patternInterface);
}

hstring NumberBoxAutomationPeer::GetClassNameCore()
{
    return winrt::hstring_name_of<winrt::NumberBox>();
}

hstring NumberBoxAutomationPeer::GetNameCore()
{
    winrt::hstring name = __super::GetNameCore();

    // ### this needs to be fixed

    if (name.empty())
    {
        if (auto numberBox = Owner().try_as<winrt::NumberBox>())
        {
            name = SharedHelpers::TryGetStringRepresentationFromObject(numberBox.Header());
        }
    }

    return name;
}

winrt::AutomationControlType NumberBoxAutomationPeer::GetAutomationControlTypeCore()
{
    return winrt::AutomationControlType::Spinner;
}

com_ptr<NumberBox> NumberBoxAutomationPeer::GetImpl()
{
    com_ptr<NumberBox> impl = nullptr;

    if (auto numberBoxTextBox = Owner().try_as<winrt::NumberBoxTextBox>())
    {
        auto numberBox = SharedHelpers::GetAncestorOfType<winrt::NumberBox>(winrt::VisualTreeHelper::GetParent(numberBoxTextBox));
        if (numberBox)
        {
            impl = winrt::get_self<NumberBox>(numberBox)->get_strong();
        }

    }

    return impl;
}

// IRangeValueProvider
double NumberBoxAutomationPeer::Minimum()
{
    return GetImpl()->Minimum();
}

double NumberBoxAutomationPeer::Maximum()
{
    return GetImpl()->Maximum();
}

double NumberBoxAutomationPeer::Value()
{
    return GetImpl()->Value();
}

double NumberBoxAutomationPeer::SmallChange()
{
    return GetImpl()->SmallChange();
}

double NumberBoxAutomationPeer::LargeChange()
{
    return GetImpl()->LargeChange();
}

void NumberBoxAutomationPeer::SetValue(double value)
{
    GetImpl()->Value(value);
}

void NumberBoxAutomationPeer::RaiseValueChangedEvent(double oldValue, double newValue)
{
    __super::RaisePropertyChangedEvent(winrt::RangeValuePatternIdentifiers::ValueProperty(),
        winrt::PropertyValue::CreateDouble(oldValue),
        winrt::PropertyValue::CreateDouble(newValue));
}
