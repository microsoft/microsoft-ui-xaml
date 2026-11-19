// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "TableViewTelemetry.h"
#include "MuxcTraceLogging.h"

#include <algorithm>
#include <atomic>
#include <limits>

extern const char* gFileVersion;

namespace TableViewTelemetry
{
    namespace
    {
        constexpr uint32_t schemaVersion = 1;
        constexpr uint32_t c_instanceVersion = 1;
        // A table that is reloaded or recreated writes one TableView_Instance per load; this bounds a process.
        constexpr uint32_t c_maxInstanceEvents = 64;
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

        bool IsCurrent(Session const& session, uint64_t startedIn) noexcept
        {
            return session.active && startedIn == session.generation;
        }

        bool IsEnabled() noexcept
        {
            return TraceLoggingProviderEnabled(g_hTelemetryProvider, WINEVENT_LEVEL_INFO, measuresKeyword) != FALSE;
        }

        // TableView_Instance events this process has written, against c_maxInstanceEvents.
        std::atomic<uint32_t> s_instancesWritten{};

        // The diagnostic dedupe mask has a bit per attribution and recoverability in each diagnostic slot.
        constexpr uint32_t c_attributions = static_cast<uint32_t>(TableViewTip::Attribution::Max);
        constexpr uint32_t c_errorBitsPerSlot = c_attributions * 2;
        constexpr uint32_t c_errorMaskBits = static_cast<uint32_t>(decltype(Session::reportedErrors){}.size());

        // One diagnostic per operation (setup and header refresh included), attribution and recoverability, per session.
        constexpr std::optional<uint32_t> DiagnosticSlot(TableViewTip::Operation operation) noexcept
        {
            switch (operation)
            {
            case TableViewTip::Operation::Setup: return 0;
            case TableViewTip::Operation::HeaderRefresh: return 1;
            case TableViewTip::Operation::ReplaceSource: return 2;
            case TableViewTip::Operation::Sort: return 3;
            case TableViewTip::Operation::Filter: return 4;
            case TableViewTip::Operation::GroupConfiguration: return 5;
            case TableViewTip::Operation::Selection: return 6;
            case TableViewTip::Operation::BeginEdit: return 7;
            case TableViewTip::Operation::CommitEdit: return 8;
            case TableViewTip::Operation::CancelEdit: return 9;
            case TableViewTip::Operation::GroupExpansion: return 10;
            case TableViewTip::Operation::HierarchyExpansion: return 11;
            default: return std::nullopt;
            }
        }

        // Every operation that can report has a slot, and every slot's bits fit the dedupe mask.
        constexpr bool SlotsFit() noexcept
        {
            for (uint32_t value = 0; value < static_cast<uint32_t>(TableViewTip::Operation::Max); ++value)
            {
                auto const operation = static_cast<TableViewTip::Operation>(value);
                auto const reports = TableViewTip::IsTrackedOperation(operation) ||
                    operation == TableViewTip::Operation::Setup || operation == TableViewTip::Operation::HeaderRefresh;
                auto const slot = DiagnosticSlot(operation);
                if (reports != slot.has_value() || (slot && (*slot + 1) * c_errorBitsPerSlot > c_errorMaskBits))
                {
                    return false;
                }
            }
            return true;
        }
        static_assert(SlotsFit(), "Update IsTrackedOperation and DiagnosticSlot together.");
    }

    bool ShouldSample() noexcept
    {
        return s_instancesWritten.load(std::memory_order_relaxed) < c_maxInstanceEvents && IsEnabled();
    }

    void BeginSession(Session& session) noexcept
    {
        if (session.active || session.generation == (std::numeric_limits<uint64_t>::max)()) { return; }
        auto const generation = session.generation + 1;
        auto const contentShown = session.contentShown;
        session = {};
        session.generation = generation;
        session.contentShown = contentShown;
        session.active = true;
    }

    void ReportFirstLoad() noexcept
    {
        // One event per process at the first Loaded, so app and device counts do not depend on a session
        // ending cleanly. While no listener is enabled, later loads try again.
        static std::atomic<bool> s_reported{};
        if (s_reported.load(std::memory_order_relaxed) || !IsEnabled() || s_reported.exchange(true)) { return; }
        TraceLoggingWriteActivity(g_hTelemetryProvider, "TableView_Loaded",
            g_TelemetryProviderActivityId != GUID{} ? &g_TelemetryProviderActivityId : nullptr, nullptr, TABLEVIEW_INFO_FIELDS,
            TraceLoggingUInt32(1, "LoadedVersion"),
            TraceLoggingBoolean(TRUE, "UTCReplace_AppSessionGuid"),
            TelemetryPrivacyDataTag(PDT_ProductAndServiceUsage));
    }

