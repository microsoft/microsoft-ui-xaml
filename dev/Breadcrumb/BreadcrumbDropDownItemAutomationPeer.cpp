// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "BreadcrumbDropDownItem.h"
#include "BreadcrumbDropDownItemAutomationPeer.h"
#include "BreadcrumbDropDownItemAutomationPeer.properties.cpp"

BreadcrumbDropDownItemAutomationPeer::BreadcrumbDropDownItemAutomationPeer(winrt::BreadcrumbDropDownItem const& owner)
    : ReferenceTracker(owner)
{
}

// IAutomationPeerOverrides
winrt::IInspectable BreadcrumbDropDownItemAutomationPeer::GetPatternCore(winrt::PatternInterface const& patternInterface)
{
    if (patternInterface == winrt::PatternInterface::Invoke)
    {
        return *this;
    }

    return __super::GetPatternCore(patternInterface);
}

hstring BreadcrumbDropDownItemAutomationPeer::GetClassNameCore()
{
    return winrt::hstring_name_of<winrt::BreadcrumbDropDownItem>();
}

winrt::AutomationControlType BreadcrumbDropDownItemAutomationPeer::GetAutomationControlTypeCore()
{
    return winrt::AutomationControlType::Button;
}

com_ptr<BreadcrumbDropDownItem> BreadcrumbDropDownItemAutomationPeer::GetImpl()
{
    com_ptr<BreadcrumbDropDownItem> impl = nullptr;

    if (auto dropDownItem = Owner().try_as<winrt::BreadcrumbDropDownItem>())
    {
        impl = winrt::get_self<BreadcrumbDropDownItem>(dropDownItem)->get_strong();
    }

    return impl;
}

// IInvokeProvider
void BreadcrumbDropDownItemAutomationPeer::Invoke()
{
    if (auto dropDownItem = GetImpl())
    {
        dropDownItem->OnClickEvent(nullptr, nullptr);
    }
}
