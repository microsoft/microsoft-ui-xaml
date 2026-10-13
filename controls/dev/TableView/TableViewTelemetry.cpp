// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "TableViewTelemetry.h"
#include "MuxcTraceLogging.h"

#include <limits>

extern const char* gFileVersion;

namespace TableViewTelemetry
{
    namespace
    {
        constexpr uint32_t schemaVersion = 5;
        constexpr ULONGLONG measuresKeyword = MICROSOFT_KEYWORD_MEASURES | KEYWORD_TABLEVIEW;

#define TABLEVIEW_COMMON_FIELDS \
    TraceLoggingUInt32(schemaVersion, "SchemaVersion"), \
    TraceLoggingString("TableView", "ControlType"), \
    TraceLoggingString(gFileVersion, "ControlVersion"), \
    TraceLoggingGuid(g_TelemetryProviderActivityId, "ProviderSessionId"), \
    TraceLoggingBoolean(g_TelemetryProviderActivityId != GUID{}, "ProviderSessionIdAvailable")

#define TABLEVIEW_INFO_FIELDS \
    TraceLoggingLevel(WINEVENT_LEVEL_INFO), \
    TraceLoggingKeyword(measuresKeyword), \
    TABLEVIEW_COMMON_FIELDS

        TableViewTipDimensions GetDimensions(OperationState const& state) noexcept
        {
            TableViewTipDimensions dimensions;
            dimensions.origin = state.origin;
            return dimensions;
        }

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
            case IgnoreReason::ObservationLimit: return "observation_limit";
            case IgnoreReason::UnattributedFault: return "unattributed_fault";
            default:
                OutputDebugStringW(L"TableView telemetry: invalid ignore reason.\n");
                return nullptr;
            }
        }

        void Increment(Session& session, uint32_t& count, uint32_t bit) noexcept
        {
            if (count == (std::numeric_limits<uint32_t>::max)()) { session.overflowMask |= uint32_t{ 1 } << bit; }
            else { ++count; }
        }

        bool OwnsSession(OperationState const& state) noexcept
        {
            auto const& session = state.usageOwner.session;
            return session.active && state.operationSessionGeneration == session.generation;
        }

        void RecordOutcome(OperationState& state, Result result, std::optional<IgnoreReason> reason = {}) noexcept
        {
            auto& session = state.usageOwner.session;
            if (!OwnsSession(state) || !IsMeasuredOperation(state.operation)) { return; }
            if (result == Result::Success)
            {
                auto const operation = static_cast<uint32_t>(state.operation);
                Increment(session, session.counts[operation], operation);
            }
            else if (reason == IgnoreReason::ApplicationFault) { Increment(session, session.appFaultCount, 13); }
            else if (reason == IgnoreReason::DeferredOrUnproven) { Increment(session, session.deferredCount, 14); }
            else if (reason == IgnoreReason::UnattributedFault) { Increment(session, session.unattributedFaultCount, 15); }
        }

        Stage GetOperationStage(Operation operation) noexcept
        {
            switch (operation)
            {
            case Operation::ReplaceSource: return Stage::Source;
            case Operation::Sort: return Stage::Sort;
            case Operation::Filter: return Stage::Filter;
            case Operation::GroupConfiguration:
            case Operation::GroupExpansion: return Stage::Grouping;
            case Operation::Selection: return Stage::Selection;
            case Operation::BeginEdit:
            case Operation::CommitEdit:
            case Operation::CancelEdit: return Stage::Editing;
            case Operation::Scroll: return Stage::Scroll;
            case Operation::InitialLayout:
            case Operation::Layout: return Stage::Layout;
            case Operation::HeaderRefresh: return Stage::HeaderRefresh;
            default: return Stage::None;
            }
        }

        template<class Test>
        void CompleteTip(Test& test, Result result, Stage stage, std::optional<HRESULT> error) noexcept
        {
            if (result == Result::Success)
            {
                test.set_flag(TIP_reason(TableViewTipReason::scenario_completed));
            }
            else
            {
                {
                    auto data = test.data();
                    data->failureHResultAvailable = error.has_value();
                    data->failureHResult = error.value_or(S_OK);
                }
                switch (stage)
                {
                case Stage::Template:
                case Stage::HeaderRefresh:
                    test.set_flag(TIP_reason(TableViewTipReason::template_failure)); break;
                case Stage::Source:
                    test.set_flag(TIP_reason(TableViewTipReason::source_failure)); break;
                case Stage::Sort:
                case Stage::Filter:
                case Stage::Grouping:
                    test.set_flag(TIP_reason(TableViewTipReason::shaping_failure)); break;
                case Stage::Layout:
                    test.set_flag(TIP_reason(TableViewTipReason::realization_failure)); break;
                case Stage::Selection:
                    test.set_flag(TIP_reason(TableViewTipReason::selection_failure)); break;
                case Stage::Editing:
                    test.set_flag(TIP_reason(TableViewTipReason::editing_failure)); break;
                case Stage::Scroll:
                    test.set_flag(TIP_reason(TableViewTipReason::scroll_failure)); break;
                default:
                    test.set_flag(TIP_reason(TableViewTipReason::internal_failure)); break;
                }
            }
            test.complete();
            test.reset();
        }

        void StopInitial(State& state, Result result) noexcept
        {
            state.initial = InitialState::Finished;
            state.initialOutcome = result == Result::Success ? InitialOutcome::TemplateInitialized :
                result == Result::Failure ? InitialOutcome::Failure : InitialOutcome::Ignored;
            state.initialTipPending = false;
        }

        void StopOperation(OperationState& state) noexcept
        {
            if (state.operationPending) { state.operationHandled = true; }
            state.operationPending = false;
            state.operationTipPending = false;
        }

        template<class Test>
        void IgnoreTip(Test& test, IgnoreReason reason, const char* name, std::optional<HRESULT> error, AppFaultKind kind) noexcept
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
    }

    bool IsEnabled() noexcept
    {
        return TraceLoggingProviderEnabled(g_hTelemetryProvider, WINEVENT_LEVEL_INFO, measuresKeyword) != FALSE;
    }

    bool IsOperationStarted(OperationState const& state) noexcept
    {
        return state.operationPending;
    }

    bool IsOperationHandled(OperationState const& state, Operation operation, uint64_t generation) noexcept
    {
        return generation != 0 && generation == state.operationGeneration &&
            operation == state.operation && state.operationHandled;
    }

    void BeginSession(State& state) noexcept
    {
        if (state.session.active || state.session.generation == (std::numeric_limits<uint64_t>::max)()) { return; }
        auto const generation = state.session.generation + 1;
        state.session = {};
        state.session.generation = generation;
        state.session.active = true;
        state.reportedErrors = 0;
    }

    void EndSession(State& state) noexcept
    {
        if (!state.session.active) { return; }
        IgnoreInitial(state, IgnoreReason::Unloaded);
        IgnoreOperation(state, state.operationGeneration, IgnoreReason::Unloaded);
        auto const enabled = IsEnabled();
        state.session.active = false;
        if (!enabled) { return; }
        auto const& session = state.session;
        TraceLoggingWriteActivity(g_hTelemetryProvider, "TableView_SessionSummary",
            g_TelemetryProviderActivityId != GUID{} ? &g_TelemetryProviderActivityId : nullptr, nullptr, TABLEVIEW_INFO_FIELDS,
            TraceLoggingUInt32(3, "SessionSummaryVersion"),
            TraceLoggingUInt32(static_cast<uint32_t>(state.initialOutcome), "InitialOutcome"),
            TraceLoggingUInt32(session.counts[1], "ReplaceSourceCount"),
            TraceLoggingUInt32(session.counts[2], "SortCount"),
            TraceLoggingUInt32(session.counts[5], "FilterCount"),
            TraceLoggingUInt32(session.counts[6], "GroupConfigurationCount"),
            TraceLoggingUInt32(session.counts[8], "SelectionCount"),
            TraceLoggingUInt32(session.counts[9], "BeginEditCount"),
            TraceLoggingUInt32(session.counts[10], "CommitEditCount"),
            TraceLoggingUInt32(session.counts[11], "CancelEditCount"),
            TraceLoggingUInt32(session.appFaultCount, "AppFaultCount"),
            TraceLoggingUInt32(session.deferredCount, "DeferredCount"),
            TraceLoggingUInt32(session.unattributedFaultCount, "UnattributedFaultCount"),
            TraceLoggingUInt32(session.overflowMask, "CounterOverflowMask"),
            TraceLoggingBoolean(TRUE, "UTCReplace_AppSessionGuid"),
            TelemetryPrivacyDataTag(PDT_ProductAndServiceUsage));
    }

    void AbandonSession(State& state) noexcept
    {
        state.session.active = false;
        IgnoreInitial(state, IgnoreReason::Unloaded);
        IgnoreOperation(state, state.operationGeneration, IgnoreReason::Unloaded);
    }

    void BeginInitial(State& state, Origin origin) noexcept
    {
        if (state.initial != InitialState::NotStarted) { return; }
        state.initial = InitialState::Started;
        state.origin = origin;
        state.initialTest->dimensions = GetDimensions(state);
        state.initialTipPending = state.initialTest.start() != GUID{};
    }

    void CompleteInitial(State& state, Result result, Stage stage, std::optional<HRESULT> error) noexcept
    {
        if (state.initial != InitialState::Started) { return; }
        if (result == Result::Cancelled)
        {
            IgnoreInitial(state, IgnoreReason::Cancelled, stage);
            return;
        }
        if (result != Result::Success && result != Result::Failure)
        {
            OutputDebugStringW(L"TableView telemetry: invalid terminal result.\n");
            return;
        }
        if (state.initialTipPending) { CompleteTip(state.initialTest, result, stage, error); }
        StopInitial(state, result);
    }

    void IgnoreInitial(State& state, IgnoreReason reason, Stage, std::optional<HRESULT> error, AppFaultKind kind) noexcept
    {
        if (state.initial != InitialState::Started) { return; }
        auto const name = GetIgnoreName(reason);
        if (!name) { return; }
        if (state.initialTipPending)
        {
            IgnoreTip(state.initialTest, reason, name, error, kind);
        }
        StopInitial(state, Result::Cancelled);
    }

    uint64_t BeginOperation(OperationState& state, Operation operation) noexcept
    {
        if (static_cast<uint32_t>(operation) > static_cast<uint32_t>(Operation::Scroll))
        {
            OutputDebugStringW(L"TableView telemetry: invalid operation.\n");
            return 0;
        }
        if (!IsMeasuredOperation(operation)) { return 0; }
        IgnoreOperation(state, state.operationGeneration, IgnoreReason::Superseded);
        if (state.operationGeneration == (std::numeric_limits<uint64_t>::max)()) { return 0; }
        auto const generation = ++state.operationGeneration;
        state.operationHandled = false;
        state.operation = operation;
        auto& session = state.usageOwner.session;
        state.operationSessionGeneration = session.active ? session.generation : 0;
        {
            auto data = state.operationTest.data();
            data->dimensions = GetDimensions(state);
            data->operation = operation;
        }
        state.operationTipPending = state.operationTest.start() != GUID{};
        state.operationPending = state.operationTipPending || session.active;
        return generation;
    }

    void CompleteOperation(OperationState& state, uint64_t generation, Result result, std::optional<HRESULT> error) noexcept
    {
        if (generation != state.operationGeneration || !state.operationPending) { return; }
        if (state.operationSessionGeneration && !OwnsSession(state))
        {
            IgnoreOperation(state, generation, IgnoreReason::Stale);
            return;
        }
        if (result == Result::Cancelled)
        {
            IgnoreOperation(state, generation, IgnoreReason::Cancelled);
            return;
        }
        if (result != Result::Success && result != Result::Failure)
        {
            OutputDebugStringW(L"TableView telemetry: invalid terminal result.\n");
            return;
        }
        if (state.operationTipPending) { CompleteTip(state.operationTest, result, GetOperationStage(state.operation), error); }
        RecordOutcome(state, result);
        StopOperation(state);
    }

    void IgnoreOperation(OperationState& state, uint64_t generation, IgnoreReason reason, std::optional<HRESULT> error, AppFaultKind kind) noexcept
    {
        if (generation != state.operationGeneration || !state.operationPending) { return; }
        auto const name = GetIgnoreName(reason);
        if (!name) { return; }
        if (state.operationTipPending)
        {
            IgnoreTip(state.operationTest, reason, name, error, kind);
        }
        RecordOutcome(state, Result::Cancelled, reason);
        StopOperation(state);
    }

    void FailOperation(OperationState& state, Operation operation, uint64_t generation, Stage stage, std::optional<HRESULT> error) noexcept
    {
        if (generation != state.operationGeneration && !(operation == Operation::InitialLayout && generation == 0)) { return; }
        // Zero identifies an initialization boundary before an operation was admitted.
        if (generation != 0 && IsMeasuredOperation(operation) && (!state.operationPending || operation != state.operation)) { return; }
        if (generation != 0 && state.operationSessionGeneration && !OwnsSession(state))
        {
            IgnoreOperation(state, generation, IgnoreReason::Stale);
            return;
        }
        if (generation == state.operationGeneration)
        {
            if (state.operationTipPending) { CompleteTip(state.operationTest, Result::Failure, stage, error); }
            RecordOutcome(state, Result::Failure);
            StopOperation(state);
        }
        if (&state == &state.usageOwner) { CompleteInitial(state.usageOwner, Result::Failure, stage, error); }
        ReportError(state, operation, stage, false, Attribution::Product, error);
    }

    void ReportError(OperationState& state, Operation operation, Stage stage, bool recoverable,
        Attribution attribution, std::optional<HRESULT> error, AppFaultKind kind) noexcept
    {
        if (static_cast<uint32_t>(operation) > static_cast<uint32_t>(Operation::Scroll))
        {
            OutputDebugStringW(L"TableView telemetry: invalid error operation.\n");
            return;
        }
        auto& owner = state.usageOwner;
        if (&owner != &state && state.operationSessionGeneration && !OwnsSession(state)) { return; }
        auto const bit = uint32_t{ 1 } << static_cast<uint32_t>(operation);
        if ((owner.reportedErrors & bit) ||
            !TraceLoggingProviderEnabled(g_hLoggingProvider, WINEVENT_LEVEL_VERBOSE, KEYWORD_TABLEVIEW)) { return; }
        owner.reportedErrors |= bit;
        TraceLoggingWriteActivity(g_hLoggingProvider, "TableView_Error",
            g_TelemetryProviderActivityId != GUID{} ? &g_TelemetryProviderActivityId : nullptr, nullptr,
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE), TraceLoggingKeyword(KEYWORD_TABLEVIEW), TABLEVIEW_COMMON_FIELDS,
            TraceLoggingUInt32(static_cast<uint32_t>(operation), "Operation"),
            TraceLoggingUInt32(static_cast<uint32_t>(stage), "FailureStage"),
            TraceLoggingUInt32(static_cast<uint32_t>(attribution), "Attribution"),
            TraceLoggingHResult(error.value_or(S_OK), "HResult"),
            TraceLoggingBoolean(error.has_value(), "HResultAvailable"),
            TraceLoggingUInt32(static_cast<uint32_t>(kind), "AppFaultKind"),
            TraceLoggingBoolean(recoverable, "IsRecoverable"),
            TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance));
    }

#undef TABLEVIEW_INFO_FIELDS
#undef TABLEVIEW_COMMON_FIELDS
}
