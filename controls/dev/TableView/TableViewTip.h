// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cstdint>
#include <optional>
#include "TableViewTipTests.h"
#include "TableViewTelemetry.h"

// TiP: one initialization test per template application and one test per key operation (TableViewTipTests.h).
// A sort, filter or edit commit that ran also marks its feature used on TableView_Instance.
namespace TableViewTip
{
    struct ShapingObservation
    {
        bool completed{};
        bool published{};
        bool unproven{};
        AppFaultKind appFaultKind{};
        std::optional<HRESULT> error;

        // TableView's own work threw or never ran; app faults are tagged, so they are not failures.
        bool Failed() const noexcept { return appFaultKind == AppFaultKind::None && (error.has_value() || !completed); }

        std::optional<IgnoreReason> Ignored() const noexcept
        {
            if (appFaultKind != AppFaultKind::None) { return IgnoreReason::ApplicationFault; }
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

    // The control's tests: initialization, plus the operation test its control and source operations share.
    struct State : OperationState
    {
        using OperationState::OperationState;
        TableViewInitializationTest initializationTest;
        bool initializationPending{};
        bool initializationTipPending{};
        // One verdict per template application; ResetInitialization clears it.
        bool initializationDone{};
    };

    inline bool IsOperationStarted(OperationState const& state) noexcept { return state.operationPending; }
    bool IsOperationHandled(OperationState const& state, Operation operation, uint64_t generation) noexcept;
    // Starts Initialize unless it is running or this template application already has its verdict.
    void StartInitialization(State& state, unsigned char metricsBucket) noexcept;
    void ResetInitialization(State& state) noexcept;
    void CompleteInitialization(State& state) noexcept;
    void FailInitialization(State& state, Stage stage, HRESULT error) noexcept;
    void IgnoreInitialization(State& state, IgnoreReason reason,
        std::optional<HRESULT> error = {}, AppFaultKind kind = AppFaultKind::None) noexcept;
    uint64_t BeginOperation(OperationState& state, Operation operation) noexcept;
    void CompleteOperation(OperationState& state, uint64_t generation) noexcept;
    void IgnoreOperation(OperationState& state, uint64_t generation, IgnoreReason reason,
        std::optional<HRESULT> error = {}, AppFaultKind kind = AppFaultKind::None) noexcept;
    void FailOperation(OperationState& state, uint64_t generation, Stage stage, std::optional<HRESULT> error) noexcept;
}
