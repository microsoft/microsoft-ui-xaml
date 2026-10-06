// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "TableViewTelemetry.h"
#include "MuxcTraceLogging.h"

extern const char* gFileVersion;

namespace TableViewTelemetry
{
    namespace
    {
        constexpr uint32_t schemaVersion = 2;

#define TABLEVIEW_COMMON_FIELDS \
    TraceLoggingUInt32(schemaVersion, "SchemaVersion"), \
    TraceLoggingString("TableView", "ControlType"), \
    TraceLoggingString(gFileVersion, "ControlVersion")

#define TABLEVIEW_INFO_FIELDS \
    TraceLoggingLevel(WINEVENT_LEVEL_INFO), \
    TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES), \
    TABLEVIEW_COMMON_FIELDS

        TableViewTipDimensions GetDimensions(State const& state) noexcept
        {
            TableViewTipDimensions dimensions;
            dimensions.origin = state.origin;
            dimensions.content = state.configuration.content;
            dimensions.rowCountBucket = state.configuration.rowCountBucket;
            dimensions.columnCountBucket = state.configuration.columnCountBucket;
            dimensions.configurationAvailable = state.configuration.available;
            dimensions.rowCountAvailable = state.configuration.rowCountAvailable;
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
            default:
                OutputDebugStringW(L"TableView telemetry: invalid ignore reason.\n");
                return nullptr;
            }
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

