using Microsoft.Performance.SDK;
using Microsoft.Performance.SDK.Processing;
using System;
using System.Collections.Generic;
using XamlTimeline;

namespace AppBootingVisualizerPlugin.Tables
{
    [Table]
    public static class TimelineTrack1Table
    {
        public const string TableName = "1. Timeline (Track 1)";
        public const string TableDescription = "Trace-level timeline: regions detected from XamlAppLaunch.regions.xml.";

        public static Guid TableGuid => Guid.Parse("{F1E2D3C4-B5A6-4798-89A0-1234567890AB}");

        public static TableDescriptor TableDescriptor => new TableDescriptor(
            TableGuid,
            TableName,
            TableDescription,
            category: "App Booting Visualizer Plugin",
            defaultLayout: TableLayoutStyle.GraphAndTable);

        private static readonly ColumnConfiguration NameColumn = new ColumnConfiguration(
            new ColumnMetadata(new Guid("{11111111-0001-0001-0001-000000000001}"), "Name"),
            new UIHints { Width = 210 });

        private static readonly ColumnConfiguration KindColumn = new ColumnConfiguration(
            new ColumnMetadata(new Guid("{11111111-0001-0001-0001-000000000002}"), "Kind"),
            new UIHints { Width = 80 });

        private static readonly ColumnConfiguration ColorColumn = new ColumnConfiguration(
            new ColumnMetadata(new Guid("{11111111-0001-0001-0001-000000000003}"), "Color"),
            new UIHints { Width = 80 });

        private static readonly ColumnConfiguration StartTimeColumn = new ColumnConfiguration(
            new ColumnMetadata(new Guid("{11111111-0001-0001-0001-000000000004}"), "Start Time"),
            new UIHints { Width = 120 });

        private static readonly ColumnConfiguration EndTimeColumn = new ColumnConfiguration(
            new ColumnMetadata(new Guid("{11111111-0001-0001-0001-000000000005}"), "End Time"),
            new UIHints { Width = 120 });

        private static readonly ColumnConfiguration DurationColumn = new ColumnConfiguration(
            new ColumnMetadata(new Guid("{11111111-0001-0001-0001-000000000006}"), "Duration"),
            new UIHints { Width = 90 });

        private static readonly ColumnConfiguration StartEventColumn = new ColumnConfiguration(
            new ColumnMetadata(new Guid("{11111111-0001-0001-0001-000000000007}"), "Start Event"),
            new UIHints { Width = 260 });

        private static readonly ColumnConfiguration EndEventColumn = new ColumnConfiguration(
            new ColumnMetadata(new Guid("{11111111-0001-0001-0001-000000000008}"), "End Event"),
            new UIHints { Width = 260 });

        private static readonly ColumnConfiguration ProcessColumn = new ColumnConfiguration(
            new ColumnMetadata(new Guid("{11111111-0001-0001-0001-000000000009}"), "Process"),
            new UIHints { Width = 180 });

        private static readonly ColumnConfiguration ProcessIdColumn = new ColumnConfiguration(
            new ColumnMetadata(new Guid("{11111111-0001-0001-0001-00000000000A}"), "PID"),
            new UIHints { Width = 70 });

