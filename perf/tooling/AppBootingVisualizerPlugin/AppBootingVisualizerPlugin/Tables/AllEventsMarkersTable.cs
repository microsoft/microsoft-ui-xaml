using Microsoft.Performance.SDK;
using Microsoft.Performance.SDK.Processing;
using System;
using System.Collections.Generic;
using AppBootingVisualizerPlugin.Models;
using XamlTimeline;

namespace AppBootingVisualizerPlugin.Tables
{
    // Track 3 — "All Events (markers)".
    //
    // Shows every raw TelemetryEvent whose timestamp falls inside the Track 1 display
    // window (first phase/marker start -> last phase/marker end), as a Gantt
    // strip of point markers (StartTime == EndTime). Complements Track 2:
    //   - Track 2 = paired Start/Stop phases with real durations
    //   - Track 3 = full event stream, including info-only events that never pair
    //
    // Pivot default is Provider > Process > Event so the marker strip splits
    // cleanly per provider / process. Drag-select on Track 1 auto-scopes Track 3
    // (shared global time axis).
    [Table]
    public static class AllEventsMarkersTable
    {
        public const string TableName = "3. All Events (Track 3)";
        public const string TableDescription = "Every ETW event whose timestamp lies inside the Track 1 window, rendered as point markers grouped by Provider.";

        public static Guid TableGuid => Guid.Parse("{D3E4F5A6-B7C8-49D0-BAE1-3456789012CD}");

        public static TableDescriptor TableDescriptor => new TableDescriptor(
            TableGuid,
            TableName,
            TableDescription,
            category: "App Booting Visualizer Plugin",
            defaultLayout: TableLayoutStyle.GraphAndTable);

        private static readonly ColumnConfiguration ProviderColumn = new ColumnConfiguration(
            new ColumnMetadata(new Guid("{33333333-0003-0003-0003-000000000001}"), "Provider"),
            new UIHints { Width = 280 });

        private static readonly ColumnConfiguration ProcessColumn = new ColumnConfiguration(
            new ColumnMetadata(new Guid("{33333333-0003-0003-0003-000000000002}"), "Process"),
            new UIHints { Width = 200 });

        private static readonly ColumnConfiguration ProcessIdColumn = new ColumnConfiguration(
            new ColumnMetadata(new Guid("{33333333-0003-0003-0003-000000000003}"), "PID"),
            new UIHints { Width = 70 });

        private static readonly ColumnConfiguration EventNameColumn = new ColumnConfiguration(
            new ColumnMetadata(new Guid("{33333333-0003-0003-0003-000000000004}"), "Event"),
            new UIHints { Width = 240 });

        private static readonly ColumnConfiguration ThreadIdColumn = new ColumnConfiguration(
            new ColumnMetadata(new Guid("{33333333-0003-0003-0003-000000000005}"), "TID"),
            new UIHints { Width = 70 });

        private static readonly ColumnConfiguration LevelColumn = new ColumnConfiguration(
            new ColumnMetadata(new Guid("{33333333-0003-0003-0003-000000000006}"), "Level"),
            new UIHints { Width = 70 });

        private static readonly ColumnConfiguration OpcodeColumn = new ColumnConfiguration(
            new ColumnMetadata(new Guid("{33333333-0003-0003-0003-000000000007}"), "Opcode"),
            new UIHints { Width = 70 });

        private static readonly ColumnConfiguration PayloadColumn = new ColumnConfiguration(
            new ColumnMetadata(new Guid("{33333333-0003-0003-0003-000000000008}"), "Payload"),
            new UIHints { Width = 360 });

        // Bound to BOTH StartTime and EndTime roles. Zero-duration "bars" render
        // as point markers on the Gantt — one tick per event.
        private static readonly ColumnConfiguration TimeColumn = new ColumnConfiguration(
            new ColumnMetadata(new Guid("{33333333-0003-0003-0003-000000000009}"), "Time"),
            new UIHints { Width = 120 });

