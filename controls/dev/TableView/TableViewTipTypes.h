// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cstdint>

// TiP and telemetry vocabulary without the TiP library, for headers that only need the names.
namespace TableViewTip
{
    enum class Operation : uint32_t
    {
        Setup = 0, ReplaceSource = 1, Sort = 2, HeaderRefresh = 3,
        Filter = 5, GroupConfiguration = 6, GroupExpansion = 7, Selection = 8,
        BeginEdit = 9, CommitEdit = 10, CancelEdit = 11, HierarchyExpansion = 12,
        // One past the last operation: add operations above it. TableViewTelemetry.cpp checks every value below it.
        Max
    };
    enum class IgnoreReason : unsigned short
    {
        Cancelled = 100, Vetoed = 101, ValidationRejected = 102, Superseded = 103,
        Unloaded = 104, Retemplated = 105, Stale = 106,
        DeferredOrUnproven = 107, ApplicationFault = 108, AppTemplateIncomplete = 109,
        // One past the last reason: add reasons above it. TableViewTip.cpp checks that each one has a name.
        Max
    };
    enum class AppFaultKind : uint32_t { None = 0, Callback = 1, Validation = 3, Write = 4, Factory = 5, Data = 6 };
    // Where a fault happened and whose it was; recorded on the TableView_Error diagnostic.
    enum class Stage : uint32_t
    {
        None = 0, Template = 1, Source = 3, Sort = 4, HeaderRefresh = 5,
        Selection = 6, Editing = 7, Filter = 8, Grouping = 9, Expansion = 10
    };
    enum class Attribution : uint32_t
    {
        Unknown = 0, Product = 1, Application = 2,
        // One past the last attribution: add attributions above it.
        Max
    };

    // Operations TableView tracks for supersession and TableView_Error. The ones with a TiP test are listed by
    // OperationTestIdentity (TableViewTipTests.h).
    constexpr bool IsTrackedOperation(Operation operation) noexcept
    {
        switch (operation)
        {
        case Operation::ReplaceSource:
        case Operation::Sort:
        case Operation::Filter:
        case Operation::GroupConfiguration:
        case Operation::Selection:
        case Operation::BeginEdit:
        case Operation::CommitEdit:
        case Operation::CancelEdit:
        case Operation::GroupExpansion:
        case Operation::HierarchyExpansion:
            return true;
        default:
            return false;
        }
    }

    // Every TiP test's metrics bucket, from inputs read when the test starts: 0 LTR-small, 1 LTR-large, 2 RTL-small,
    // 3 RTL-large, where large is at least c_largeDataRows data rows.
    struct MetricsInputs
    {
        bool rightToLeft{};
        bool largeData{};
    };
    constexpr uint32_t c_largeDataRows = 1000;
    constexpr unsigned char MetricsBucket(bool rightToLeft, bool largeData) noexcept
    {
        return static_cast<unsigned char>((rightToLeft ? 2 : 0) + (largeData ? 1 : 0));
    }
}