        public static void BuildTable(
            ITableBuilder tableBuilder,
            IReadOnlyList<TimelineItem> items,
            DateTime traceStart,
            DateTime traceEnd,
            IReadOnlyDictionary<int, string> processLabels)
        {
            // Always register columns and the default table configuration, even when no
            // items were detected — otherwise WPA throws "error while querying table"
            // because the table has no schema.
            var safeItems = (items != null && items.Count > 0)
                ? items
                : (IReadOnlyList<TimelineItem>)new[]
                {
                    new TimelineItem
                    {
                        Name = "(no timeline regions detected — see %TEMP%\\AppBootingVisualizerPlugin.log)",
                        Kind = RegionKind.Phase,
                        Color = "#888888",
                        StartEvent = new TelemetryEvent { Timestamp = traceStart },
                        EndEvent = new TelemetryEvent { Timestamp = traceEnd > traceStart ? traceEnd : traceStart.AddMilliseconds(1) },
                    }
                };

            var indexProj = Projection.Index(safeItems);

            var startProj = indexProj.Compose(item => ToRelativeTimestamp(item.Start, traceStart));
            var endProj = indexProj.Compose(item =>
            {
                var end = item.End.HasValue ? item.End.Value : item.Start;
                return ToRelativeTimestamp(end, traceStart);
            });
            var durationProj = indexProj.Compose(item =>
            {
                if (!item.End.HasValue)
                {
                    return TimestampDelta.Zero;
                }
                var ticks = (item.End.Value - item.Start).Ticks;
                return new TimestampDelta(ticks * 100);  // 1 tick = 100 ns
            });

            tableBuilder.SetRowCount(safeItems.Count)
                .AddColumn(NameColumn, indexProj.Compose(item => item.Name))
                .AddColumn(KindColumn, indexProj.Compose(item => item.Kind.ToString()))
                .AddColumn(ColorColumn, indexProj.Compose(item => item.Color))
                .AddColumn(StartTimeColumn, startProj)
                .AddColumn(EndTimeColumn, endProj)
                .AddColumn(DurationColumn, durationProj)
                .AddColumn(StartEventColumn, indexProj.Compose(item =>
                    item.StartEvent == null ? string.Empty : item.StartEvent.ProviderName + "/" + item.StartEvent.EventName))
                .AddColumn(EndEventColumn, indexProj.Compose(item =>
                    item.EndEvent == null ? string.Empty : item.EndEvent.ProviderName + "/" + item.EndEvent.EventName))
                .AddColumn(ProcessColumn, indexProj.Compose(item => Services.ProcessLabeler.Resolve(item.ProcessName, item.ProcessId, processLabels)))
                .AddColumn(ProcessIdColumn, indexProj.Compose(item => item.ProcessId));

            // Default view: Track 1 Gantt — each row is a colored bar from StartTime
            // to EndTime. Mirrors microsoft/Microsoft-Performance-Tools-Linux-Android's
            // PerfettoCpuSchedTable (the canonical WPA Gantt) exactly:
            //   - Zone 1 (before PivotColumn): pivot keys
            //   - Zone 2 (between Pivot and Graph): always-visible data columns
            //   - Zone 3 (after GraphColumn): graphed columns (StartTime, EndTime)
            //   - All three time roles (StartTime, EndTime, Duration) MUST be set —
            //     Duration role is informational only; bars are positioned by Start/End.
            //   - AddColumnRole uses Metadata.Guid (per the working SDK examples).
            var ganttConfig = new TableConfiguration("Timeline (Track 1)")
            {
                Columns = new[]
                {
                    // Zone 1: pivot keys -> one Gantt row per (Process, Name)
                    ProcessColumn,
                    NameColumn,
                    TableConfiguration.PivotColumn,
                    // Zone 2: data columns (always visible, not graphed)
                    ProcessIdColumn,
                    KindColumn,
                    StartEventColumn,
                    EndEventColumn,
                    ColorColumn,
                    DurationColumn,
                    TableConfiguration.GraphColumn,
                    // Zone 3: graphed columns
                    StartTimeColumn,
                    EndTimeColumn,
                },
            };
            ganttConfig.AddColumnRole(ColumnRole.StartTime, StartTimeColumn.Metadata.Guid);
            ganttConfig.AddColumnRole(ColumnRole.EndTime,   EndTimeColumn.Metadata.Guid);
            ganttConfig.AddColumnRole(ColumnRole.Duration,  DurationColumn.Metadata.Guid);

            // Alternate view: flat table — every column on the table side, no graph.
            // Useful for sorting / filtering / inspecting all rows at once.
            var flatConfig = new TableConfiguration("Timeline Table (flat)")
            {
                Columns = new[]
                {
                    ProcessColumn,
                    ProcessIdColumn,
                    NameColumn,
                    KindColumn,
                    StartEventColumn,
                    EndEventColumn,
                    StartTimeColumn,
                    EndTimeColumn,
                    DurationColumn,
                    ColorColumn,
                    TableConfiguration.PivotColumn,
                    TableConfiguration.GraphColumn,
                },
            };

            tableBuilder.AddTableConfiguration(ganttConfig)
                        .AddTableConfiguration(flatConfig)
                        .SetDefaultTableConfiguration(ganttConfig);
        }

        private static Timestamp ToRelativeTimestamp(DateTime when, DateTime traceStart)
        {
            var delta = when - traceStart;
            var ns = delta.Ticks * 100;  // 1 tick = 100 ns
            if (ns < 0) ns = 0;
            return Timestamp.FromNanoseconds(ns);
        }
    }
}
