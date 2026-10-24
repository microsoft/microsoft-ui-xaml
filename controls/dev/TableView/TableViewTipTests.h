// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cstdint>
#include <optional>
#include <tip/tip.h>
#include "WindowsAppSdk-ProductInfo.h"
#include "TableViewTipTypes.h"

namespace TableViewTip
{
    // 17 was internal_failure under 64477764 and 64477774; do not reuse it.
    enum class TableViewTipReason : unsigned short
    {
        scenario_completed = 1, scenario_not_completed = 2,
        template_setup_failed = 18, source_attach_failed = 19,
        sort_failed = 20, edit_begin_failed = 21, edit_commit_failed = 22,
        filter_failed = 23, group_expansion_failed = 24, hierarchy_expansion_failed = 25
    };

    // The operation tests' measure IDs.
    namespace OperationMeasure
    {
        constexpr unsigned int Sort = 64745033;
        constexpr unsigned int EditBegin = 64745034;
        constexpr unsigned int EditCommit = 64745035;
        constexpr unsigned int Filter = 64745036;
        constexpr unsigned int GroupExpansion = 64745037;
        constexpr unsigned int HierarchyExpansion = 64745038;
    }

    // What an operation's TiP test reports under, and the flag its failure sets (TIP_reason: TiP names a flag after its
    // spelling). TableViewOperationTest requires every failure reason clear.
    struct TestIdentity
    {
        unsigned int measure;
        const char* name;
        unsigned short failReason;
        const char* failReasonName;
    };

    // An operation without an identity runs no TiP test.
    constexpr std::optional<TestIdentity> OperationTestIdentity(Operation operation) noexcept
    {
        switch (operation)
        {
        case Operation::Sort:
            return TestIdentity{ OperationMeasure::Sort, "TableViewSortTest",
                TIP_reason(TableViewTipReason::sort_failed) };
        case Operation::BeginEdit:
            return TestIdentity{ OperationMeasure::EditBegin, "TableViewEditBeginTest",
                TIP_reason(TableViewTipReason::edit_begin_failed) };
        case Operation::CommitEdit:
            return TestIdentity{ OperationMeasure::EditCommit, "TableViewEditCommitTest",
                TIP_reason(TableViewTipReason::edit_commit_failed) };
        case Operation::Filter:
            return TestIdentity{ OperationMeasure::Filter, "TableViewFilterTest",
                TIP_reason(TableViewTipReason::filter_failed) };
        case Operation::GroupExpansion:
            return TestIdentity{ OperationMeasure::GroupExpansion, "TableViewGroupExpansionTest",
                TIP_reason(TableViewTipReason::group_expansion_failed) };
        case Operation::HierarchyExpansion:
            return TestIdentity{ OperationMeasure::HierarchyExpansion, "TableViewHierarchyExpansionTest",
                TIP_reason(TableViewTipReason::hierarchy_expansion_failed) };
        default: return std::nullopt;
        }
    }

    // config_set_identity fail-fasts on a zero measure or a null name, and only a tracked operation starts a test.
    constexpr bool OperationTestIdentitiesValid() noexcept
    {
        for (uint32_t value = 0; value < static_cast<uint32_t>(Operation::Max); ++value)
        {
            auto const operation = static_cast<Operation>(value);
            auto const identity = OperationTestIdentity(operation);
            if (identity && (!IsTrackedOperation(operation) || !identity->measure || !identity->name)) { return false; }
        }
        return true;
    }
    static_assert(OperationTestIdentitiesValid(), "Every operation test needs a measure, a name and a tracked operation.");

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

    // From inputs ready (template applied, source attached) to the layout pass of the first row or group header,
    // or at once for an empty view.
    TIP_declare_test(TableViewInitializationTest, 64477764)
    {
        TIP_set_data_version(1);
        TIP_set_metrics_enabled();
        using reason = TableViewTipReason;

        TableViewTipDimensions dimensions;
        HRESULT failureHResult{};
        bool failureHResultAvailable{};
        HRESULT diagnosticHResult{};
        bool diagnosticHResultAvailable{};
        AppFaultKind appFaultKind{};
        // The metrics bucket's inputs (MetricsBucket), and the rows before and after the filter (Log2Bucket).
        bool rightToLeft{};
        bool largeData{};
        uint8_t sourceRowsBucket{};
        uint8_t dataRowsBucket{};

        TIP_require_all_flags_clear(reason::template_setup_failed, reason::source_attach_failed);

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
            archive(TIP_value(dimensions), TIP_value(rightToLeft), TIP_value(largeData), TIP_value(sourceRowsBucket),
                TIP_value(dataRowsBucket));
            return MetricsBucket(rightToLeft, largeData);
        }
    };

    // Sort, edit begin and commit, filter, and group and tree-node expansion share this test; each reports under its
    // own measure and name (OperationTestIdentity), so the declaration's measure is Sort's. Initialize keeps its own
    // declaration, so a change to these tests' data leaves its data version alone.
    TIP_declare_test(TableViewOperationTest, OperationMeasure::Sort)
    {
        TIP_set_data_version(1);
        TIP_set_metrics_enabled();
        using reason = TableViewTipReason;

        TableViewTipDimensions dimensions;
        HRESULT failureHResult{};
        bool failureHResultAvailable{};
        HRESULT diagnosticHResult{};
        bool diagnosticHResultAvailable{};
        AppFaultKind appFaultKind{};
        // The metrics bucket's inputs (MetricsBucket), and the rows before and after the filter (Log2Bucket).
        bool rightToLeft{};
        bool largeData{};
        uint8_t sourceRowsBucket{};
        uint8_t dataRowsBucket{};

        TIP_require_all_flags_clear(reason::sort_failed, reason::edit_begin_failed, reason::edit_commit_failed,
            reason::filter_failed, reason::group_expansion_failed, reason::hierarchy_expansion_failed);

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
            archive(TIP_value(dimensions), TIP_value(rightToLeft), TIP_value(largeData), TIP_value(sourceRowsBucket),
                TIP_value(dataRowsBucket));
            return MetricsBucket(rightToLeft, largeData);
        }
    };
}