        public static void BuildTable(
            ITableBuilder tableBuilder,
            IReadOnlyList<TelemetryEvent> events,
            DateTime traceStart,
            DateTime traceEnd,
            IReadOnlyDictionary<int, string> processLabels)
        {
            // Filter events to the Track 1 display window. The processor narrows
            // traceStart/End to min(item.Start)..max(item.End) across all Track 1
            // items in BuildTimeline(), so this naturally bounds the markers.
            var windowed = new List<TelemetryEvent>();
            if (events != null)
            {
                foreach (var ev in events)
                {
                    if (ev.Timestamp < traceStart || ev.Timestamp > traceEnd) continue;
                    windowed.Add(ev);
                }
            }

            // Always register columns + the default config, even when empty, so
            // WPA doesn't throw "error while querying table" for an unsourced schema.
            var safeEvents = windowed.Count > 0
                ? (IReadOnlyList<TelemetryEvent>)windowed
                : new[]
                {
                    new TelemetryEvent
                    {
                        EventName = "(no events in Track 1 window — see %TEMP%\\AppBootingVisualizerPlugin.log)",
                        ProviderName = string.Empty,
                        ProcessName = string.Empty,
                        Timestamp = traceStart,
                    },
                };

            var indexProj = Projection.Index(safeEvents);

            var timeProj = indexProj.Compose(e => ToRelativeTimestamp(e.Timestamp, traceStart, traceEnd));

            tableBuilder.SetRowCount(safeEvents.Count)
                .AddColumn(ProviderColumn,     indexProj.Compose(e => e.ProviderName))
                .AddColumn(ProcessColumn,      indexProj.Compose(e => Services.ProcessLabeler.Resolve(e.ProcessName, e.ProcessId, processLabels)))
                .AddColumn(ProcessIdColumn,    indexProj.Compose(e => e.ProcessId))
                .AddColumn(EventNameColumn,    indexProj.Compose(e => e.EventName))
                .AddColumn(ThreadIdColumn,     indexProj.Compose(e => e.ThreadId))
                .AddColumn(LevelColumn,        indexProj.Compose(e => e.Level))
                .AddColumn(OpcodeColumn,       indexProj.Compose(e => e.Opcode))
                .AddColumn(PayloadColumn,      indexProj.Compose(e => e.PayloadText))
                .AddColumn(TimeColumn,         timeProj);

            // Default: marker strip grouped by Provider > Process > EventName.
            // Same zone pattern as Track 1/2 — pivots before PivotColumn, data
            // columns between, time columns after GraphColumn. StartTime and
            // EndTime roles BOTH bind to TimeColumn so each event renders as a
            // single tick (zero-duration bar).
            var markersByProvider = new TableConfiguration("All Events (markers)")
            {
                Columns = new[]
                {
                    // Zone 1: pivot keys -> fold Provider > Process > EventName.
                    ProviderColumn,
                    ProcessColumn,
                    EventNameColumn,
                    TableConfiguration.PivotColumn,
                    // Zone 2: data columns (visible, not graphed).
                    ProcessIdColumn,
                    ThreadIdColumn,
                    LevelColumn,
                    OpcodeColumn,
                    PayloadColumn,
                    TableConfiguration.GraphColumn,
                    // Zone 3: graphed column (single time, used for both roles).
                    TimeColumn,
                },
            };
            markersByProvider.AddColumnRole(ColumnRole.StartTime, TimeColumn.Metadata.Guid);
            markersByProvider.AddColumnRole(ColumnRole.EndTime,   TimeColumn.Metadata.Guid);

            // Alternate: flat table for raw event inspection (sort, search, etc.).
            var flatConfig = new TableConfiguration("All Events Table (flat)")
            {
                Columns = new[]
                {
                    TimeColumn,
                    ProviderColumn,
                    ProcessColumn,
                    ProcessIdColumn,
                    ThreadIdColumn,
                    EventNameColumn,
                    LevelColumn,
                    OpcodeColumn,
                    PayloadColumn,
                    TableConfiguration.PivotColumn,
                    TableConfiguration.GraphColumn,
                },
            };

            tableBuilder.AddTableConfiguration(markersByProvider)
                        .AddTableConfiguration(flatConfig)
                        .SetDefaultTableConfiguration(markersByProvider);
        }

        private static Timestamp ToRelativeTimestamp(DateTime when, DateTime traceStart, DateTime traceEnd)
        {
            var ns = (when - traceStart).Ticks * 100;  // 1 tick = 100 ns
            if (ns < 0) ns = 0;
            var maxNs = (traceEnd - traceStart).Ticks * 100;
            if (maxNs < 0) maxNs = 0;
            if (ns > maxNs) ns = maxNs;
            return Timestamp.FromNanoseconds(ns);
        }
    }
}
