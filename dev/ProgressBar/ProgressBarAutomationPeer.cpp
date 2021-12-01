// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ResourceAccessor.h"
#include "ProgressBar.h"
#include "ProgressBarAutomationPeer.h"
#include "Utils.h"

#include "ProgressBarAutomationPeer.properties.cpp"

ProgressBarAutomationPeer::ProgressBarAutomationPeer(winrt::ProgressBar const& owner)
    : ReferenceTracker(owner)
{
}

// IAutomationPeerOverrides
winrt::IInspectable ProgressBarAutomationPeer::GetPatternCore(winrt::PatternInterface const& patternInterface)
{
    if (patternInterface == winrt::PatternInterface::RangeValue)
    {
        if (auto progressBar = Owner().try_as<winrt::ProgressBar>())
        {
            if (progressBar.IsIndeterminate())
            {
                return nullptr;
            }
        }

        return *this;
    }

    return __super::GetPatternCore(patternInterface);
}

hstring ProgressBarAutomationPeer::GetClassNameCore()
{
    return winrt::hstring_name_of<winrt::ProgressBar>();
}

winrt::hstring ProgressBarAutomationPeer::GetNameCore()
{
    //Check to see if the item has a defined AutomationProperties.Name
    winrt::hstring name = __super::GetNameCore();

    if (auto progressBar = Owner().try_as<winrt::ProgressBar>())
    {
        if (progressBar.ShowError())
        {
            return winrt::hstring{ ResourceAccessor::GetLocalizedStringResource(SR_ProgressBarErrorStatus) + name };
        }
        else if (progressBar.ShowPaused())
        {
            return winrt::hstring{ ResourceAccessor::GetLocalizedStringResource(SR_ProgressBarPausedStatus) + name };
        }
        else if (progressBar.IsIndeterminate())
        {
            return winrt::hstring{ ResourceAccessor::GetLocalizedStringResource(SR_ProgressBarIndeterminateStatus) + name };
        }
    }
    return name;
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
    return std::numeric_limits<double>::quiet_NaN();
}

double ProgressBarAutomationPeer::LargeChange()
{
    return std::numeric_limits<double>::quiet_NaN();
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
