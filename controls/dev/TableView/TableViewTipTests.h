// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cstdint>
#include <tip/tip.h>
#include "WindowsAppSdk-ProductInfo.h"
#include "TableViewTipTypes.h"

namespace TableViewTip
{
    enum class TableViewTipReason : unsigned short
    {
        scenario_completed = 1, scenario_not_completed = 2, internal_failure = 17
    };

    struct TableViewTipDimensions
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

    TIP_declare_test(TableViewInitializationTest, 64477764)
    {
        TIP_set_data_version(6);
        TIP_set_metrics_enabled();
        using reason = TableViewTipReason;

        TableViewTipDimensions dimensions;
        HRESULT failureHResult{};
        bool failureHResultAvailable{};
        HRESULT diagnosticHResult{};
        bool diagnosticHResultAvailable{};
        AppFaultKind appFaultKind{};

        TIP_require_all_flags_clear(reason::internal_failure);

        void evaluate()
        {
            TIP_fail_if_complete_not_called();
            TIP_fail_if(!state().is_flag_set(reason::scenario_completed), reason::scenario_not_completed);
            TIP_succeed(reason::scenario_completed);
        }

        template<class Archive>
        void serialize(Archive& archive)
        {
            archive(TIP_value(dimensions), TIP_value(failureHResult), TIP_value(failureHResultAvailable),
                TIP_value(diagnosticHResult), TIP_value(diagnosticHResultAvailable), TIP_value(appFaultKind));
        }

        template<class Archive>
        unsigned char serialize_metrics(Archive& archive)
        {
            archive(TIP_value(dimensions));
            return 0;
        }
    };

    TIP_declare_test(TableViewOperationTest, 64477774)
    {
        TIP_set_data_version(5);
        TIP_set_metrics_enabled();
        using reason = TableViewTipReason;

        TableViewTipDimensions dimensions;
        Operation operation{};
        HRESULT failureHResult{};
        bool failureHResultAvailable{};
        HRESULT diagnosticHResult{};
        bool diagnosticHResultAvailable{};
        AppFaultKind appFaultKind{};

        TIP_require_all_flags_clear(reason::internal_failure);

        void evaluate()
        {
            TIP_fail_if_complete_not_called();
            TIP_fail_if(!state().is_flag_set(reason::scenario_completed), reason::scenario_not_completed);
            TIP_succeed(reason::scenario_completed);
        }

        template<class Archive>
        void serialize(Archive& archive)
        {
            archive(TIP_value(dimensions), TIP_value(operation), TIP_value(failureHResult), TIP_value(failureHResultAvailable),
                TIP_value(diagnosticHResult), TIP_value(diagnosticHResultAvailable), TIP_value(appFaultKind));
        }

        template<class Archive>
        unsigned char serialize_metrics(Archive& archive)
        {
            archive(TIP_value(dimensions));
            return static_cast<unsigned char>(operation);
        }
    };
}
