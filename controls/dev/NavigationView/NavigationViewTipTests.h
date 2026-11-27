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
}
