using Microsoft.Performance.SDK;
using Microsoft.Performance.SDK.Processing;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using AppBootingVisualizerPlugin.Models;
using XamlTimeline;

namespace AppBootingVisualizerPlugin.Tables
{
    // Track 2 — "Events by Provider".
    //
    // Mirrors the WinUI reference app's "click a Track 1 region -> see all events
    // grouped by provider" panel. Each row is a Start/Stop pair (one Opcode==1
    // matched with one Opcode==2 on the same ThreadId + base EventName, produced
    // by EventPairer) — so it renders as a real Gantt bar with a real duration,
    // not an instantaneous marker.
    //
    // Rows are pivoted Provider > Process > EventName, mirroring the WinUI app's
    // per-provider boxes with per-event sub-groupings. The shared global time
    // axis is already clamped to the Track 1 window, so drag-select on Track 1
    // auto-scopes Track 2.
    [Table]
    public static class EventsByProviderTable
    {
        public const string TableName = "2. Events by Provider (Track 2)";
        public const string TableDescription = "ETW Start/Stop pairs grouped by Provider. Each row is a phase with a real duration. Drag-select on Track 1 to scope the chart.";

        public static Guid TableGuid => Guid.Parse("{C2D3E4F5-A6B7-48C9-9AD0-2345678901BC}");

        public static TableDescriptor TableDescriptor => new TableDescriptor(
            TableGuid,
            TableName,
            TableDescription,
            category: "App Booting Visualizer Plugin",
            defaultLayout: TableLayoutStyle.GraphAndTable);

        private static readonly ColumnConfiguration ProviderColumn = new ColumnConfiguration(
            new ColumnMetadata(new Guid("{22222222-0002-0002-0002-000000000001}"), "Provider"),
            new UIHints { Width = 280 });

        private static readonly ColumnConfiguration ProcessColumn = new ColumnConfiguration(
            new ColumnMetadata(new Guid("{22222222-0002-0002-0002-000000000002}"), "Process"),
            new UIHints { Width = 180 });

        private static readonly ColumnConfiguration ProcessIdColumn = new ColumnConfiguration(
            new ColumnMetadata(new Guid("{22222222-0002-0002-0002-000000000003}"), "PID"),
            new UIHints { Width = 70 });

        private static readonly ColumnConfiguration EventNameColumn = new ColumnConfiguration(
            new ColumnMetadata(new Guid("{22222222-0002-0002-0002-000000000004}"), "Event"),
            new UIHints { Width = 240 });

        private static readonly ColumnConfiguration ThreadIdColumn = new ColumnConfiguration(
            new ColumnMetadata(new Guid("{22222222-0002-0002-0002-000000000005}"), "TID"),
            new UIHints { Width = 70 });

        private static readonly ColumnConfiguration LevelColumn = new ColumnConfiguration(
            new ColumnMetadata(new Guid("{22222222-0002-0002-0002-000000000006}"), "Level"),
            new UIHints { Width = 70 });

        private static readonly ColumnConfiguration StartPayloadColumn = new ColumnConfiguration(
            new ColumnMetadata(new Guid("{22222222-0002-0002-0002-000000000007}"), "Start Payload"),
            new UIHints { Width = 280 });

        private static readonly ColumnConfiguration StopPayloadColumn = new ColumnConfiguration(
            new ColumnMetadata(new Guid("{22222222-0002-0002-0002-000000000008}"), "Stop Payload"),
            new UIHints { Width = 280 });

        private static readonly ColumnConfiguration StartTimeColumn = new ColumnConfiguration(
            new ColumnMetadata(new Guid("{22222222-0002-0002-0002-000000000009}"), "Start Time"),
            new UIHints { Width = 120 });

        private static readonly ColumnConfiguration EndTimeColumn = new ColumnConfiguration(
            new ColumnMetadata(new Guid("{22222222-0002-0002-0002-00000000000A}"), "End Time"),
            new UIHints { Width = 120 });

        private static readonly ColumnConfiguration DurationColumn = new ColumnConfiguration(
            new ColumnMetadata(new Guid("{22222222-0002-0002-0002-00000000000B}"), "Duration"),
            new UIHints { Width = 100 });

