// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "TableViewTip.h"

#include <limits>

namespace TableViewTip
{
    namespace
    {
        const char* GetIgnoreName(IgnoreReason reason) noexcept
        {
            switch (reason)
            {
            case IgnoreReason::Cancelled: return "cancelled";
            case IgnoreReason::Vetoed: return "vetoed";
            case IgnoreReason::ValidationRejected: return "validation_rejected";
            case IgnoreReason::Superseded: return "superseded";
            case IgnoreReason::Unloaded: return "unloaded";
            case IgnoreReason::Retemplated: return "retemplated";
            case IgnoreReason::Stale: return "stale";
            case IgnoreReason::DeferredOrUnproven: return "deferred_or_unproven";
            case IgnoreReason::ApplicationFault: return "application_fault";
            default:
                OutputDebugStringW(L"TableView TiP: invalid ignore reason.\n");
                return nullptr;
            }
        }

        template<class Test>
        void CloseCompleted(Test& test) noexcept
        {
            test.set_flag(TIP_reason(TableViewTipReason::scenario_completed));
            test.complete();
            test.reset();
        }

        template<class Test>
        void CloseFailed(Test& test, std::optional<HRESULT> error) noexcept
        {
            {
                auto data = test.data();
                data->failureHResultAvailable = error.has_value();
                data->failureHResult = error.value_or(S_OK);
            }
            test.set_flag(TIP_reason(TableViewTipReason::internal_failure));
            test.complete();
            test.reset();
        }

        template<class Test>
        void CloseIgnored(Test& test, IgnoreReason reason, const char* name, std::optional<HRESULT> error, AppFaultKind kind) noexcept
        {
            {
                auto data = test.data();
                data->diagnosticHResult = error.value_or(S_OK);
                data->diagnosticHResultAvailable = error.has_value();
                data->appFaultKind = kind;
            }
            test.complete_and_ignore(static_cast<unsigned short>(reason), name);
            test.reset();
        }

        bool OwnsSession(OperationState const& state) noexcept
        {
            return state.session.active && state.operationSessionGeneration == state.session.generation;
        }

        void StopOperation(OperationState& state) noexcept
        {
            if (state.operationPending) { state.operationHandled = true; }
            state.operationPending = false;
            state.operationTipPending = false;
        }

        // Ends the setup run; returns whether its TiP test was running and still needs closing.
        bool StopSetup(State& state) noexcept
        {
            auto const tipPending = state.setupTipPending;
            state.setupPending = false;
            state.setupTipPending = false;
            return tipPending;
        }
    }

    bool IsOperationHandled(OperationState const& state, Operation operation, uint64_t generation) noexcept
    {
        return generation != 0 && generation == state.operationGeneration &&
            operation == state.operation && state.operationHandled;
    }

    // Every template application is one setup run; OnApplyTemplate ignores an unfinished one first.
    // The run is tracked even when TiP is unavailable, so a setup failure still gets its diagnostic.
    void StartSetup(State& state) noexcept
    {
        state.setupPending = true;
        state.setupTipPending = state.setupTest.start() != GUID{};
    }

    void CompleteSetup(State& state) noexcept
    {
        if (!state.setupPending) { return; }
        if (StopSetup(state)) { CloseCompleted(state.setupTest); }
    }

    void FailSetup(State& state, HRESULT error) noexcept
    {
        if (!state.setupPending) { return; }
        if (StopSetup(state)) { CloseFailed(state.setupTest, error); }
        TableViewTelemetry::ReportError(state.session, 0, Operation::Setup, Stage::Template, false, Attribution::Product, error);
    }

    void IgnoreSetup(State& state, IgnoreReason reason, std::optional<HRESULT> error, AppFaultKind kind) noexcept
    {
        if (!state.setupPending) { return; }
        auto const name = GetIgnoreName(reason);
        if (!name) { return; }
        if (StopSetup(state)) { CloseIgnored(state.setupTest, reason, name, error, kind); }
    }

    uint64_t BeginOperation(OperationState& state, Operation operation) noexcept
    {
        if (static_cast<uint32_t>(operation) >= static_cast<uint32_t>(Operation::Max))
        {
            OutputDebugStringW(L"TableView TiP: invalid operation.\n");
            return 0;
        }
        if (!IsMeasuredOperation(operation)) { return 0; }
        IgnoreOperation(state, state.operationGeneration, IgnoreReason::Superseded);
        if (state.operationGeneration == (std::numeric_limits<uint64_t>::max)()) { return 0; }
        auto const generation = ++state.operationGeneration;
        state.operationHandled = false;
        state.operation = operation;
        state.operationSessionGeneration = state.session.active ? state.session.generation : 0;
        {
            auto data = state.operationTest.data();
            data->operation = operation;
        }
        state.operationTipPending = state.operationTest.start() != GUID{};
        state.operationPending = state.operationTipPending || state.session.active;
        return generation;
    }

    void CompleteOperation(OperationState& state, uint64_t generation) noexcept
    {
        if (generation != state.operationGeneration || !state.operationPending) { return; }
        if (state.operationSessionGeneration && !OwnsSession(state))
        {
            IgnoreOperation(state, generation, IgnoreReason::Stale);
            return;
        }
        if (state.operationTipPending) { CloseCompleted(state.operationTest); }
        TableViewTelemetry::CountOperation(state.session, state.operationSessionGeneration, state.operation);
        StopOperation(state);
    }

    void IgnoreOperation(OperationState& state, uint64_t generation, IgnoreReason reason, std::optional<HRESULT> error, AppFaultKind kind) noexcept
    {
        if (generation != state.operationGeneration || !state.operationPending) { return; }
        auto const name = GetIgnoreName(reason);
        if (!name) { return; }
        if (state.operationTipPending)
        {
            CloseIgnored(state.operationTest, reason, name, error, kind);
        }
        TableViewTelemetry::CountOperation(state.session, state.operationSessionGeneration, state.operation, reason);
        StopOperation(state);
    }

    void FailOperation(OperationState& state, uint64_t generation, Stage stage, std::optional<HRESULT> error) noexcept
    {
        if (!generation || generation != state.operationGeneration || !state.operationPending) { return; }
        if (state.operationSessionGeneration && !OwnsSession(state))
        {
            IgnoreOperation(state, generation, IgnoreReason::Stale);
            return;
        }
        if (state.operationTipPending) { CloseFailed(state.operationTest, error); }
        StopOperation(state);
        TableViewTelemetry::ReportError(state.session, state.operationSessionGeneration, state.operation, stage, false,
            Attribution::Product, error);
    }
}
