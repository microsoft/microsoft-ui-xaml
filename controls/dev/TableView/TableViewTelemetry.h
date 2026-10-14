// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include "TableViewTipTests.h"

namespace TableViewTelemetry
{
    enum class Result : uint32_t { Success = 1, Failure = 2, Cancelled = 3 };
    enum class Stage : uint32_t
    {
        None = 0, Template = 1, Source = 3, Sort = 4, HeaderRefresh = 5,
        Selection = 6, Editing = 7, Filter = 8, Grouping = 9
    };
    enum class InitialState { NotStarted, Started, Finished };
    enum class InitialOutcome : uint32_t { NotObserved = 0, TemplateInitialized = 1, Ignored = 3 };
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
        std::array<uint32_t, 12> counts{};
        uint64_t generation{};
        uint32_t deferredCount{};
        uint32_t unattributedFaultCount{};
        uint32_t overflowMask{};
        bool active{};
    };

    struct State;

    // One in-flight TiP operation; it counts into its owner's loaded-session counters.
    struct OperationState
    {
        explicit OperationState(State& owner) noexcept : usageOwner(owner) {}
        OperationState(OperationState const&) = delete;
        OperationState& operator=(OperationState const&) = delete;

        TableViewOperationTest operationTest;
        State& usageOwner;
        uint64_t operationGeneration{};
        uint64_t operationSessionGeneration{};
        Operation operation{};
        bool operationTipPending{};
        bool operationPending{};
        bool operationHandled{};
    };

    // Only the control owns initialization, loaded-session counters and the diagnostic mask.
    struct State : OperationState
    {
        State() noexcept : OperationState(*this) {}
        TableViewInitializationTest initialTest;
        Session session;
        InitialState initial{};
        InitialOutcome initialOutcome{};
        uint64_t reportedErrors{};
        bool initialTipPending{};
    };

    inline bool IsOperationStarted(OperationState const& state) noexcept { return state.operationPending; }
    bool IsOperationHandled(OperationState const& state, Operation operation, uint64_t generation) noexcept;
    void BeginSession(State& state) noexcept;
    void EndSession(State& state) noexcept;
    void AbandonSession(State& state) noexcept;
    void BeginInitial(State& state) noexcept;
    void CompleteInitial(State& state) noexcept;
    void IgnoreInitial(State& state, IgnoreReason reason,
        std::optional<HRESULT> error = {}, AppFaultKind kind = AppFaultKind::None) noexcept;
    uint64_t BeginOperation(OperationState& state, Operation operation) noexcept;
    void CompleteOperation(OperationState& state, uint64_t generation) noexcept;
    void IgnoreOperation(OperationState& state, uint64_t generation, IgnoreReason reason,
        std::optional<HRESULT> error = {}, AppFaultKind kind = AppFaultKind::None) noexcept;
    void FailOperation(OperationState& state, uint64_t generation, Stage stage, std::optional<HRESULT> error) noexcept;
    void ReportError(OperationState& state, Operation operation, Stage stage, bool recoverable,
        Attribution attribution = Attribution::Unknown, std::optional<HRESULT> error = {},
        AppFaultKind kind = AppFaultKind::None) noexcept;
}
