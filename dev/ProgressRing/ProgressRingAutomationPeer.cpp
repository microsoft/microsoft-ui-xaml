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

winrt::IInspectable ProgressRingAutomationPeer::GetPatternCore(winrt::PatternInterface const& patternInterface)
{
    if (patternInterface == winrt::PatternInterface::RangeValue)
    {
        if (GetImpl()->IsIndeterminate())
        {
            return nullptr;
        }
        else
        {
            return *this;
        }
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
        if (progressRing.IsActive())
        {
            if (progressRing.IsIndeterminate())
            {
                return winrt::hstring{ ResourceAccessor::GetLocalizedStringResource(SR_ProgressRingIndeterminateStatus) + L" " + name };
            }
            else
            {
                return name;
            }
        }
    }
    return name;
}

winrt::AutomationControlType ProgressRingAutomationPeer::GetAutomationControlTypeCore()
{
    return winrt::AutomationControlType::ProgressBar;
}

winrt::hstring ProgressRingAutomationPeer::GetLocalizedControlTypeCore()
{
    return ResourceAccessor::GetLocalizedStringResource(SR_ProgressRingName);
}

// IRangeValueProvider
double ProgressRingAutomationPeer::Minimum()
{
    return GetImpl()->Minimum();
}

double ProgressRingAutomationPeer::Maximum()
{
    return GetImpl()->Maximum();
}

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

void ProgressRingAutomationPeer::SetValue(double value)
{
    GetImpl()->Value(value);
}

com_ptr<ProgressRing> ProgressRingAutomationPeer::GetImpl()
{
    com_ptr<ProgressRing> impl = nullptr;

    if (auto numberBox = Owner().try_as<winrt::ProgressRing>())
    {
        impl = winrt::get_self<ProgressRing>(numberBox)->get_strong();
    }

    return impl;
}