    void EndSession(Session& session) noexcept
    {
        if (!session.active) { return; }
        session.active = false;
        // Capped per process; presence does not depend on it (TableView_Loaded).
        if (!ShouldSample() || s_instancesWritten.fetch_add(1, std::memory_order_relaxed) >= c_maxInstanceEvents)
        {
            return;
        }
        auto const& sampled = session.sampled;
        TraceLoggingWriteActivity(g_hTelemetryProvider, "TableView_Instance",
            g_TelemetryProviderActivityId != GUID{} ? &g_TelemetryProviderActivityId : nullptr, nullptr, TABLEVIEW_INFO_FIELDS,
            TraceLoggingUInt32(c_instanceVersion, "InstanceVersion"),
            TraceLoggingBoolean(session.generation == 1, "IsFirstLoad"),
            TraceLoggingUInt8(static_cast<uint8_t>(session.contentShown), "ContentShown"),
            // Whatever the samples saw, a feature that was used was enabled.
            TraceLoggingUInt32(sampled.enabledFeatures | session.usedFeatures, "EnabledFeatures"),
            TraceLoggingUInt32(session.usedFeatures, "UsedFeatures"),
            TraceLoggingUInt8(TableViewTip::Log2Bucket(sampled.sourceRows), "SourceRowsBucket"),
            TraceLoggingUInt8(TableViewTip::Log2Bucket(sampled.dataRows), "DataRowsBucket"),
            TraceLoggingUInt8(TableViewTip::Log2Bucket(sampled.peakRealizedRows), "PeakRealizedRowsBucket"),
            TraceLoggingUInt32(sampled.columnKinds, "ColumnKinds"),
            TraceLoggingUInt32(sampled.sourceKinds, "SourceKinds"),
            TraceLoggingBoolean(sampled.rightToLeft, "IsRightToLeft"),
            TraceLoggingBoolean(sampled.templateIncomplete, "TemplateIncomplete"),
            TraceLoggingBoolean(TRUE, "UTCReplace_AppSessionGuid"),
            TelemetryPrivacyDataTag(PDT_ProductAndServiceUsage));
    }

    void AddSample(Session& session, Sample const& sample) noexcept
    {
        if (!session.active) { return; }
        auto& sampled = session.sampled;
        sampled.enabledFeatures |= sample.enabledFeatures;
        sampled.columnKinds |= sample.columnKinds;
        sampled.sourceKinds |= sample.sourceKinds;
        sampled.sourceRows = (std::max)(sampled.sourceRows, sample.sourceRows);
        sampled.dataRows = (std::max)(sampled.dataRows, sample.dataRows);
        sampled.peakRealizedRows = (std::max)(sampled.peakRealizedRows, sample.peakRealizedRows);
        sampled.rightToLeft = sampled.rightToLeft || sample.rightToLeft;
        sampled.templateIncomplete = sampled.templateIncomplete || sample.templateIncomplete;
    }

    void MarkUsed(Session& session, uint64_t startedIn, uint32_t feature) noexcept
    {
        if (!IsCurrent(session, startedIn) || session.contentShown != ContentShown::Rows) { return; }
        session.usedFeatures |= feature;
    }

    void ReportError(Session& session, uint64_t startedIn, TableViewTip::Operation operation, TableViewTip::Stage stage,
        bool recoverable, TableViewTip::Attribution attribution, std::optional<HRESULT> error, TableViewTip::AppFaultKind kind,
        uint32_t faultCount) noexcept
    {
        auto const slot = DiagnosticSlot(operation);
        if (!slot || static_cast<uint32_t>(attribution) >= c_attributions)
        {
            OutputDebugStringW(L"TableView telemetry: invalid error operation.\n");
            return;
        }
        if (startedIn && !IsCurrent(session, startedIn)) { return; }
        // One report per operation, attribution and recoverability: an app fault cannot hide a later product error,
        // and a fault TableView recovered from cannot hide a later one that stopped an operation.
        auto const bit = *slot * c_errorBitsPerSlot + static_cast<uint32_t>(attribution) * 2 + (recoverable ? 1 : 0);
        if (session.reportedErrors[bit] ||
            !TraceLoggingProviderEnabled(g_hLoggingProvider, WINEVENT_LEVEL_VERBOSE, KEYWORD_TABLEVIEW)) { return; }
        session.reportedErrors[bit] = true;
        TraceLoggingWriteActivity(g_hLoggingProvider, "TableView_Error",
            g_TelemetryProviderActivityId != GUID{} ? &g_TelemetryProviderActivityId : nullptr, nullptr,
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE), TraceLoggingKeyword(KEYWORD_TABLEVIEW), TABLEVIEW_COMMON_FIELDS,
            TraceLoggingUInt32(static_cast<uint32_t>(operation), "Operation"),
            TraceLoggingUInt32(static_cast<uint32_t>(stage), "FailureStage"),
            TraceLoggingUInt32(static_cast<uint32_t>(attribution), "Attribution"),
            TraceLoggingHResult(error.value_or(S_OK), "HResult"),
            TraceLoggingBoolean(error.has_value(), "HResultAvailable"),
            TraceLoggingUInt32(static_cast<uint32_t>(kind), "AppFaultKind"),
            TraceLoggingUInt32(faultCount, "FaultCount"),
            TraceLoggingBoolean(recoverable, "IsRecoverable"),
            TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance));
    }

#undef TABLEVIEW_INFO_FIELDS
#undef TABLEVIEW_COMMON_FIELDS
}