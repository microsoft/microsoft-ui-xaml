// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ResourceAccessor.h"
#include "ProgressRing.h"
#include "ProgressRingAutomationPeer.h"
#include "Utils.h"

#include "ProgressRingAutomationPeer.properties.cpp"

ProgressRingAutomationPeer::ProgressRingAutomationPeer(winrt::ProgressRing const& owner)
    : ReferenceTracker(owner)
{
}

// IAutomationPeerOverrides
winrt::IInspectable ProgressRingAutomationPeer::GetPatternCore(winrt::PatternInterface const& patternInterface)
{
    if (patternInterface == winrt::PatternInterface::RangeValue)
    {
        if (auto progressRing = Owner().try_as<winrt::ProgressRing>())
        {
            if (progressRing.IsIndeterminate())
            {
                return nullptr;
            }
        }

        return *this;
    }

     return __super::GetPatternCore(patternInterface);
}

winrt::hstring ProgressRingAutomationPeer::GetClassNameCore()
{
    return winrt::hstring_name_of<winrt::ProgressRing>();
}

winrt::hstring ProgressRingAutomationPeer::GetNameCore()
{
    //Check to see if the item has a defined AutomationProperties.Name
    winrt::hstring name = __super::GetNameCore();

    if (auto progressRing = Owner().try_as<winrt::ProgressRing>())
    {
        if (progressRing.IsIndeterminate())
        {
            return winrt::hstring{ ResourceAccessor::GetLocalizedStringResource(SR_ProgressRingIndeterminateStatus) + name };
        }
    }
    return name;
}

winrt::AutomationControlType ProgressRingAutomationPeer::GetAutomationControlTypeCore()
{
    return winrt::AutomationControlType::ProgressBar;
}

com_ptr<ProgressRing> ProgressRingAutomationPeer::GetImpl()
{
    com_ptr<ProgressRing> impl = nullptr;

    if (auto progressRing = Owner().try_as<winrt::ProgressRing>())
    {
        impl = winrt::get_self<ProgressRing>(progressRing)->get_strong();
    }

    return impl;
}

// IRangeValueProvider	
double ProgressRingAutomationPeer::Value()
{
    return GetImpl()->Value();
}

double ProgressRingAutomationPeer::SmallChange()
{
    return std::numeric_limits<double>::quiet_NaN();
}

double ProgressRingAutomationPeer::LargeChange()
{
    return std::numeric_limits<double>::quiet_NaN();
}

double ProgressRingAutomationPeer::Minimum()
{
    return GetImpl()->Minimum();
}

double ProgressRingAutomationPeer::Maximum()
{
    return GetImpl()->Maximum();
}

void ProgressRingAutomationPeer::SetValue(double value)
{
    GetImpl()->Value(value);
}
