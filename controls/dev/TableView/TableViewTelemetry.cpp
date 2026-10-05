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
    }

    bool IsEnabled() noexcept
    {
        return TraceLoggingProviderEnabled(g_hTelemetryProvider, WINEVENT_LEVEL_INFO, MICROSOFT_KEYWORD_MEASURES) != FALSE;
    }

    bool NeedsLayout(State const& state) noexcept
    {
        return (state.initial == InitialState::Started || !state.usageReported || IsOperationStarted(state)) && IsEnabled();
    }

    bool IsOperationStarted(State const& state) noexcept
    {
        return state.operationActivity && state.operationActivity->IsStarted();
    }

    void BeginInitial(State& state, Origin origin) noexcept
    {
        if (state.initial != InitialState::NotStarted) { return; }
        state.initial = InitialState::Finished;
        if (!IsEnabled()) { return; }
        state.origin = origin;
        state.initial = InitialState::Started;
        state.initialActivity.emplace();
        TraceLoggingWriteStart(*state.initialActivity, "TableView_Initialization", TABLEVIEW_COMMON_FIELDS,
            TraceLoggingUInt32(static_cast<uint32_t>(state.origin), "StartOrigin"),
            TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance));
    }

    void CompleteInitial(State& state, Result result, Stage stage) noexcept
    {
        if (state.initial != InitialState::Started) { return; }
        if (result == Result::Started)
        {
            OutputDebugStringW(L"TableView telemetry: invalid terminal result.\n");
            return;
        }
        state.initial = InitialState::Finished;
        TraceLoggingWriteStop(*state.initialActivity, "TableView_Initialization", TABLEVIEW_COMMON_FIELDS,
            TraceLoggingUInt32(static_cast<uint32_t>(result), "Result"),
            TraceLoggingUInt32(static_cast<uint32_t>(stage), "FailureStage"),
            TraceLoggingUInt32(static_cast<uint32_t>(state.origin), "StartOrigin"),
            TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance));
    }

    void ReportUsage(State& state, Configuration const& configuration) noexcept
    {
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
        CompleteOperation(state, Result::Cancelled);
        auto const generation = ++state.operationGeneration;
        if (!IsEnabled()) { return generation; }
        state.operation = operation;
        state.operationActivity.emplace();
        if (state.initialActivity)
        {
            state.operationActivity->SetRelatedActivity(*state.initialActivity);
        }
        TraceLoggingWriteStart(*state.operationActivity, "TableView_Performance", TABLEVIEW_COMMON_FIELDS,
            TraceLoggingUInt32(static_cast<uint32_t>(state.operation), "Operation"),
            TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance));
        return generation;
    }

    void CompleteOperation(State& state, Result result) noexcept
    {
        if (!IsOperationStarted(state)) { return; }
        if (result == Result::Started)
        {
            OutputDebugStringW(L"TableView telemetry: invalid terminal result.\n");
            return;
        }
        TraceLoggingWriteStop(*state.operationActivity, "TableView_Performance", TABLEVIEW_COMMON_FIELDS,
            TraceLoggingUInt32(static_cast<uint32_t>(state.operation), "Operation"),
            TraceLoggingUInt32(static_cast<uint32_t>(result), "Result"),
            TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance));
        state.operationActivity.reset();
    }

    void FailOperation(State& state, Operation operation, uint64_t generation, Stage stage) noexcept
    {
        if (generation == state.operationGeneration)
        {
            CompleteOperation(state, Result::Failure);
        }
        CompleteInitial(state, Result::Failure, stage);
        ReportError(state, operation, stage, false);
    }

    void ReportError(State& state, Operation operation, Stage stage, bool recoverable) noexcept
    {
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
