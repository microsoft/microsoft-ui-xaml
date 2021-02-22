// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "BreadcrumbItem.h"
#include "BreadcrumbItemAutomationPeer.h"
#include "BreadcrumbItemAutomationPeer.properties.cpp"

BreadcrumbItemAutomationPeer::BreadcrumbItemAutomationPeer(winrt::BreadcrumbItem const& owner)
    : ReferenceTracker(owner)
{
}

BreadcrumbItemAutomationPeer::BreadcrumbItemAutomationPeer(winrt::BreadcrumbDropDownItem const& owner)
    : ReferenceTracker(owner)
{
}

// IAutomationPeerOverrides
winrt::IInspectable BreadcrumbItemAutomationPeer::GetPatternCore(winrt::PatternInterface const& patternInterface)
{
    if (patternInterface == winrt::PatternInterface::Invoke)
    {
        return *this;
    }

    return __super::GetPatternCore(patternInterface);
}

hstring BreadcrumbItemAutomationPeer::GetClassNameCore()
{
    return winrt::hstring_name_of<winrt::BreadcrumbItem>();
}

winrt::AutomationControlType BreadcrumbItemAutomationPeer::GetAutomationControlTypeCore()
{
    return winrt::AutomationControlType::Button;
}

com_ptr<BreadcrumbItem> BreadcrumbItemAutomationPeer::GetImpl()
{
    com_ptr<BreadcrumbItem> impl = nullptr;
    
    if (auto breadcrumbItem = Owner().try_as<winrt::BreadcrumbItem>())
    {
        impl = winrt::get_self<BreadcrumbItem>(breadcrumbItem)->get_strong();
    }

    return impl;
}

com_ptr<BreadcrumbDropDownItem> BreadcrumbItemAutomationPeer::GetImpl2()
{
    com_ptr<BreadcrumbDropDownItem> impl = nullptr;

    if (auto breadcrumbItem = Owner().try_as<winrt::BreadcrumbDropDownItem>())
    {
        impl = winrt::get_self<BreadcrumbDropDownItem>(breadcrumbItem)->get_strong();
    }

    return impl;
}

// IInvokeProvider
void BreadcrumbItemAutomationPeer::Invoke()
{
    if (auto breadcrumbItem = GetImpl())
    {
        breadcrumbItem->OnClickEvent(nullptr, nullptr);
    }
    else if(auto breadcrumbItem = GetImpl2())
    {
        breadcrumbItem->OnClickEvent(nullptr, nullptr);
    }
}
