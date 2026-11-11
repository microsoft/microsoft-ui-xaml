// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include "TableViewTipTests.h"

// Telemetry: one TableView_SessionSummary per loaded session, plus the local-only TableView_Error
// diagnostic. TiP outcomes live in TableViewTip.h.
namespace TableViewTelemetry
{
    // Completed-operation counts for one loaded session, written once at Unloaded.
    struct Session
    {
        std::array<uint32_t, 12> counts{};
        uint64_t generation{};
        uint32_t deferredCount{};
        uint32_t overflowMask{};
        uint64_t reportedErrors{};
        bool active{};
    };

    void BeginSession(Session& session) noexcept;
    void EndSession(Session& session) noexcept;
    // An operation counts only into the session it started in, and only while that session is active.
    // An empty ignore reason means it completed.
    void CountOperation(Session& session, uint64_t startedIn, TableViewTip::Operation operation,
        std::optional<TableViewTip::IgnoreReason> ignored = {}) noexcept;
    // startedIn is the operation's session generation, or 0 when the report is not tied to one.
    void ReportError(Session& session, uint64_t startedIn, TableViewTip::Operation operation, TableViewTip::Stage stage,
        bool recoverable, TableViewTip::Attribution attribution = TableViewTip::Attribution::Unknown,
        std::optional<HRESULT> error = {}, TableViewTip::AppFaultKind kind = TableViewTip::AppFaultKind::None) noexcept;
}
