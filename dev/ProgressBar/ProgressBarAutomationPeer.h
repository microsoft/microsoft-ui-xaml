// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ProgressBar.h"
#include "ProgressBarAutomationPeer.g.h"

class ProgressBarAutomationPeer :
    public ReferenceTracker<ProgressBarAutomationPeer, winrt::implementation::ProgressBarAutomationPeerT>
{

public:
    ProgressBarAutomationPeer(winrt::ProgressBar const& owner);

    // IAutomationPeerOverrides
    winrt::IInspectable GetPatternCore(winrt::PatternInterface const& patternInterface);
    winrt::hstring GetClassNameCore();
    winrt::hstring GetNameCore();
    winrt::AutomationControlType GetAutomationControlTypeCore();

    bool IsReadOnly() { return true; }

private:
};

