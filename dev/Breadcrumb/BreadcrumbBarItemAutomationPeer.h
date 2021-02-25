// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "BreadcrumbBarItem.h"
#include "BreadcrumbBarItemAutomationPeer.g.h"

class BreadcrumbBarItemAutomationPeer :
    public ReferenceTracker<BreadcrumbBarItemAutomationPeer, winrt::implementation::BreadcrumbBarItemAutomationPeerT>
{

public:
    BreadcrumbBarItemAutomationPeer(winrt::BreadcrumbBarItem const& owner);

    // IAutomationPeerOverrides
    winrt::IInspectable GetPatternCore(winrt::PatternInterface const& patternInterface);
    hstring GetClassNameCore();
    winrt::AutomationControlType GetAutomationControlTypeCore();

    // IInvokeProvider
    void Invoke();

private:
    com_ptr<BreadcrumbBarItem> GetImpl();
};
