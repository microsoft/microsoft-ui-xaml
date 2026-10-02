// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "TableViewTelemetry.h"
#include "MuxcTraceLogging.h"

#include <combaseapi.h>
#include <bcrypt.h>
#include <chrono>

#pragma comment(lib, "bcrypt.lib")

extern const char* gFileVersion;

namespace TableViewTelemetry
{
    namespace
    {
        constexpr uint32_t schemaVersion = 1;
        constexpr uint32_t sampleDenominator = 16;

        uint64_t Timestamp() noexcept
        {
            return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count());
        }

        bool CreateId(GUID& id) noexcept
        {
            if (SUCCEEDED(CoCreateGuid(&id))) { return true; }
            OutputDebugStringW(L"TableView telemetry: GUID creation failed.\n");
            return false;
        }

        bool EnsureId(State& state) noexcept
        {
            return state.id != GUID{} || CreateId(state.id);
        }

#define TABLEVIEW_FIELDS(state) \
    TraceLoggingLevel(WINEVENT_LEVEL_INFO), \
    TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES), \
    TraceLoggingUInt32(schemaVersion, "SchemaVersion"), \
    TraceLoggingString("TableView", "ControlType"), \
    TraceLoggingString(gFileVersion, "ControlVersion"), \
    TraceLoggingGuid(state.id, "TableViewInstanceId")

        void WriteInitial(State const& state, Result result, Stage stage, double duration) noexcept
        {
            TraceLoggingWrite(g_hTelemetryProvider, "TableView_Initialization", TABLEVIEW_FIELDS(state),
                TraceLoggingUInt32(static_cast<uint32_t>(result), "Result"),
                TraceLoggingUInt32(static_cast<uint32_t>(stage), "FailureStage"),
                TraceLoggingUInt32(static_cast<uint32_t>(state.origin), "StartOrigin"),
                TraceLoggingFloat64(duration, "DurationMs"),
                TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance));
        }

        void WritePerformance(State const& state, Operation operation, Result result, double duration, uint32_t denominator) noexcept
        {
            TraceLoggingWrite(g_hTelemetryProvider, "TableView_Performance", TABLEVIEW_FIELDS(state),
                TraceLoggingUInt32(static_cast<uint32_t>(operation), "Operation"),
                TraceLoggingUInt32(static_cast<uint32_t>(result), "Result"),
                TraceLoggingUInt32(static_cast<uint32_t>(state.origin), "StartOrigin"),
                TraceLoggingFloat64(duration, "DurationMs"),
                TraceLoggingUInt32(denominator, "SampleDenominator"),
                TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance));
        }
    }

    bool IsEnabled() noexcept
    {
        return TraceLoggingProviderEnabled(g_hTelemetryProvider, WINEVENT_LEVEL_INFO, MICROSOFT_KEYWORD_MEASURES) != FALSE;
    }

    bool NeedsLayout(State const& state) noexcept
    {
        return (state.initial == InitialState::Started || !state.usageReported || state.operationStarted) && IsEnabled();
    }

    void BeginInitial(State& state, Origin origin) noexcept
    {
        if (state.initial != InitialState::NotStarted) { return; }
        state.initial = InitialState::Finished;
        if (!IsEnabled() || !EnsureId(state)) { return; }
        state.origin = origin;
        state.initial = InitialState::Started;
        state.initialStarted = Timestamp();
        WriteInitial(state, Result::Started, Stage::None, 0);
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
        auto const started = state.initialStarted;
        state.initialStarted = 0;
        if (!IsEnabled()) { return; }
        auto const duration = (Timestamp() - started) / 1000.0;
        WriteInitial(state, result, stage, duration);
        if (result == Result::Success)
        {
            WritePerformance(state, Operation::InitialLayout, result, duration, 1);
        }
    }

    void ReportUsage(State& state, Configuration const& configuration) noexcept
    {
        if (state.usageReported || !IsEnabled() || !EnsureId(state)) { return; }
        state.usageReported = true;
        TraceLoggingWrite(g_hTelemetryProvider, "TableView_UsageSummary", TABLEVIEW_FIELDS(state),
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
        uint32_t selection{};
        if (BCryptGenRandom(nullptr, reinterpret_cast<PUCHAR>(&selection), sizeof(selection), BCRYPT_USE_SYSTEM_PREFERRED_RNG) < 0)
        {
            OutputDebugStringW(L"TableView telemetry: sampling random source failed.\n");
            return generation;
        }
        if ((selection % sampleDenominator) != 0 || !EnsureId(state)) { return generation; }
        state.operation = operation;
        state.operationStarted = Timestamp();
        return generation;
    }

    void CompleteOperation(State& state, Result result) noexcept
    {
        if (!state.operationStarted) { return; }
        auto const started = state.operationStarted;
        state.operationStarted = 0;
        if (!IsEnabled()) { return; }
        WritePerformance(state, state.operation, result, (Timestamp() - started) / 1000.0, sampleDenominator);
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
        if ((state.reportedErrors & bit) || !IsEnabled() || !EnsureId(state)) { return; }
        state.reportedErrors |= bit;
        TraceLoggingWrite(g_hTelemetryProvider, "TableView_Error", TABLEVIEW_FIELDS(state),
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
#undef TABLEVIEW_FIELDS
}
