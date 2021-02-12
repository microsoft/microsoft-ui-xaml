// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "BreadcrumbDropDownItem.h"
#include "BreadcrumbDropDownItemAutomationPeer.g.h"

class BreadcrumbDropDownItemAutomationPeer :
    public ReferenceTracker<BreadcrumbDropDownItemAutomationPeer, winrt::implementation::BreadcrumbDropDownItemAutomationPeerT>
{

public:
    BreadcrumbDropDownItemAutomationPeer(winrt::BreadcrumbDropDownItem const& owner);

    // IAutomationPeerOverrides
    winrt::IInspectable GetPatternCore(winrt::PatternInterface const& patternInterface);
    hstring GetClassNameCore();
    winrt::AutomationControlType GetAutomationControlTypeCore();

    // IInvokeProvider
    void Invoke();

private:
    com_ptr<BreadcrumbDropDownItem> GetImpl();
};

