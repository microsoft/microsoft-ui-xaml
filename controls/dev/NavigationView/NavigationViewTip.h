// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "NavigationViewTipTests.h"

namespace NavigationViewTip
{
    // Scopes one NavigationViewSelectionTest run to one ChangeSelection call. The run is active only when TiP
    // sampled it or NavigationView tracing is on, so callers can skip the checks otherwise. Every outcome is
    // also traced as "TiP NavigationViewSelectionTest: <outcome> (<reason>)" for diagnostics and tests.
    // A run left open (an early return or an exception) is ignored as superseded or unwound_by_exception.
    class SelectionRun
    {
    public:
        SelectionRun(const winrt::IInspectable& owner, winrt::NavigationViewPaneDisplayMode paneDisplayMode,
            bool isTopNavigation, bool isSettingsItem) noexcept;
        ~SelectionRun();
        SelectionRun(const SelectionRun&) = delete;
        SelectionRun& operator=(const SelectionRun&) = delete;

        bool IsActive() const noexcept { return m_active; }
        // reason is scenario_completed, or the failure that was found. An ignore reason ignores the run.
        void Complete(SelectionTipReason reason) noexcept;
        void Ignore(SelectionTipReason reason) noexcept;

    private:
        void Trace(PCWSTR outcome, SelectionTipReason reason) noexcept;

        NavigationViewSelectionTest m_test;
        winrt::IInspectable m_owner{ nullptr };
        int m_uncaughtExceptions{};
        bool m_tipStarted{};
        bool m_active{};
    };
}
