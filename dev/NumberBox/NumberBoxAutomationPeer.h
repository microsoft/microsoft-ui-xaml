// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "NumberBox.h"
#include "NumberBoxAutomationPeer.g.h"

class NumberBoxAutomationPeer :
    public ReferenceTracker<NumberBoxAutomationPeer, winrt::implementation::NumberBoxAutomationPeerT, winrt::IRangeValueProvider>
{

public:
    NumberBoxAutomationPeer(winrt::NumberBox const& owner);

    // IAutomationPeerOverrides
    winrt::IInspectable GetPatternCore(winrt::PatternInterface const& patternInterface);
    hstring GetClassNameCore();
    hstring GetNameCore();
    winrt::AutomationControlType GetAutomationControlTypeCore();

    // IRangeValueProvider
    bool IsReadOnly() { return false; }
    double Minimum();
    double Maximum();
    double Value();
    double SmallChange();
    double LargeChange();
    void SetValue(double value);

    void RaiseValueChangedEvent(double oldValue, double newValue);

private:
    com_ptr<NumberBox> GetImpl();
};

