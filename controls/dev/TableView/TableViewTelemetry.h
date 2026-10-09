// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <TraceLoggingActivity.h>
#include "MuxcTraceLogging.h"
#include "TableViewTipTests.h"

namespace TableViewTelemetry
{
    using Activity = TraceLoggingActivity<
        g_hTelemetryProvider, MICROSOFT_KEYWORD_MEASURES | KEYWORD_TABLEVIEW, WINEVENT_LEVEL_INFO>;

    enum class Result : uint32_t { Started = 0, Success = 1, Failure = 2, Cancelled = 3 };
    enum class Stage : uint32_t
    {
        None = 0, Template = 1, Layout = 2, Source = 3, Sort = 4, HeaderRefresh = 5,
        Selection = 6, Editing = 7, Filter = 8, Grouping = 9, Scroll = 10
    };
    enum class InitialState { NotStarted, Started, Finished };
    enum class InitialOutcome : uint32_t { NotObserved = 0, Ready = 1, Failure = 2, Ignored = 3 };
    enum class Attribution : uint32_t { Unknown = 0, Product = 1, Application = 2 };

    struct ShapingObservation
    {
        bool completed{};
        bool published{};
        bool unproven{};
        AppFaultKind appFaultKind{};
        std::optional<HRESULT> error;

        std::optional<IgnoreReason> Outcome() const noexcept
        {
            if (appFaultKind != AppFaultKind::None) { return IgnoreReason::ApplicationFault; }
            if (error.has_value()) { return IgnoreReason::UnattributedFault; }
            if (!completed) { return IgnoreReason::UnattributedFault; }
            if (!published || unproven) { return IgnoreReason::DeferredOrUnproven; }
            return {};
        }
    };

    struct Session
    {
        std::array<uint32_t, 13> counts{};
        uint64_t generation{};
        uint32_t appFaultCount{};
        uint32_t deferredCount{};
        uint32_t unattributedFaultCount{};
        uint32_t retemplateCount{};
        uint32_t overflowMask{};
        bool active{};
        bool collectorEnabledAtLoad{};
        bool collectorEverEnabled{};
        bool collectorObservedDisabled{};
    };

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
        void (*initialCompleted)(void*) noexcept {};
        void* initialCompletedContext{};
        TableViewInitializationTest initialTest;
        TableViewOperationTest operationTest;
        Configuration configuration{ Content::Empty, 0, false, false };
        Session session;
        // Borrowed only by synchronous control actions and the control-owned scroll state.
        State* usageOwner{};
        InitialState initial{};
        InitialOutcome initialOutcome{};
        Origin origin{};
        uint64_t operationGeneration{};
        uint64_t operationSessionGeneration{};
        Operation operation{};
        uint32_t reportedErrors{};
        bool initialTipPending{};
        bool operationTipPending{};
        bool operationPending{};
        bool operationHandled{};
    };

    bool IsEnabled() noexcept;
    bool IsOperationStarted(State const& state) noexcept;
    bool IsOperationHandled(State const& state, Operation operation, uint64_t generation) noexcept;
    bool NeedsLayout(State const& state) noexcept;
    void BeginSession(State& state) noexcept;
    void EndSession(State& state) noexcept;
    void AbandonSession(State& state) noexcept;
    void RecordRetemplate(State& state) noexcept;
    void BeginInitial(State& state, Origin origin) noexcept;
    void CompleteInitial(State& state, Result result, Stage stage = Stage::None, std::optional<HRESULT> error = {}) noexcept;
    void IgnoreInitial(State& state, IgnoreReason reason, Stage stage = Stage::None,
        std::optional<HRESULT> error = {}, AppFaultKind kind = AppFaultKind::None) noexcept;
    bool UpdateConfiguration(State& state, Configuration const& configuration) noexcept;
    uint64_t BeginOperation(State& state, Operation operation) noexcept;
    void CompleteOperation(State& state, Result result, std::optional<HRESULT> error = {}) noexcept;
    void CompleteOperation(State& state, uint64_t generation, Result result, std::optional<HRESULT> error = {}) noexcept;
    void IgnoreOperation(State& state, uint64_t generation, IgnoreReason reason,
        std::optional<HRESULT> error = {}, AppFaultKind kind = AppFaultKind::None) noexcept;
    void FailOperation(State& state, Operation operation, uint64_t generation, Stage stage, std::optional<HRESULT> error = {}) noexcept;
    void ReportError(State& state, Operation operation, Stage stage, bool recoverable,
        Attribution attribution = Attribution::Unknown, std::optional<HRESULT> error = {},
        AppFaultKind kind = AppFaultKind::None) noexcept;
    uint32_t CountBucket(uint32_t count) noexcept;
}
