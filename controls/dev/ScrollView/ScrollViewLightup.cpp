// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"    // This is a direct (non-PCH) compile
#include "common.h"
#include "TypeLogging.h"
#include "ScrollPresenterTypeLogging.h"
#include "ScrollPresenter.h"
#include "ScrollView.h"
#include "RuntimeProfiler.h"
#include "FocusHelper.h"
#include "RegUtil.h"
#include "ScrollViewTestHooks.h"


// explicitly instantiate deletion of ScrollView::AutoHideScrollBarsState in this translation unit
template void std::default_delete<ScrollView::AutoHideScrollBarsState>::operator()(ScrollView::AutoHideScrollBarsState*) const noexcept;
template void std::default_delete<const ScrollView::AutoHideScrollBarsState>::operator()(const ScrollView::AutoHideScrollBarsState*) const noexcept;

struct ScrollView::AutoHideScrollBarsState
{
    winrt::IUISettings5 m_uiSettings5{ nullptr };
    winrt::IUISettings5::AutoHideScrollBarsChanged_revoker m_autoHideScrollBarsChangedRevoker{};

    void HookUISettingsEvent(ScrollView* scrollView)
    {
        // Introduced in 19H1, IUISettings5 exposes the AutoHideScrollBars property and AutoHideScrollBarsChanged event.
        if (!m_uiSettings5)
        {
            winrt::UISettings uiSettings;

            m_uiSettings5 = uiSettings.try_as<winrt::IUISettings5>();
            if (m_uiSettings5)
            {
                m_autoHideScrollBarsChangedRevoker = m_uiSettings5.AutoHideScrollBarsChanged(
                    winrt::auto_revoke,
                    { scrollView, &ScrollView::OnAutoHideScrollBarsChanged });
            }
        }
    }
};
    
std::unique_ptr<ScrollView::AutoHideScrollBarsState> ScrollView::MakeAutoHideScrollBarsState()
{
    return std::make_unique<AutoHideScrollBarsState>();
}

void ScrollView::OnAutoHideScrollBarsChanged(
    winrt::UISettings const& uiSettings,
    winrt::UISettingsAutoHideScrollBarsChangedEventArgs const& args)
{
    // OnAutoHideScrollBarsChanged is called on a non-UI thread, process notification on the UI thread using a dispatcher.
    m_dispatcherQueue.TryEnqueue(winrt::DispatcherQueueHandler(
        [strongThis = get_strong()]()
    {
        strongThis->m_autoHideScrollControllersValid = false;
        strongThis->UpdateVisualStates(
            true  /*useTransitions*/,
            false /*showIndicators*/,
            false /*hideIndicators*/,
            true  /*scrollControllersAutoHidingChanged*/);
    }));
}

void ScrollView::HookUISettingsEvent()
{
    m_autoHideScrollBarsState->HookUISettingsEvent(this);
}

bool ScrollView::AreScrollControllersAutoHiding()
{
    // Use the cached value unless it was invalidated.
    if (m_autoHideScrollControllersValid)
    {
        return m_autoHideScrollControllers;
    }

    m_autoHideScrollControllersValid = true;

    if (auto globalTestHooks = ScrollViewTestHooks::GetGlobalTestHooks())
    {
        winrt::IReference<bool> autoHideScrollControllers = globalTestHooks->GetAutoHideScrollControllers(*this);

        if (autoHideScrollControllers)
        {
            // Test hook takes precedence over UISettings and registry key settings.
            m_autoHideScrollControllers = autoHideScrollControllers.Value();
            return m_autoHideScrollControllers;
        }
    }

    if (m_autoHideScrollBarsState->m_uiSettings5)
    {
        m_autoHideScrollControllers = m_autoHideScrollBarsState->m_uiSettings5.AutoHideScrollBars();
    }
    else
    {
        m_autoHideScrollControllers = RegUtil::UseDynamicScrollbars();
    }

    return m_autoHideScrollControllers;
}

// On RS4 and RS5, update m_autoHideScrollControllers based on the DynamicScrollbars registry key value
// and update the visual states if the value changed.
void ScrollView::UpdateScrollControllersAutoHiding(
    bool forceUpdate)
{
    if ((forceUpdate || !m_autoHideScrollBarsState->m_uiSettings5) && m_autoHideScrollControllersValid)
    {
        m_autoHideScrollControllersValid = false;

        const bool oldAutoHideScrollControllers = m_autoHideScrollControllers;
        const bool newAutoHideScrollControllers = AreScrollControllersAutoHiding();

        if (oldAutoHideScrollControllers != newAutoHideScrollControllers)
        {
            UpdateVisualStates(
                true  /*useTransitions*/,
                false /*showIndicators*/,
                false /*hideIndicators*/,
                true  /*scrollControllersAutoHidingChanged*/);
        }
    }
}

