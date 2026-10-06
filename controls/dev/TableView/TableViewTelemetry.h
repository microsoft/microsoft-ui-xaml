// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cstdint>
#include <optional>
#include <TraceLoggingActivity.h>
#include "MuxcTraceLogging.h"
#include "TableViewTipTests.h"

namespace TableViewTelemetry
{
    using Activity = TraceLoggingActivity<
        g_hTelemetryProvider, MICROSOFT_KEYWORD_MEASURES, WINEVENT_LEVEL_INFO>;

    enum class Result : uint32_t { Started = 0, Success = 1, Failure = 2, Cancelled = 3 };
    enum class Stage : uint32_t
    {
        None = 0, Template = 1, Layout = 2, Source = 3, Sort = 4, HeaderRefresh = 5,
        Selection = 6, Editing = 7, Filter = 8, Grouping = 9, Scroll = 10
    };
    enum class InitialState { NotStarted, Started, Finished };

    struct Configuration
    {
        Content content{};
        uint32_t columnCountBucket{};
        bool grouped{};
        bool available{ true };
        uint32_t rowCountBucket{};
        bool rowCountAvailable{};
    };

    // One control's UI-thread state; never retains an item, column or visual.
    struct State
    {
        State() = default;
        State(State const&) = delete;
        State& operator=(State const&) = delete;

        std::optional<Activity> initialActivity;
        std::optional<Activity> operationActivity;
        TableViewInitializationTest initialTest;
        TableViewOperationTest operationTest;
        Configuration configuration{ Content::Empty, 0, false, false };
        InitialState initial{};
        Origin origin{};
        uint64_t operationGeneration{};
        Operation operation{};
        uint32_t reportedErrors{};
        bool usageReported{};
        bool initialTipPending{};
        bool operationTipPending{};
        bool operationPending{};
    };

    bool IsEnabled() noexcept;
    bool IsOperationStarted(State const& state) noexcept;
    bool NeedsLayout(State const& state) noexcept;
    void BeginInitial(State& state, Origin origin) noexcept;
    void CompleteInitial(State& state, Result result, Stage stage = Stage::None, std::optional<HRESULT> error = {}) noexcept;
    void IgnoreInitial(State& state, IgnoreReason reason, Stage stage = Stage::None) noexcept;
    bool UpdateConfiguration(State& state, Configuration const& configuration) noexcept;
    void ReportUsage(State& state, Configuration const& configuration) noexcept;
    uint64_t BeginOperation(State& state, Operation operation) noexcept;
    void CompleteOperation(State& state, Result result, std::optional<HRESULT> error = {}) noexcept;
    void CompleteOperation(State& state, uint64_t generation, Result result, std::optional<HRESULT> error = {}) noexcept;
    void IgnoreOperation(State& state, uint64_t generation, IgnoreReason reason) noexcept;
    void FailOperation(State& state, Operation operation, uint64_t generation, Stage stage, std::optional<HRESULT> error = {}) noexcept;
    void ReportError(State& state, Operation operation, Stage stage, bool recoverable) noexcept;
    uint32_t CountBucket(uint32_t count) noexcept;
}