        public static void BuildTable(
            ITableBuilder tableBuilder,
            IReadOnlyList<PairedEvent> pairs,
            DateTime traceStart,
            DateTime traceEnd,
            IReadOnlyDictionary<int, string> processLabels,
            IReadOnlyList<PhaseDefinition> phaseDefinitions = null,
            IReadOnlyList<TimelineItem> trackOneItems = null)
        {
            // Always register columns + the default config, even when empty, so WPA
            // doesn't throw "error while querying table" for an unsourced schema.
            var safePairs = (pairs != null && pairs.Count > 0)
                ? pairs
                : (IReadOnlyList<PairedEvent>)new[]
                {
                    new PairedEvent
                    {
                        EventName = "(no Start/Stop pairs found — see %TEMP%\\AppBootingVisualizerPlugin.log)",
                        ProviderName = string.Empty,
                        ProcessName = string.Empty,
                        Start = traceStart,
                        Stop = traceEnd > traceStart ? traceEnd : traceStart.AddMilliseconds(1),
                    },
                };

            var indexProj = Projection.Index(safePairs);

            var startProj = indexProj.Compose(p => ToRelativeTimestamp(p.Start, traceStart, traceEnd));
            var endProj = indexProj.Compose(p => ToRelativeTimestamp(p.Stop, traceStart, traceEnd));
            var durationProj = indexProj.Compose(p =>
            {
                var ticks = (p.Stop - p.Start).Ticks;
                if (ticks < 0) ticks = 0;
                return new TimestampDelta(ticks * 100);  // 1 tick = 100 ns
            });

            tableBuilder.SetRowCount(safePairs.Count)
                .AddColumn(ProviderColumn,     indexProj.Compose(p => p.ProviderName))
                .AddColumn(ProcessColumn,      indexProj.Compose(p => Services.ProcessLabeler.Resolve(p.ProcessName, p.ProcessId, processLabels)))
                .AddColumn(ProcessIdColumn,    indexProj.Compose(p => p.ProcessId))
                .AddColumn(EventNameColumn,    indexProj.Compose(p => p.EventName))
                .AddColumn(ThreadIdColumn,     indexProj.Compose(p => p.ThreadId))
                .AddColumn(LevelColumn,        indexProj.Compose(p => p.Level))
                .AddColumn(StartPayloadColumn, indexProj.Compose(p => p.StartPayload))
                .AddColumn(StopPayloadColumn,  indexProj.Compose(p => p.StopPayload))
                .AddColumn(StartTimeColumn,    startProj)
                .AddColumn(EndTimeColumn,      endProj)
                .AddColumn(DurationColumn,     durationProj);

            // Default: Gantt grouped by Provider > Process > EventName. Same zone
            // pattern as Track 1 — pivot keys first, data columns between Pivot
            // and Graph, time columns after Graph, all three time roles bound.
            var ganttByProvider = new TableConfiguration("Events by Provider")
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
                    StartPayloadColumn,
                    StopPayloadColumn,
                    DurationColumn,
                    TableConfiguration.GraphColumn,
                    // Zone 3: graphed columns.
                    StartTimeColumn,
                    EndTimeColumn,
                },
            };
            ganttByProvider.AddColumnRole(ColumnRole.StartTime, StartTimeColumn.Metadata.Guid);
            ganttByProvider.AddColumnRole(ColumnRole.EndTime,   EndTimeColumn.Metadata.Guid);
            ganttByProvider.AddColumnRole(ColumnRole.Duration,  DurationColumn.Metadata.Guid);

            // Alternate: flat table for sorting / filtering / inspection.
            var flatConfig = new TableConfiguration("Events Table (flat)")
            {
                Columns = new[]
                {
                    ProviderColumn,
                    ProcessColumn,
                    ProcessIdColumn,
                    EventNameColumn,
                    ThreadIdColumn,
                    LevelColumn,
                    StartTimeColumn,
                    EndTimeColumn,
                    DurationColumn,
                    StartPayloadColumn,
                    StopPayloadColumn,
                    TableConfiguration.PivotColumn,
                    TableConfiguration.GraphColumn,
                },
            };

            tableBuilder.AddTableConfiguration(ganttByProvider)
                        .AddTableConfiguration(flatConfig)
                        .SetDefaultTableConfiguration(ganttByProvider);

