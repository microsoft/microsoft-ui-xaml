// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "NavigationViewTip.h"
#include "NavigationViewTrace.h"

#include <exception>

namespace NavigationViewTip
{
    namespace
    {
        constexpr unsigned short c_firstIgnoreReason = 100;

        const char* GetReasonName(SelectionTipReason reason) noexcept
        {
            switch (reason)
            {
            case SelectionTipReason::scenario_completed: return "scenario_completed";
            case SelectionTipReason::scenario_not_completed: return "scenario_not_completed";
            case SelectionTipReason::selected_item_mismatch: return "selected_item_mismatch";
            case SelectionTipReason::selected_container_not_selected: return "selected_container_not_selected";
            case SelectionTipReason::previous_container_still_selected: return "previous_container_still_selected";
            case SelectionTipReason::superseded: return "superseded";
            case SelectionTipReason::not_ready: return "not_ready";
            case SelectionTipReason::unwound_by_exception: return "unwound_by_exception";
            default: return "unknown";
            }
        }

        const char* GetReasonName(PaneTipReason reason) noexcept
        {
            switch (reason)
            {
            case PaneTipReason::scenario_completed: return "scenario_completed";
            case PaneTipReason::scenario_not_completed: return "scenario_not_completed";
            case PaneTipReason::request_not_applied: return "request_not_applied";
            case PaneTipReason::pane_state_mismatch: return "pane_state_mismatch";
            case PaneTipReason::closed_compact_state_stale: return "closed_compact_state_stale";
            case PaneTipReason::superseded: return "superseded";
            case PaneTipReason::not_ready: return "not_ready";
            case PaneTipReason::unwound_by_exception: return "unwound_by_exception";
            case PaneTipReason::cancelled: return "cancelled";
            case PaneTipReason::pane_not_shown: return "pane_not_shown";
            case PaneTipReason::custom_template: return "custom_template";
            case PaneTipReason::not_checked: return "not_checked";
            default: return "unknown";
            }
        }

        const char* GetTriggerName(PaneTipTrigger trigger) noexcept
        {
            switch (trigger)
            {
            case PaneTipTrigger::app: return "app";
            case PaneTipTrigger::toggle_button: return "toggle_button";
            case PaneTipTrigger::split_view: return "split_view";
            case PaneTipTrigger::light_dismiss: return "light_dismiss";
            case PaneTipTrigger::item_selected: return "item_selected";
            case PaneTipTrigger::search: return "search";
            case PaneTipTrigger::gamepad: return "gamepad";
            case PaneTipTrigger::layout: return "layout";
            default: return "unknown";
            }
        }

        bool IsTraceObserved() noexcept
        {
            return IsNavigationViewTracingEnabled() || NavigationViewTrace::s_IsDebugOutputEnabled ||
                NavigationViewTrace::s_IsVerboseDebugOutputEnabled;
        }
    }

    template <class TTest, class TReason>
    void TipRun<TTest, TReason>::Start(const winrt::IInspectable& owner, const char* testName) noexcept
    {
        m_tipStarted = m_test.start() != GUID{};
        m_active = m_tipStarted || IsTraceObserved();
        if (m_active)
        {
            m_owner = winrt::make_weak(owner);
            m_testName = testName;
        }
    }

    template <class TTest, class TReason>
    void TipRun<TTest, TReason>::Complete(TReason reason) noexcept
    {
        if (static_cast<unsigned short>(reason) >= c_firstIgnoreReason)
        {
            Ignore(reason);
            return;
        }
        if (!m_active)
        {
            return;
        }
        m_active = false;
        if (m_tipStarted)
        {
            m_test.set_flag(static_cast<unsigned short>(reason), GetReasonName(reason));
            m_test.complete();
            m_test.reset();
        }
        Trace(reason == TReason::scenario_completed ? L"Succeeded" : L"Failed", reason);
    }

    template <class TTest, class TReason>
    void TipRun<TTest, TReason>::Ignore(TReason reason) noexcept
    {
        if (!m_active)
        {
            return;
        }
        m_active = false;
        if (m_tipStarted)
        {
            m_test.complete_and_ignore(static_cast<unsigned short>(reason), GetReasonName(reason));
            m_test.reset();
        }
        Trace(L"Ignored", reason);
    }

    template <class TTest, class TReason>
    void TipRun<TTest, TReason>::Trace(PCWSTR outcome, TReason reason) noexcept
    {
        // The owner can be gone, e.g. when a pending pane run is released with its NavigationView.
        const auto owner = m_owner.get();
        NAVIGATIONVIEW_TRACE_INFO(owner, L"TiP %hs: %s (%hs)%hs\n", m_testName, outcome, GetReasonName(reason), m_detail);
        m_owner = nullptr;
    }

    template class TipRun<NavigationViewSelectionTest, SelectionTipReason>;
    template class TipRun<NavigationViewPaneTest, PaneTipReason>;

    SelectionRun::SelectionRun(const winrt::IInspectable& owner, winrt::NavigationViewPaneDisplayMode paneDisplayMode,
        bool isTopNavigation, bool isSettingsItem) noexcept :
        m_uncaughtExceptions(std::uncaught_exceptions())
    {
        {
            auto data = Test().data();
            data->paneDisplayMode = static_cast<uint32_t>(paneDisplayMode);
            data->isTopNavigation = isTopNavigation;
            data->isSettingsItem = isSettingsItem;
        }
        Start(owner, "NavigationViewSelectionTest");
    }

    SelectionRun::~SelectionRun()
    {
        if (IsActive())
        {
            Ignore(std::uncaught_exceptions() > m_uncaughtExceptions ?
                SelectionTipReason::unwound_by_exception : SelectionTipReason::superseded);
        }
    }

    PaneRun::PaneRun(const winrt::IInspectable& owner, winrt::NavigationViewPaneDisplayMode paneDisplayMode,
        winrt::NavigationViewDisplayMode displayMode, bool isOpening, PaneTipTrigger trigger) noexcept
    {
        {
            auto data = Test().data();
            data->paneDisplayMode = static_cast<uint32_t>(paneDisplayMode);
            data->displayMode = static_cast<uint32_t>(displayMode);
            data->isOpening = isOpening;
            data->trigger = static_cast<uint32_t>(trigger);
        }
        Start(owner, "NavigationViewPaneTest");
        if (IsActive() && FAILED(StringCchPrintfA(m_detail, ARRAYSIZE(m_detail), "; %s, %s", isOpening ? "open" : "close", GetTriggerName(trigger))))
        {
            m_detail[0] = '\0';
        }
    }

    PaneRun::~PaneRun()
    {
        Ignore(PaneTipReason::not_checked);
    }
}
