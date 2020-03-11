// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ProgressRing.h"
#include "ProgressRingAutomationPeer.g.h"

class ProgressRingAutomationPeer :
    public ReferenceTracker<ProgressRingAutomationPeer, winrt::implementation::ProgressRingAutomationPeerT, winrt::IRangeValueProvider>
{

public:
    ProgressRingAutomationPeer(winrt::ProgressRing const& owner);

    // IAutomationPeerOverrides
    winrt::IInspectable GetPatternCore(winrt::PatternInterface const& patternInterface);
    hstring GetClassNameCore();
    winrt::hstring GetNameCore();
    winrt::AutomationControlType GetAutomationControlTypeCore();

    // IRangeValueProvider is necessary here to override IsReadOnly() to true.
    bool IsReadOnly() { return true; }
    double Value();
    double SmallChange();
    double LargeChange();
    double Minimum();
    double Maximum();
    void SetValue(double value);

private:
    com_ptr<ProgressRing> GetImpl();
};

