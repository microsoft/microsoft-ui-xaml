// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "BreadcrumbItem.h"
#include "BreadcrumbItemAutomationPeer.g.h"

class BreadcrumbItemAutomationPeer :
    public ReferenceTracker<BreadcrumbItemAutomationPeer, winrt::implementation::BreadcrumbItemAutomationPeerT>
{

public:
    BreadcrumbItemAutomationPeer(winrt::BreadcrumbItem const& owner);

    // IAutomationPeerOverrides
    winrt::IInspectable GetPatternCore(winrt::PatternInterface const& patternInterface);
    hstring GetClassNameCore();
    winrt::AutomationControlType GetAutomationControlTypeCore();

    // IInvokeProvider
    void Invoke();

private:
    com_ptr<BreadcrumbItem> GetImpl();
};
