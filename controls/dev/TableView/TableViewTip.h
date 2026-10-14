// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cstdint>
#include <optional>
#include "TableViewTipTests.h"
#include "TableViewTelemetry.h"

// TiP: one setup test per template application and one test per key operation (TableViewTipTests.h).
// Completed operations, and deferred or unattributed ones, also count into the session telemetry.
namespace TableViewTip
{
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

    // One in-flight operation test.
    struct OperationState
    {
        explicit OperationState(TableViewTelemetry::Session& session) noexcept : session(session) {}
        OperationState(OperationState const&) = delete;
        OperationState& operator=(OperationState const&) = delete;

        TableViewOperationTest operationTest;
        TableViewTelemetry::Session& session;
        uint64_t operationGeneration{};
        uint64_t operationSessionGeneration{};
        Operation operation{};
        bool operationTipPending{};
        bool operationPending{};
        bool operationHandled{};
    };

    // The control's tests: setup, plus the operation test its control and source operations share.
    struct State : OperationState
    {
        using OperationState::OperationState;
        TableViewInitializationTest setupTest;
        bool setupTipPending{};
    };

    inline bool IsOperationStarted(OperationState const& state) noexcept { return state.operationPending; }
    bool IsOperationHandled(OperationState const& state, Operation operation, uint64_t generation) noexcept;
    void StartSetup(State& state) noexcept;
    void CompleteSetup(State& state) noexcept;
    void FailSetup(State& state, HRESULT error) noexcept;
    void IgnoreSetup(State& state, IgnoreReason reason,
        std::optional<HRESULT> error = {}, AppFaultKind kind = AppFaultKind::None) noexcept;
    uint64_t BeginOperation(OperationState& state, Operation operation) noexcept;
    void CompleteOperation(OperationState& state, uint64_t generation) noexcept;
    void IgnoreOperation(OperationState& state, uint64_t generation, IgnoreReason reason,
        std::optional<HRESULT> error = {}, AppFaultKind kind = AppFaultKind::None) noexcept;
    void FailOperation(OperationState& state, uint64_t generation, Stage stage, std::optional<HRESULT> error) noexcept;
}
