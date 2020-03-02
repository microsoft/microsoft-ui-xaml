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
        return *this;
    }

    return __super::GetPatternCore(patternInterface);
}

hstring ProgressRingAutomationPeer::GetClassNameCore()
{
    return winrt::hstring_name_of<winrt::ProgressRing>();
}

winrt::AutomationControlType ProgressRingAutomationPeer::GetAutomationControlTypeCore()
{
    return winrt::AutomationControlType::ProgressRing;
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
