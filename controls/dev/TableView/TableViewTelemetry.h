// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cstdint>
#include <optional>
#include "TableViewTipTypes.h"

// Telemetry: a once-per-process TableView_Loaded, one TableView_Instance per loaded session, and the local-only
// TableView_Error diagnostic. TiP outcomes live in TableViewTip.h.
namespace TableViewTelemetry
{
    // TableView_Instance's EnabledFeatures and UsedFeatures bits.
    namespace Feature
    {
        constexpr uint32_t Sorting = 1u << 0;
        constexpr uint32_t Editing = 1u << 1;
        constexpr uint32_t Filtering = 1u << 2;
        constexpr uint32_t Grouping = 1u << 3;
        constexpr uint32_t Hierarchy = 1u << 4;
        constexpr uint32_t LiveShaping = 1u << 5;
    }
    // TableView_Instance's ColumnKinds bits. A column whose class is not one of ours is the app's.
    namespace ColumnKind
    {
        constexpr uint32_t Text = 1u << 0;
        constexpr uint32_t Template = 1u << 1;
        constexpr uint32_t AppDerived = 1u << 2;
    }
    // TableView_Instance's SourceKinds bits: a plain collection TableView projected, or the app's own TableViewSource.
    namespace SourceKind
    {
        constexpr uint32_t Projected = 1u << 0;
        constexpr uint32_t AppSupplied = 1u << 1;
    }
    // What the instance has shown across its loads: a row or group header (Rows, which Empty never replaces),
    // or only a source with no data rows (Empty).
    enum class ContentShown : uint8_t { None = 0, Rows = 1, Empty = 2 };

    // What one sample point (Loaded, Unloaded) read. A session keeps every sample's bits and the largest counts.
    struct Sample
    {
        uint32_t enabledFeatures{};
        uint32_t columnKinds{};
        uint32_t sourceKinds{};
        // TableViewSource::DataRowCount, and the containers the rows repeater has created. It creates one only when
        // none is free, so this is the most rows realized at once, plus the most group headers (they pool separately).
        uint32_t dataRows{};
        uint32_t peakRealizedRows{};
        bool rightToLeft{};
        bool templateIncomplete{};
    };

    // One loaded session, written once as TableView_Instance at Unloaded or destruction.
    struct Session
    {
        uint64_t generation{};
        // TableView_Error dedupe: one bit per operation, attribution and recoverability.
        uint64_t reportedErrors{};
        Sample sampled;
        uint32_t usedFeatures{};
        // Per instance rather than per session: BeginSession keeps it.
        ContentShown contentShown{};
        bool active{};
    };

    // Whether a sample can still be written: a listener collects TableView's telemetry and the process cap
    // is not reached. Otherwise TableView skips sampling.
    bool ShouldSample() noexcept;
    void BeginSession(Session& session) noexcept;
    // Writes TableView_Instance, capped per process, and ends the session.
    void EndSession(Session& session) noexcept;
    // Presence: once per process, at the first Loaded.
    void ReportFirstLoad() noexcept;
    void AddSample(Session& session, Sample const& sample) noexcept;
    // A one-shot latch: TableView calls it for every element it realizes, so it is a single compare once Rows is seen.
    inline void NoteContent(Session& session, ContentShown content) noexcept
    {
        if (session.contentShown != ContentShown::Rows && content != ContentShown::None) { session.contentShown = content; }
    }
    // Use counts only into the session it happened in, and only once the instance has shown rows.
    void MarkUsed(Session& session, uint64_t startedIn, uint32_t feature) noexcept;

    // The feature a measured operation exercises, or 0. Editing is used by a commit, not by opening an editor.
    constexpr uint32_t FeatureOf(TableViewTip::Operation operation) noexcept
    {
        switch (operation)
        {
        case TableViewTip::Operation::Sort: return Feature::Sorting;
        case TableViewTip::Operation::Filter: return Feature::Filtering;
        case TableViewTip::Operation::CommitEdit: return Feature::Editing;
        default: return 0;
        }
    }

    // startedIn is the operation's session generation, or 0 when the report is not tied to one.
    void ReportError(Session& session, uint64_t startedIn, TableViewTip::Operation operation, TableViewTip::Stage stage,
        bool recoverable, TableViewTip::Attribution attribution = TableViewTip::Attribution::Unknown,
        std::optional<HRESULT> error = {}, TableViewTip::AppFaultKind kind = TableViewTip::AppFaultKind::None) noexcept;
}