// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cstdint>
#include <tip/tip.h>
#include "WindowsAppSdk-ProductInfo.h"

namespace NavigationViewTip
{
    enum class SelectionTipReason : unsigned short
    {
        scenario_completed = 1,
        scenario_not_completed = 2,             // The run completed without the scenario_completed flag (fail closed).

        // Failures: the selection NavigationView just applied is not what the app or user sees.
        selected_item_mismatch = 10,            // SelectedItem is not the item that was selected.
        selected_container_not_selected = 11,   // The realized container of the selected item has IsSelected == false.
        previous_container_still_selected = 12, // The realized container of the previous item still has IsSelected == true.

        // Ignored: the run cannot tell whether NavigationView did its job.
        superseded = 100,                       // A reentrant selection (e.g. from ItemInvoked or SelectionChanged) took over.
        not_ready = 101,                        // The template is not applied yet, so there are no containers to check.
        unwound_by_exception = 102,             // An exception (often from an app event handler) left ChangeSelection early.
    };

    struct NavigationViewTipDimensions
    {
        uint32_t componentMajor{ WINUI_RELEASE_MAJOR };
        uint32_t productMajor{ WINDOWSAPPSDK_RELEASE_MAJOR };
        uint32_t productMinor{ WINDOWSAPPSDK_RELEASE_MINOR };
        uint32_t componentBuild{ WINUI_BUILD_VERSION };

        template<class Archive>
        void serialize(Archive& archive)
        {
            archive(TIP_value(componentMajor), TIP_value(productMajor), TIP_value(productMinor),
                TIP_value(componentBuild));
        }
    };

    // One run per selection change (NavigationView::ChangeSelection). Passes when, after NavigationView has
    // applied the selection and raised its events, SelectedItem and the realized containers' IsSelected agree.
    TIP_declare_test(NavigationViewSelectionTest, 65470603)
    {
        TIP_set_data_version(1);
        TIP_set_metrics_enabled();
        using reason = SelectionTipReason;

        NavigationViewTipDimensions dimensions;
        uint32_t paneDisplayMode{};
        bool isTopNavigation{};
        bool isSettingsItem{};

        TIP_require_all_flags_clear(reason::selected_item_mismatch, reason::selected_container_not_selected,
            reason::previous_container_still_selected);

        void evaluate()
        {
            TIP_fail_if_complete_not_called();
            TIP_fail_if(!state().is_flag_set(reason::scenario_completed), reason::scenario_not_completed);
            TIP_succeed(reason::scenario_completed);
        }

        template<class Archive>
        void serialize(Archive& archive)
        {
            archive(TIP_value(dimensions), TIP_value(paneDisplayMode), TIP_value(isTopNavigation), TIP_value(isSettingsItem));
        }

        // Bucket by pane display mode, so a failure rate can be read per mode.
        template<class Archive>
        unsigned char serialize_metrics(Archive& archive)
        {
            archive(TIP_value(dimensions));
            return static_cast<unsigned char>(paneDisplayMode);
        }
    };

    enum class PaneTipReason : unsigned short
    {
        scenario_completed = 1,
        scenario_not_completed = 2,             // The run completed without the scenario_completed flag (fail closed).

        // Failures: the pane state NavigationView reports is not the state of the pane on screen.
        request_not_applied = 10,               // NavigationView asked to open or close the pane, but IsPaneOpen did not change.
        pane_state_mismatch = 11,               // NavigationView.IsPaneOpen != RootSplitView.IsPaneOpen.
        closed_compact_state_stale = 12,        // The ClosedCompact state does not match the SplitView's IsPaneOpen and DisplayMode.

        // Ignored: the run cannot tell whether NavigationView did its job.
        superseded = 100,                       // IsPaneOpen changed again before the check ran (for example from PaneOpening).
        not_ready = 101,                        // The template is not applied yet, so there is no SplitView to compare with.
        unwound_by_exception = 102,             // An exception (often from an app event handler) left the open or close early.
        cancelled = 103,                        // The app cancelled PaneClosing for a light-dismiss close.
        pane_not_shown = 104,                   // Top mode or IsPaneVisible == false: there is no left pane on screen.
        custom_template = 105,                  // The template's SplitView.IsPaneOpen is not bound to NavigationView.IsPaneOpen.
        not_checked = 106,                      // The NavigationView was released before the check ran.
    };

    // What asked for the pane to open or close.
    enum class PaneTipTrigger : uint8_t
    {
        app = 0,                                // The app set IsPaneOpen.
        toggle_button = 1,                      // The pane toggle button.
        split_view = 2,                         // The SplitView closed its pane, for example on a tap outside an overlay pane.
        light_dismiss = 3,                      // NavigationView closed an overlay pane for the back key or Alt+Left.
        item_selected = 4,                      // NavigationView closed the pane after an item was invoked or selected.
        search = 5,                             // The search button, or a chosen AutoSuggestBox suggestion.
        gamepad = 6,                            // The GamepadView key.
        layout = 7,                             // NavigationView opened or closed the pane for a display mode, width or IsPaneVisible change.
    };

    // One run per IsPaneOpen change that NavigationView handles. Passes when, after NavigationView has applied the change
    // and raised its events, NavigationView.IsPaneOpen, the template's SplitView.IsPaneOpen and the ClosedCompact state agree.
    // TODO(TiP): replace 0 with the TiP measure id before shipping.
    TIP_declare_test(NavigationViewPaneTest, 0)
    {
        TIP_set_data_version(1);
        TIP_set_metrics_enabled();
        using reason = PaneTipReason;

        NavigationViewTipDimensions dimensions;
        uint32_t paneDisplayMode{};
        uint32_t displayMode{};
        bool isOpening{};
        uint32_t trigger{};

        TIP_require_all_flags_clear(reason::request_not_applied, reason::pane_state_mismatch,
            reason::closed_compact_state_stale);

        void evaluate()
        {
            TIP_fail_if_complete_not_called();
            TIP_fail_if(!state().is_flag_set(reason::scenario_completed), reason::scenario_not_completed);
            TIP_succeed(reason::scenario_completed);
        }

        template<class Archive>
        void serialize(Archive& archive)
        {
            archive(TIP_value(dimensions), TIP_value(paneDisplayMode), TIP_value(displayMode), TIP_value(isOpening),
                TIP_value(trigger));
        }

        // Bucket by pane display mode (tens digit) and display mode (units digit), e.g. 31 = LeftCompact in Compact.
        template<class Archive>
        unsigned char serialize_metrics(Archive& archive)
        {
            archive(TIP_value(dimensions));
            return static_cast<unsigned char>(paneDisplayMode * 10 + displayMode);
        }
    };
}
