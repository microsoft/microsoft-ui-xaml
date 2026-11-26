// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "NavigationViewTipTests.h"

namespace NavigationViewTip
{
    // One run of a NavigationView TiP test. The run is active only when TiP sampled it or NavigationView tracing is on,
    // so callers can skip the checks otherwise. Every outcome is also traced as
    // "TiP <test name>: <outcome> (<reason>)<detail>" for diagnostics and tests.
    // Reasons below 100 complete the run (scenario_completed, or the failure that was found); 100 and up ignore it.
    template <class TTest, class TReason>
    class TipRun
    {
    public:
        TipRun(const TipRun&) = delete;
        TipRun& operator=(const TipRun&) = delete;

        bool IsActive() const noexcept { return m_active; }
        void Complete(TReason reason) noexcept;
        void Ignore(TReason reason) noexcept;

    protected:
        TipRun() = default;
        ~TipRun() = default;

        // Call once, after filling Test().data().
        void Start(const winrt::IInspectable& owner, const char* testName) noexcept;
        TTest& Test() noexcept { return m_test; }

        // Appended to the traced outcome line.
        char m_detail[48]{};

    private:
        void Trace(PCWSTR outcome, TReason reason) noexcept;

        TTest m_test;
        winrt::weak_ref<winrt::IInspectable> m_owner{ nullptr };
        const char* m_testName{ "" };
        bool m_tipStarted{};
        bool m_active{};
    };

    // Scopes one NavigationViewSelectionTest run to one ChangeSelection call.
    // A run left open (an early return or an exception) is ignored as superseded or unwound_by_exception.
    class SelectionRun : public TipRun<NavigationViewSelectionTest, SelectionTipReason>
    {
    public:
        SelectionRun(const winrt::IInspectable& owner, winrt::NavigationViewPaneDisplayMode paneDisplayMode,
            bool isTopNavigation, bool isSettingsItem) noexcept;
        ~SelectionRun();

    private:
        int m_uncaughtExceptions{};
    };

    // One NavigationViewPaneTest run for one IsPaneOpen change, or for one open or close request that did not change it.
    // NavigationView keeps the run until its check runs; a run that is never checked is ignored as not_checked.
    class PaneRun : public TipRun<NavigationViewPaneTest, PaneTipReason>
    {
    public:
        PaneRun(const winrt::IInspectable& owner, winrt::NavigationViewPaneDisplayMode paneDisplayMode,
            winrt::NavigationViewDisplayMode displayMode, bool isOpening, PaneTipTrigger trigger) noexcept;
        ~PaneRun();
    };

    // Labels the IsPaneOpen changes made while it is in scope with a trigger. An outer label wins, so a toggle button
    // click that calls OpenPane stays toggle_button.
    class PaneTriggerScope
    {
    public:
        PaneTriggerScope(PaneTipTrigger& current, PaneTipTrigger trigger) noexcept :
            m_current(current), m_previous(current)
        {
            if (m_current == PaneTipTrigger::app)
            {
                m_current = trigger;
            }
        }
        ~PaneTriggerScope() { m_current = m_previous; }
        PaneTriggerScope(const PaneTriggerScope&) = delete;
        PaneTriggerScope& operator=(const PaneTriggerScope&) = delete;

    private:
        PaneTipTrigger& m_current;
        PaneTipTrigger m_previous;
    };
}
