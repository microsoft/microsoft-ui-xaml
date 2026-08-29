// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ResourceAccessor.h"
#include "BreadcrumbBarItem.h"
#include "BreadcrumbBarItemAutomationPeer.h"
#include "BreadcrumbBarItemAutomationPeer.properties.cpp"

BreadcrumbBarItemAutomationPeer::BreadcrumbBarItemAutomationPeer(winrt::BreadcrumbBarItem const& owner)
    : ReferenceTracker(owner)
{
}

// IAutomationPeerOverrides
hstring BreadcrumbBarItemAutomationPeer::GetLocalizedControlTypeCore()
{
    return ResourceAccessor::GetLocalizedStringResource(SR_BreadcrumbBarItemLocalizedControlType);
}

winrt::IInspectable BreadcrumbBarItemAutomationPeer::GetPatternCore(winrt::PatternInterface const& patternInterface)
{
    if (patternInterface == winrt::PatternInterface::Invoke)
    {
        return *this;
    }

    return __super::GetPatternCore(patternInterface);
}

hstring BreadcrumbBarItemAutomationPeer::GetClassNameCore()
{
    return winrt::hstring_name_of<winrt::BreadcrumbBarItem>();
}

winrt::AutomationControlType BreadcrumbBarItemAutomationPeer::GetAutomationControlTypeCore()
{
    return winrt::AutomationControlType::Button;
}

com_ptr<BreadcrumbBarItem> BreadcrumbBarItemAutomationPeer::GetImpl()
{
    com_ptr<BreadcrumbBarItem> impl = nullptr;
    
    if (auto breadcrumbItem = Owner().try_as<winrt::BreadcrumbBarItem>())
    {
        impl = winrt::get_self<BreadcrumbBarItem>(breadcrumbItem)->get_strong();
    }

    return impl;
}

// IInvokeProvider
void BreadcrumbBarItemAutomationPeer::Invoke()
{
    if (auto breadcrumbItem = GetImpl())
    {
        breadcrumbItem->OnClickEvent(nullptr, nullptr);
    }
}
