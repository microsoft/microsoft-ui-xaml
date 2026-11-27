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

        bool IsTraceObserved() noexcept
        {
            return IsNavigationViewTracingEnabled() || NavigationViewTrace::s_IsDebugOutputEnabled ||
                NavigationViewTrace::s_IsVerboseDebugOutputEnabled;
        }
    }

    SelectionRun::SelectionRun(const winrt::IInspectable& owner, winrt::NavigationViewPaneDisplayMode paneDisplayMode,
        bool isTopNavigation, bool isSettingsItem) noexcept :
        m_uncaughtExceptions(std::uncaught_exceptions())
    {
        {
            auto data = m_test.data();
            data->paneDisplayMode = static_cast<uint32_t>(paneDisplayMode);
            data->isTopNavigation = isTopNavigation;
            data->isSettingsItem = isSettingsItem;
        }
        m_tipStarted = m_test.start() != GUID{};
        m_active = m_tipStarted || IsTraceObserved();
        if (m_active)
        {
            m_owner = owner;
        }
    }

    SelectionRun::~SelectionRun()
    {
        if (m_active)
        {
            Ignore(std::uncaught_exceptions() > m_uncaughtExceptions ?
                SelectionTipReason::unwound_by_exception : SelectionTipReason::superseded);
        }
    }

    void SelectionRun::Complete(SelectionTipReason reason) noexcept
    {
        if (reason >= SelectionTipReason::superseded)
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
        Trace(reason == SelectionTipReason::scenario_completed ? L"Succeeded" : L"Failed", reason);
    }

    void SelectionRun::Ignore(SelectionTipReason reason) noexcept
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

    void SelectionRun::Trace(PCWSTR outcome, SelectionTipReason reason) noexcept
    {
        NAVIGATIONVIEW_TRACE_INFO(m_owner, L"TiP NavigationViewSelectionTest: %s (%hs)\n", outcome, GetReasonName(reason));
        m_owner = nullptr;
    }
}
