// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ResourceAccessor.h"
#include "ProgressBar.h"
#include "ProgressBarAutomationPeer.h"
#include "Utils.h"

ProgressBarAutomationPeer::ProgressBarAutomationPeer(winrt::ProgressBar const& owner)
    : ReferenceTracker(owner)
{
}

// IAutomationPeerOverrides
winrt::IInspectable ProgressBarAutomationPeer::GetPatternCore(winrt::PatternInterface const& patternInterface)
{
    if (patternInterface == winrt::PatternInterface::RangeValue)
    {
        return *this;
    }

    return __super::GetPatternCore(patternInterface);
}

hstring ProgressBarAutomationPeer::GetClassNameCore()
{
    return winrt::hstring_name_of<winrt::ProgressBar>();
}

winrt::AutomationControlType ProgressBarAutomationPeer::GetAutomationControlTypeCore()
{
    return winrt::AutomationControlType::ProgressBar;
}

com_ptr<ProgressBar> ProgressBarAutomationPeer::GetImpl()
{
    com_ptr<ProgressBar> impl = nullptr;

    if (auto progressBar = Owner().try_as<winrt::ProgressBar>())
    {
        impl = winrt::get_self<ProgressBar>(progressBar)->get_strong();
    }

    return impl;
}

// IRangeValueProvider
double ProgressBarAutomationPeer::Value()
{
    return GetImpl()->Value();
}

double ProgressBarAutomationPeer::SmallChange()
{
    return NAN;
}

double ProgressBarAutomationPeer::LargeChange()
{
    return NAN;
}

double ProgressBarAutomationPeer::Minimum()
{
    return GetImpl()->Minimum();
}

double ProgressBarAutomationPeer::Maximum()
{
    return GetImpl()->Maximum();
}

void ProgressBarAutomationPeer::SetValue(double value)
{
    GetImpl()->Value(value);
}
