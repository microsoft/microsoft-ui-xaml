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
            case IgnoreReason::AppTemplateIncomplete: return "app_template_incomplete";
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

        // TiP names a flag after its spelling, so each initialization failure reason is written out.
        void CloseInitializationFailed(TableViewInitializationTest& test, Stage stage, HRESULT error) noexcept
        {
            {
                auto data = test.data();
                data->failureHResultAvailable = true;
                data->failureHResult = error;
            }
            if (stage == Stage::Source) { test.set_flag(TIP_reason(TableViewTipReason::source_attach_failed)); }
            else { test.set_flag(TIP_reason(TableViewTipReason::template_setup_failed)); }
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

        // Ends the initialization run with its verdict; returns whether its TiP test was running and still needs closing.
        bool StopInitialization(State& state) noexcept
        {
            auto const tipPending = state.initializationTipPending;
            state.initializationPending = false;
            state.initializationTipPending = false;
            state.initializationDone = true;
            return tipPending;
        }
    }

    bool IsOperationHandled(OperationState const& state, Operation operation, uint64_t generation) noexcept
    {
        return generation != 0 && generation == state.operationGeneration &&
            operation == state.operation && state.operationHandled;
    }

    // The run is tracked even when TiP is unavailable, so a failure still gets its diagnostic.
    void StartInitialization(State& state, unsigned char metricsBucket) noexcept
    {
        if (state.initializationPending || state.initializationDone) { return; }
        state.initializationPending = true;
        {
            auto data = state.initializationTest.data();
            data->metricsBucket = metricsBucket;
        }
        state.initializationTipPending = state.initializationTest.start() != GUID{};
    }

    // A new template application; OnApplyTemplate ignores an unfinished run first.
    void ResetInitialization(State& state) noexcept
    {
        state.initializationDone = false;
    }

    void CompleteInitialization(State& state) noexcept
    {
        if (!state.initializationPending) { return; }
        if (StopInitialization(state)) { CloseCompleted(state.initializationTest); }
    }

    // Stage::Source fails with source_attach_failed, any other stage with template_setup_failed. The diagnostic is
    // written even when Initialize already has its verdict.
    void FailInitialization(State& state, Stage stage, HRESULT error) noexcept
    {
        if (state.initializationPending && StopInitialization(state))
        {
            CloseInitializationFailed(state.initializationTest, stage, error);
        }
        TableViewTelemetry::ReportError(state.session, 0, Operation::Setup, stage, false, Attribution::Product, error);
    }

    void IgnoreInitialization(State& state, IgnoreReason reason, std::optional<HRESULT> error, AppFaultKind kind) noexcept
    {
        if (!state.initializationPending) { return; }
        auto const name = GetIgnoreName(reason);
        if (!name) { return; }
        if (StopInitialization(state)) { CloseIgnored(state.initializationTest, reason, name, error, kind); }
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
        TableViewTelemetry::MarkUsed(state.session, state.operationSessionGeneration, TableViewTelemetry::FeatureOf(state.operation));
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
        // The operation ran; TiP only could not prove it published.
        if (reason == IgnoreReason::DeferredOrUnproven)
        {
            TableViewTelemetry::MarkUsed(state.session, state.operationSessionGeneration, TableViewTelemetry::FeatureOf(state.operation));
        }
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