        void StopInitialActivity(State& state, Result result, Stage stage) noexcept
        {
            state.initial = InitialState::Finished;
            state.initialTipPending = false;
            if (state.initialActivity && state.initialActivity->IsStarted())
            {
                TraceLoggingWriteStop(*state.initialActivity, "TableView_Initialization", TABLEVIEW_COMMON_FIELDS,
                    TraceLoggingUInt32(static_cast<uint32_t>(result), "Result"),
                    TraceLoggingUInt32(static_cast<uint32_t>(stage), "FailureStage"),
                    TraceLoggingUInt32(static_cast<uint32_t>(state.origin), "StartOrigin"),
                    TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance));
            }
        }

        void StopOperationActivity(State& state, Result result) noexcept
        {
            state.operationPending = false;
            state.operationTipPending = false;
            if (state.operationActivity && state.operationActivity->IsStarted())
            {
                TraceLoggingWriteStop(*state.operationActivity, "TableView_Performance", TABLEVIEW_COMMON_FIELDS,
                    TraceLoggingUInt32(static_cast<uint32_t>(state.operation), "Operation"),
                    TraceLoggingUInt32(static_cast<uint32_t>(result), "Result"),
                    TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance));
            }
            state.operationActivity.reset();
        }

        void ReportFeatureUse(State const& state) noexcept
        {
            if (!IsEnabled() || !IsMeasuredOperation(state.operation)) { return; }
            // Each completed action is also evidence of an active control in that backend window.
            TraceLoggingWriteActivity(g_hTelemetryProvider, "TableView_FeatureUse",
                state.initialActivity ? state.initialActivity->Id() : nullptr, nullptr, TABLEVIEW_INFO_FIELDS,
                TraceLoggingUInt32(1, "FeatureUseVersion"),
                TraceLoggingUInt32(static_cast<uint32_t>(state.operation), "Operation"),
                TraceLoggingBoolean(true, "ObservedActiveTableView"),
                TraceLoggingUInt32(static_cast<uint32_t>(state.configuration.content), "ContentKind"),
                TraceLoggingUInt32(state.configuration.columnCountBucket, "ColumnCountBucket"),
                TraceLoggingUInt32(state.configuration.rowCountBucket, "RowCountBucket"),
                TraceLoggingBoolean(state.configuration.rowCountAvailable, "RowCountAvailable"),
                TraceLoggingBoolean(state.configuration.available, "ConfigurationAvailable"),
                TelemetryPrivacyDataTag(PDT_ProductAndServiceUsage));
        }
    }

    bool IsEnabled() noexcept
    {
        return TraceLoggingProviderEnabled(g_hTelemetryProvider, WINEVENT_LEVEL_INFO, MICROSOFT_KEYWORD_MEASURES) != FALSE;
    }

    bool NeedsLayout(State const& state) noexcept
    {
        return state.initial == InitialState::Started || IsOperationStarted(state) ||
            (!state.usageReported && IsEnabled());
    }

    bool IsOperationStarted(State const& state) noexcept
    {
        return state.operationPending;
    }

    void BeginInitial(State& state, Origin origin) noexcept
    {
        if (state.initial != InitialState::NotStarted) { return; }
        state.initial = InitialState::Finished;
        state.origin = origin;
        state.initialTest->dimensions = GetDimensions(state);
        state.initialTipPending = state.initialTest.start() != GUID{};
        if (state.initialTipPending) { state.initial = InitialState::Started; }
        if (IsEnabled())
        {
            state.initial = InitialState::Started;
            state.initialActivity.emplace();
            TraceLoggingWriteStart(*state.initialActivity, "TableView_Initialization", TABLEVIEW_COMMON_FIELDS,
                TraceLoggingUInt32(static_cast<uint32_t>(state.origin), "StartOrigin"),
                TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance));
        }
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
        StopInitialActivity(state, result, stage);
    }

    void IgnoreInitial(State& state, IgnoreReason reason, Stage stage) noexcept
    {
        if (state.initial != InitialState::Started) { return; }
        auto const name = GetIgnoreName(reason);
        if (!name) { return; }
        if (state.initialTipPending)
        {
            // This supported completion bypasses success requirements in the released TiP library.
            state.initialTest.complete_and_ignore(static_cast<unsigned short>(reason), name);
            state.initialTest.reset();
        }
        StopInitialActivity(state, Result::Cancelled, stage);
    }

    bool UpdateConfiguration(State& state, Configuration const& configuration) noexcept
    {
        if (configuration.rowCountBucket > 4 || configuration.columnCountBucket > 4 ||
            static_cast<uint32_t>(configuration.content) > static_cast<uint32_t>(Content::GroupHeaders))
        {
            OutputDebugStringW(L"TableView telemetry: invalid configuration buckets.\n");
            return false;
        }
        state.configuration = configuration;
        auto const dimensions = GetDimensions(state);
        if (state.initialTipPending) { state.initialTest->dimensions = dimensions; }
        if (state.operationTipPending) { state.operationTest->dimensions = dimensions; }
        return true;
    }

    void ReportUsage(State& state, Configuration const& configuration) noexcept
    {
        if (!UpdateConfiguration(state, configuration)) { return; }
        if (state.usageReported || !IsEnabled()) { return; }
        state.usageReported = true;
        TraceLoggingWriteActivity(g_hTelemetryProvider, "TableView_UsageSummary",
            state.initialActivity ? state.initialActivity->Id() : nullptr, nullptr, TABLEVIEW_INFO_FIELDS,
            TraceLoggingUInt32(static_cast<uint32_t>(configuration.content), "ContentKind"),
            TraceLoggingUInt32(configuration.columnCountBucket, "ColumnCountBucket"),
            TraceLoggingBoolean(configuration.grouped, "IsGrouped"),
            TraceLoggingBoolean(configuration.available, "ConfigurationAvailable"),
            TelemetryPrivacyDataTag(PDT_ProductAndServiceUsage));
    }

    uint64_t BeginOperation(State& state, Operation operation) noexcept
    {
        IgnoreOperation(state, state.operationGeneration, IgnoreReason::Superseded);
        auto const generation = ++state.operationGeneration;
        if (static_cast<uint32_t>(operation) > static_cast<uint32_t>(Operation::Scroll))
        {
            OutputDebugStringW(L"TableView telemetry: invalid operation.\n");
            return generation;
        }
        state.operation = operation;
        if (IsMeasuredOperation(operation))
        {
            {
                auto data = state.operationTest.data();
                data->dimensions = GetDimensions(state);
                data->operation = operation;
            }
            state.operationTipPending = state.operationTest.start() != GUID{};
        }
        state.operationPending = state.operationTipPending;
        if (IsEnabled())
        {
            state.operationActivity.emplace();
            if (state.initialActivity)
            {
                state.operationActivity->SetRelatedActivity(*state.initialActivity);
            }
            TraceLoggingWriteStart(*state.operationActivity, "TableView_Performance", TABLEVIEW_COMMON_FIELDS,
                TraceLoggingUInt32(static_cast<uint32_t>(state.operation), "Operation"),
                TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance));
            state.operationPending = state.operationPending || state.operationActivity->IsStarted();
        }
        return generation;
    }

    void CompleteOperation(State& state, Result result, std::optional<HRESULT> error) noexcept
    {
        CompleteOperation(state, state.operationGeneration, result, error);
    }

    void CompleteOperation(State& state, uint64_t generation, Result result, std::optional<HRESULT> error) noexcept
    {
        if (generation != state.operationGeneration || !state.operationPending) { return; }
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
        StopOperationActivity(state, result);
        if (result == Result::Success) { ReportFeatureUse(state); }
    }

    void IgnoreOperation(State& state, uint64_t generation, IgnoreReason reason) noexcept
    {
        if (generation != state.operationGeneration || !state.operationPending) { return; }
        auto const name = GetIgnoreName(reason);
        if (!name) { return; }
        if (state.operationTipPending)
        {
            state.operationTest.complete_and_ignore(static_cast<unsigned short>(reason), name);
            state.operationTest.reset();
        }
        StopOperationActivity(state, Result::Cancelled);
    }

    void FailOperation(State& state, Operation operation, uint64_t generation, Stage stage, std::optional<HRESULT> error) noexcept
    {
        if (generation != state.operationGeneration && !(operation == Operation::InitialLayout && generation == 0)) { return; }
        // Zero identifies an initialization boundary before an operation was admitted.
        if (generation != 0 && IsMeasuredOperation(operation) && (!state.operationPending || operation != state.operation)) { return; }
        if (generation == state.operationGeneration)
        {
            if (state.operationTipPending) { CompleteTip(state.operationTest, Result::Failure, stage, error); }
            StopOperationActivity(state, Result::Failure);
        }
        CompleteInitial(state, Result::Failure, stage, error);
        ReportError(state, operation, stage, false);
    }

    void ReportError(State& state, Operation operation, Stage stage, bool recoverable) noexcept
    {
        if (static_cast<uint32_t>(operation) > static_cast<uint32_t>(Operation::Scroll))
        {
            OutputDebugStringW(L"TableView telemetry: invalid error operation.\n");
            return;
        }
        auto const bit = uint32_t{ 1 } << static_cast<uint32_t>(operation);
        if ((state.reportedErrors & bit) || !IsEnabled()) { return; }
        state.reportedErrors |= bit;
        TraceLoggingWriteActivity(g_hTelemetryProvider, "TableView_Error",
            state.initialActivity ? state.initialActivity->Id() : nullptr, nullptr, TABLEVIEW_INFO_FIELDS,
            TraceLoggingUInt32(static_cast<uint32_t>(operation), "Operation"),
            TraceLoggingUInt32(static_cast<uint32_t>(stage), "FailureStage"),
            // These boundaries can call app-supplied templates and delegates.
            TraceLoggingUInt32(0, "Attribution"),
            TraceLoggingBoolean(recoverable, "IsRecoverable"),
            TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance));
    }

    uint32_t CountBucket(uint32_t count) noexcept
    {
        if (count <= 1) { return count; }
        if (count <= 4) { return 2; }
        if (count <= 16) { return 3; }
        return 4;
    }
#undef TABLEVIEW_INFO_FIELDS
#undef TABLEVIEW_COMMON_FIELDS
}