            // Per-region presets. For each Track 1 region definition, add a
            // TableConfiguration named "Region: <name>" that pre-filters Track 2
            // down to the providers that region cares about (from its
            // SpecialProviders allowlist in XamlAppLaunch.regions.xml). For
            // ProcessDependent regions we also restrict to the PIDs that
            // actually fired the region in this trace, so the user sees only
            // the launching app's events instead of every process.
            //
            // Pivot/zone layout matches the default Gantt config so switching
            // presets feels seamless.
            if (phaseDefinitions != null)
            {
                foreach (var region in phaseDefinitions)
                {
                    if (region == null || string.IsNullOrEmpty(region.Name))
                    {
                        continue;
                    }

                    var query = BuildRegionFilterQuery(region, trackOneItems, processLabels, safePairs);
                    if (string.IsNullOrEmpty(query))
                    {
                        // Nothing to filter on for this region; skip rather than
                        // add an empty preset that would show every row.
                        continue;
                    }

                    var regionConfig = new TableConfiguration("Region: " + region.Name)
                    {
                        Columns = ganttByProvider.Columns,
                        InitialFilterQuery = query,
                        InitialFilterShouldKeep = true,
                    };
                    regionConfig.AddColumnRole(ColumnRole.StartTime, StartTimeColumn.Metadata.Guid);
                    regionConfig.AddColumnRole(ColumnRole.EndTime,   EndTimeColumn.Metadata.Guid);
                    regionConfig.AddColumnRole(ColumnRole.Duration,  DurationColumn.Metadata.Guid);

                    tableBuilder.AddTableConfiguration(regionConfig);
                }
            }
        }

        // Builds the WPA InitialFilterQuery for a region's preset. Format:
        //   ([Provider]:"X" OR [Provider]:"Y") AND ([Process]:"foo (12)" OR ...)
        // Providers come from two sources, unioned:
        //   (a) <SpecialProviders> in the region XML (explicit allowlist).
        //   (b) Any provider whose cumulative paired-event duration inside the
        //       region's window is >= region.ThresholdPercentage of the parent
        //       window (default 5%). Microsoft-Windows-XAML is excluded from
        //       this threshold pass for non-ProcessDependent regions — see
        //       RegionProviderSummarizer for details.
        // Process clause is only emitted for ProcessDependent regions and uses
        // the labels from ProcessLabeler so it matches the values rendered in
        // the Process column.
        private static string BuildRegionFilterQuery(
            PhaseDefinition region,
            IReadOnlyList<TimelineItem> trackOneItems,
            IReadOnlyDictionary<int, string> processLabels,
            IReadOnlyList<PairedEvent> pairs)
        {
            var clauses = new List<string>();

            // Collect this region's detected items once — used by both the
            // threshold computation and the process clause.
            var matchingItems = new List<TimelineItem>();
            if (trackOneItems != null)
            {
                foreach (var item in trackOneItems)
                {
                    if (item != null
                        && string.Equals(item.Name, region.Name, StringComparison.OrdinalIgnoreCase))
                    {
                        matchingItems.Add(item);
                    }
                }
            }

            var providerSet = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            if (region.SpecialProviders != null)
            {
                foreach (var sp in region.SpecialProviders)
                {
                    if (!string.IsNullOrWhiteSpace(sp))
                    {
                        providerSet.Add(sp);
                    }
                }
            }

            var thresholdProviders = Services.RegionProviderSummarizer
                .ComputeThresholdProviders(region, matchingItems, pairs);
            foreach (var p in thresholdProviders)
            {
                providerSet.Add(p);
            }

            if (providerSet.Count > 0)
            {
                var providerTerms = providerSet
                    .OrderBy(p => p, StringComparer.OrdinalIgnoreCase)
                    .Select(p => "[Provider]:\"" + EscapeQueryLiteral(p) + "\"");
                var providerClause = string.Join(" OR ", providerTerms);
                clauses.Add("(" + providerClause + ")");
            }

            if (region.ProcessDependent && matchingItems.Count > 0)
            {
                var pids = new HashSet<int>();
                foreach (var item in matchingItems)
                {
                    if (item.ProcessId > 0)
                    {
                        pids.Add(item.ProcessId);
                    }
                }

                if (pids.Count > 0)
                {
                    var processTerms = pids
                        .Select(pid => ProcessLabelFor(pid, processLabels))
                        .Where(lbl => !string.IsNullOrEmpty(lbl))
                        .Select(lbl => "[Process]:\"" + EscapeQueryLiteral(lbl) + "\"");
                    var processClause = string.Join(" OR ", processTerms);
                    if (!string.IsNullOrEmpty(processClause))
                    {
                        clauses.Add("(" + processClause + ")");
                    }
                }
            }

            return string.Join(" AND ", clauses);
        }

        private static string ProcessLabelFor(int pid, IReadOnlyDictionary<int, string> labels)
        {
            if (labels != null && labels.TryGetValue(pid, out var label) && !string.IsNullOrEmpty(label))
            {
                return label;
            }
            return "(" + pid + ")";
        }

        private static string EscapeQueryLiteral(string value)
        {
            if (string.IsNullOrEmpty(value))
            {
                return string.Empty;
            }
            // WPA's filter query parser uses backslash-escaped double quotes inside
            // quoted literals. Backslashes themselves must also be doubled.
            return value.Replace("\\", "\\\\").Replace("\"", "\\\"");
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
