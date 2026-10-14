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

        bool IsEnabled() noexcept
        {
            return TraceLoggingProviderEnabled(g_hTelemetryProvider, WINEVENT_LEVEL_INFO, measuresKeyword) != FALSE;
        }

        bool IsCurrent(Session const& session, uint64_t startedIn) noexcept
        {
            return session.active && startedIn == session.generation;
        }

        void Increment(Session& session, uint32_t& count, uint32_t bit) noexcept
        {
            if (count == (std::numeric_limits<uint32_t>::max)()) { session.overflowMask |= uint32_t{ 1 } << bit; }
            else { ++count; }
        }
    }

    void BeginSession(Session& session) noexcept
    {
        if (session.active || session.generation == (std::numeric_limits<uint64_t>::max)()) { return; }
        auto const generation = session.generation + 1;
        session = {};
        session.generation = generation;
        session.active = true;
    }

    void EndSession(Session& session) noexcept
    {
        if (!session.active) { return; }
        auto const enabled = IsEnabled();
        session.active = false;
        if (!enabled) { return; }
        TraceLoggingWriteActivity(g_hTelemetryProvider, "TableView_SessionSummary",
            g_TelemetryProviderActivityId != GUID{} ? &g_TelemetryProviderActivityId : nullptr, nullptr, TABLEVIEW_INFO_FIELDS,
            TraceLoggingUInt32(5, "SessionSummaryVersion"),
            TraceLoggingUInt32(session.counts[1], "ReplaceSourceCount"),
            TraceLoggingUInt32(session.counts[2], "SortCount"),
            TraceLoggingUInt32(session.counts[5], "FilterCount"),
            TraceLoggingUInt32(session.counts[6], "GroupConfigurationCount"),
            TraceLoggingUInt32(session.counts[8], "SelectionCount"),
            TraceLoggingUInt32(session.counts[9], "BeginEditCount"),
            TraceLoggingUInt32(session.counts[10], "CommitEditCount"),
            TraceLoggingUInt32(session.counts[11], "CancelEditCount"),
            TraceLoggingUInt32(session.deferredCount, "DeferredCount"),
            TraceLoggingUInt32(session.unattributedFaultCount, "UnattributedFaultCount"),
            TraceLoggingUInt32(session.overflowMask, "CounterOverflowMask"),
            TraceLoggingBoolean(TRUE, "UTCReplace_AppSessionGuid"),
            TelemetryPrivacyDataTag(PDT_ProductAndServiceUsage));
    }

    void CountOperation(Session& session, uint64_t startedIn, TableViewTip::Operation operation,
        std::optional<TableViewTip::IgnoreReason> ignored) noexcept
    {
        if (!IsCurrent(session, startedIn) || !TableViewTip::IsMeasuredOperation(operation)) { return; }
        if (!ignored)
        {
            auto const index = static_cast<uint32_t>(operation);
            Increment(session, session.counts[index], index);
        }
        else if (*ignored == TableViewTip::IgnoreReason::DeferredOrUnproven) { Increment(session, session.deferredCount, 14); }
        else if (*ignored == TableViewTip::IgnoreReason::UnattributedFault) { Increment(session, session.unattributedFaultCount, 15); }
    }

    void ReportError(Session& session, uint64_t startedIn, TableViewTip::Operation operation, TableViewTip::Stage stage,
        bool recoverable, TableViewTip::Attribution attribution, std::optional<HRESULT> error, TableViewTip::AppFaultKind kind) noexcept
    {
        if (static_cast<uint32_t>(operation) > static_cast<uint32_t>(TableViewTip::Operation::CancelEdit) ||
            static_cast<uint32_t>(attribution) > static_cast<uint32_t>(TableViewTip::Attribution::Application))
        {
            OutputDebugStringW(L"TableView telemetry: invalid error operation.\n");
            return;
        }
        if (startedIn && !IsCurrent(session, startedIn)) { return; }
        // One report per operation and attribution, so an app fault cannot hide a later product error.
        auto const bit = uint64_t{ 1 } << (static_cast<uint32_t>(operation) * 3 + static_cast<uint32_t>(attribution));
        if ((session.reportedErrors & bit) ||
            !TraceLoggingProviderEnabled(g_hLoggingProvider, WINEVENT_LEVEL_VERBOSE, KEYWORD_TABLEVIEW)) { return; }
        session.reportedErrors |= bit;
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
