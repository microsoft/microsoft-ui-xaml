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
        constexpr uint32_t schemaVersion = 6;
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

        // Counter slots for the measured operations, so an operation's enum value is never an index or a bit.
        // Bit n of the overflow mask is slot n; the bit after the last slot is the deferred count.
        constexpr std::optional<uint32_t> CountSlot(TableViewTip::Operation operation) noexcept
        {
            switch (operation)
            {
            case TableViewTip::Operation::ReplaceSource: return 0;
            case TableViewTip::Operation::Sort: return 1;
            case TableViewTip::Operation::Filter: return 2;
            case TableViewTip::Operation::GroupConfiguration: return 3;
            case TableViewTip::Operation::Selection: return 4;
            case TableViewTip::Operation::BeginEdit: return 5;
            case TableViewTip::Operation::CommitEdit: return 6;
            case TableViewTip::Operation::CancelEdit: return 7;
            default: return std::nullopt;
            }
        }
        constexpr uint32_t c_countSlots = static_cast<uint32_t>(std::tuple_size_v<decltype(Session::counts)>);
        constexpr uint32_t c_deferredOverflowBit = c_countSlots;
        // The diagnostic dedupe mask has one bit per attribution in each diagnostic slot.
        constexpr uint32_t c_attributionBits = static_cast<uint32_t>(TableViewTip::Attribution::Application) + 1;

        // One diagnostic per operation (setup and header refresh included) and attribution, per session.
        constexpr std::optional<uint32_t> DiagnosticSlot(TableViewTip::Operation operation) noexcept
        {
            switch (operation)
            {
            case TableViewTip::Operation::Setup: return 0;
            case TableViewTip::Operation::HeaderRefresh: return 1;
            default:
                if (auto const slot = CountSlot(operation)) { return *slot + 2; }
                return std::nullopt;
            }
        }

        // The slot tables cover exactly the measured operations and fit the session's fields.
        constexpr bool SlotsFit() noexcept
        {
            for (uint32_t value = 0; value <= static_cast<uint32_t>(TableViewTip::Operation::CancelEdit); ++value)
            {
                auto const operation = static_cast<TableViewTip::Operation>(value);
                auto const slot = CountSlot(operation);
                auto const diagnostic = DiagnosticSlot(operation);
                if (TableViewTip::IsMeasuredOperation(operation) != slot.has_value() || (slot && *slot >= c_countSlots) ||
                    (diagnostic && (*diagnostic + 1) * c_attributionBits > 32))
                {
                    return false;
                }
            }
            return c_deferredOverflowBit < 32;
        }
        static_assert(SlotsFit(), "Update IsMeasuredOperation, CountSlot, DiagnosticSlot and Session together.");

        uint32_t CountOf(Session const& session, TableViewTip::Operation operation) noexcept
        {
            return session.counts[*CountSlot(operation)];
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
        // Presence comes from TableView_Loaded, so a session with nothing to count sends nothing.
        if (session.deferredCount == 0 && session.overflowMask == 0 &&
            std::all_of(session.counts.begin(), session.counts.end(), [](uint32_t count) { return count == 0; }))
        {
            return;
        }
        if (!IsEnabled()) { return; }
        TraceLoggingWriteActivity(g_hTelemetryProvider, "TableView_SessionSummary",
            g_TelemetryProviderActivityId != GUID{} ? &g_TelemetryProviderActivityId : nullptr, nullptr, TABLEVIEW_INFO_FIELDS,
            TraceLoggingUInt32(6, "SessionSummaryVersion"),
            TraceLoggingUInt32(CountOf(session, TableViewTip::Operation::ReplaceSource), "ReplaceSourceCount"),
            TraceLoggingUInt32(CountOf(session, TableViewTip::Operation::Sort), "SortCount"),
            TraceLoggingUInt32(CountOf(session, TableViewTip::Operation::Filter), "FilterCount"),
            TraceLoggingUInt32(CountOf(session, TableViewTip::Operation::GroupConfiguration), "GroupConfigurationCount"),
            TraceLoggingUInt32(CountOf(session, TableViewTip::Operation::Selection), "SelectionCount"),
            TraceLoggingUInt32(CountOf(session, TableViewTip::Operation::BeginEdit), "BeginEditCount"),
            TraceLoggingUInt32(CountOf(session, TableViewTip::Operation::CommitEdit), "CommitEditCount"),
            TraceLoggingUInt32(CountOf(session, TableViewTip::Operation::CancelEdit), "CancelEditCount"),
            TraceLoggingUInt32(session.deferredCount, "DeferredCount"),
            TraceLoggingUInt32(session.overflowMask, "CounterOverflowMask"),
            TraceLoggingBoolean(TRUE, "UTCReplace_AppSessionGuid"),
            TelemetryPrivacyDataTag(PDT_ProductAndServiceUsage));
    }

    void CountOperation(Session& session, uint64_t startedIn, TableViewTip::Operation operation,
        std::optional<TableViewTip::IgnoreReason> ignored) noexcept
    {
        if (!IsCurrent(session, startedIn)) { return; }
        auto const slot = CountSlot(operation);
        if (!slot) { return; }
        if (!ignored)
        {
            Increment(session, session.counts[*slot], *slot);
        }
        else if (*ignored == TableViewTip::IgnoreReason::DeferredOrUnproven)
        {
            Increment(session, session.deferredCount, c_deferredOverflowBit);
        }
    }

    void ReportError(Session& session, uint64_t startedIn, TableViewTip::Operation operation, TableViewTip::Stage stage,
        bool recoverable, TableViewTip::Attribution attribution, std::optional<HRESULT> error, TableViewTip::AppFaultKind kind) noexcept
    {
        auto const slot = DiagnosticSlot(operation);
        if (!slot || static_cast<uint32_t>(attribution) > static_cast<uint32_t>(TableViewTip::Attribution::Application))
        {
            OutputDebugStringW(L"TableView telemetry: invalid error operation.\n");
            return;
        }
        if (startedIn && !IsCurrent(session, startedIn)) { return; }
        // One report per operation and attribution, so an app fault cannot hide a later product error.
        auto const bit = uint32_t{ 1 } << (*slot * c_attributionBits + static_cast<uint32_t>(attribution));
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