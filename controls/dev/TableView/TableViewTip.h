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
    // An exception's HRESULT and, when app code threw it, which app code (AppFaultKind::None otherwise).
    struct Fault
    {
        HRESULT error{ E_FAIL };
        AppFaultKind appFault{};
    };
    // In a catch: the current exception as a Fault. App code's exceptions carry their kind in their type
    // (RethrowAppFault, or shaping for the app's collection), so whoever catches one can attribute it.
    Fault CurrentFault() noexcept;
    // In a catch around a call into app code: rethrows the exception as the app's, with the same HRESULT and error
    // info (out of memory stays a std::bad_alloc). An exception already tagged keeps its kind.
    [[noreturn]] void RethrowAppFault(AppFaultKind kind);

    // An app fault leaving a callback that a projected object made (SelectionModel calls TableView back through the
    // ABI). The ABI keeps the exception's error object but drops its C++ type, so the call into that object tags the
    // exception again when its error object is this one; the HRESULT alone never matches.
    struct AppFaultHop
    {
        winrt::com_ptr<::IUnknown> error;
        AppFaultKind kind{};
    };
    // In a catch, before the exception leaves the callback: the hop when app code threw it, else empty.
    AppFaultHop CurrentAppFaultHop() noexcept;
    // In a catch where the projected call returned: rethrows the exception as the app's again when it is the hop's.
    void RestoreAppFault(AppFaultHop const& hop);

    struct ShapingObservation
    {
        bool completed{};
        bool published{};
        bool unproven{};
        AppFaultKind appFaultKind{};
        std::optional<HRESULT> error;
        ContainedFaults contained;

        // TableView's own work threw or never ran; app faults are tagged, so they are not failures.
        bool Failed() const noexcept { return appFaultKind == AppFaultKind::None && (error.has_value() || !completed); }

        std::optional<IgnoreReason> Ignored() const noexcept
        {
            if (appFaultKind != AppFaultKind::None) { return IgnoreReason::ApplicationFault; }
            if (!published || unproven) { return IgnoreReason::DeferredOrUnproven; }
            return {};
        }
    };

    // One in-flight tracked operation, and its TiP test when the operation has one.
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
    void StartInitialization(State& state, MetricsInputs metrics) noexcept;
    void ResetInitialization(State& state) noexcept;
    void CompleteInitialization(State& state) noexcept;
    void FailInitialization(State& state, Stage stage, HRESULT error) noexcept;
    void IgnoreInitialization(State& state, IgnoreReason reason,
        std::optional<HRESULT> error = {}, AppFaultKind kind = AppFaultKind::None) noexcept;
    // metrics is read only by an operation with a TiP test (OperationTestIdentity); withTest false runs none.
    uint64_t BeginOperation(OperationState& state, Operation operation, MetricsInputs metrics, bool withTest = true) noexcept;
    void CompleteOperation(OperationState& state, uint64_t generation) noexcept;
    void IgnoreOperation(OperationState& state, uint64_t generation, IgnoreReason reason,
        std::optional<HRESULT> error = {}, AppFaultKind kind = AppFaultKind::None) noexcept;
    void FailOperation(OperationState& state, uint64_t generation, Stage stage, std::optional<HRESULT> error) noexcept;
}
