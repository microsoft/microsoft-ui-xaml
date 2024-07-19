﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ProgressRing.h"
#include "ProgressRingAutomationPeer.g.h"

class ProgressRingAutomationPeer :
    public ReferenceTracker<ProgressRingAutomationPeer, winrt::implementation::ProgressRingAutomationPeerT>
{

public:
    ProgressRingAutomationPeer(winrt::ProgressRing const& owner);

    winrt::IInspectable GetPatternCore(winrt::PatternInterface const& patternInterface);
    winrt::hstring GetClassNameCore();
    winrt::hstring GetNameCore();
    winrt::AutomationControlType GetAutomationControlTypeCore();
    winrt::hstring GetLocalizedControlTypeCore();

    // IRangeValueProvider
    bool IsReadOnly() { return true; }
    double Minimum();
    double Maximum();
    double Value();
    double SmallChange();
    double LargeChange();
    void SetValue(double value);

private:
    com_ptr<ProgressRing> GetImpl();
};

