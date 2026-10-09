// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cstdint>
#include <tip/tip.h>
#include "WindowsAppSdk-ProductInfo.h"

namespace TableViewTelemetry
{
    enum class Operation : uint32_t
    {
        InitialLayout = 0, ReplaceSource = 1, Sort = 2, HeaderRefresh = 3, Layout = 4,
        Filter = 5, GroupConfiguration = 6, GroupExpansion = 7, Selection = 8,
        BeginEdit = 9, CommitEdit = 10, CancelEdit = 11, Scroll = 12
    };
    enum class Origin : uint32_t { Template = 0, Loaded = 1 };
    enum class Content : uint32_t { Empty = 0, Rows = 1, GroupHeaders = 2 };
    enum class IgnoreReason : unsigned short
    {
        Cancelled = 100, Vetoed = 101, ValidationRejected = 102, Superseded = 103,
        Unloaded = 104, Retemplated = 105, Stale = 106
    };

    constexpr bool IsMeasuredOperation(Operation operation) noexcept
    {
        switch (operation)
        {
        case Operation::ReplaceSource:
        case Operation::Sort:
        case Operation::Filter:
        case Operation::GroupConfiguration:
        case Operation::GroupExpansion:
        case Operation::Selection:
        case Operation::BeginEdit:
        case Operation::CommitEdit:
        case Operation::CancelEdit:
        case Operation::Scroll:
            return true;
        default:
            return false;
        }
    }

    enum class TableViewTipReason : unsigned short
    {
        scenario_completed = 1, scenario_not_completed = 2,
        template_failure = 10, source_failure = 11, shaping_failure = 12, realization_failure = 13,
        selection_failure = 14, editing_failure = 15, scroll_failure = 16, internal_failure = 17,
        cancelled = 100, vetoed = 101, validation_rejected = 102, superseded = 103,
        unloaded = 104, retemplated = 105, stale = 106
    };

    struct TableViewTipDimensions
    {
        uint32_t componentMajor{ WINUI_RELEASE_MAJOR };
        uint32_t productMajor{ WINDOWSAPPSDK_RELEASE_MAJOR };
        uint32_t productMinor{ WINDOWSAPPSDK_RELEASE_MINOR };
        uint32_t componentBuild{ WINUI_BUILD_VERSION };
        Origin origin{};
        Content content{};
        uint32_t rowCountBucket{};
        uint32_t columnCountBucket{};
        bool configurationAvailable{};
        bool rowCountAvailable{};

        template<class Archive>
        void serialize(Archive& archive)
        {
            archive(TIP_value(componentMajor), TIP_value(productMajor), TIP_value(productMinor),
                TIP_value(componentBuild), TIP_value(origin), TIP_value(content), TIP_value(rowCountBucket),
                TIP_value(columnCountBucket), TIP_value(configurationAvailable), TIP_value(rowCountAvailable));
        }
    };

    TIP_declare_test(TableViewInitializationTest, 64477764)
    {
        TIP_set_data_version(1);
        TIP_set_metrics_enabled();
        using reason = TableViewTipReason;

        TableViewTipDimensions dimensions;
        HRESULT failureHResult{};
        bool failureHResultAvailable{};

        TIP_require_all_flags_clear(reason::template_failure, reason::source_failure, reason::shaping_failure,
            reason::realization_failure, reason::selection_failure, reason::editing_failure,
            reason::scroll_failure, reason::internal_failure);

        void evaluate()
        {
            TIP_fail_if_complete_not_called();
            TIP_fail_if(!state().is_flag_set(reason::scenario_completed), reason::scenario_not_completed);
            TIP_succeed(reason::scenario_completed);
        }

        template<class Archive>
        void serialize(Archive& archive)
        {
            archive(TIP_value(dimensions), TIP_value(failureHResult), TIP_value(failureHResultAvailable));
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
        TIP_set_data_version(1);
        TIP_set_metrics_enabled();
        using reason = TableViewTipReason;

        TableViewTipDimensions dimensions;
        Operation operation{};
        HRESULT failureHResult{};
        bool failureHResultAvailable{};

        TIP_require_all_flags_clear(reason::template_failure, reason::source_failure, reason::shaping_failure,
            reason::realization_failure, reason::selection_failure, reason::editing_failure,
            reason::scroll_failure, reason::internal_failure);

        void evaluate()
        {
            TIP_fail_if_complete_not_called();
            TIP_fail_if(!state().is_flag_set(reason::scenario_completed), reason::scenario_not_completed);
            TIP_succeed(reason::scenario_completed);
        }

        template<class Archive>
        void serialize(Archive& archive)
        {
            archive(TIP_value(dimensions), TIP_value(operation), TIP_value(failureHResult), TIP_value(failureHResultAvailable));
        }

        template<class Archive>
        unsigned char serialize_metrics(Archive& archive)
        {
            archive(TIP_value(dimensions));
            return static_cast<unsigned char>(operation);
        }
    };
}
