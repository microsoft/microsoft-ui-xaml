using Microsoft.Performance.SDK.Processing;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using AppBootingVisualizerPlugin.Models;
using XamlTimeline;
using AppBootingVisualizerPlugin.Services;
using AppBootingVisualizerPlugin.Tables;

namespace AppBootingVisualizerPlugin
{
    public sealed class AppBootingVisualizerPluginProcessor : ICustomDataProcessor
    {
        // The Microsoft-Windows-XAML provider GUID. We auto-pick the process with the
        // most events from this provider as the "selected process" for ${ProcessName}
        // substitution in payload filters and PID filtering for ProcessDependent regions.
        private static readonly Guid XamlProviderGuid = Guid.Parse("{531a35ab-63ce-4bcf-aa98-f88c7a89e455}");

        private readonly IEnumerable<IDataSource> dataSources;
        private readonly HashSet<Guid> enabledTables = new HashSet<Guid>();
        private readonly List<TelemetryEvent> events = new List<TelemetryEvent>();
        private IReadOnlyList<TimelineItem> trackOneItems = Array.Empty<TimelineItem>();
        private IReadOnlyList<PairedEvent> pairedEvents = Array.Empty<PairedEvent>();
        private IReadOnlyDictionary<int, string> processLabels = new Dictionary<int, string>();
        private IReadOnlyList<PhaseDefinition> phaseDefinitions = Array.Empty<PhaseDefinition>();
        private DateTime traceStartTime = DateTime.UtcNow;
        private DateTime traceEndTime = DateTime.UtcNow;

        public AppBootingVisualizerPluginProcessor(IEnumerable<IDataSource> dataSources)
        {
            this.dataSources = dataSources;
        }

        public IEnumerable<TableDescriptor> GetMetadataTables()
        {
            return Array.Empty<TableDescriptor>();
        }

        public void BuildTable(TableDescriptor tableDescriptor, ITableBuilder tableBuilder)
        {
            if (tableDescriptor.Guid == TimelineTrack1Table.TableGuid)
            {
                TimelineTrack1Table.BuildTable(tableBuilder, this.trackOneItems, this.traceStartTime, this.traceEndTime, this.processLabels);
            }
            else if (tableDescriptor.Guid == EventsByProviderTable.TableGuid)
            {
                // Track 2: Start/Stop pairs produced once in ProcessAsync by
                // EventPairer (mirrors WinUI SpecialChildrenBuilder's stack logic).
                // Shares the Track 1 display window so the Gantts line up on the
                // same x-axis (drag-select on Track 1 auto-scopes Track 2).
                //
                // Region presets: phaseDefinitions + trackOneItems are passed so the
                // table can add one TableConfiguration per Track 1 region, pre-
                // filtered to that region's SpecialProviders (and, for
                // ProcessDependent regions, to the PIDs that actually fired the
                // region in this trace).
                EventsByProviderTable.BuildTable(
                    tableBuilder,
                    this.pairedEvents,
                    this.traceStartTime,
                    this.traceEndTime,
                    this.processLabels,
                    this.phaseDefinitions,
                    this.trackOneItems);
            }
            else if (tableDescriptor.Guid == AllEventsMarkersTable.TableGuid)
            {
                // Track 3: every raw event whose timestamp falls inside the Track 1
                // window, rendered as point markers (StartTime == EndTime). Complements
                // Track 2 by including unpaired / info-only events that never form
                // Start/Stop pairs.
                AllEventsMarkersTable.BuildTable(tableBuilder, this.events, this.traceStartTime, this.traceEndTime, this.processLabels);
            }
        }

        public DataSourceInfo GetDataSourceInfo()
        {
            // DataSourceInfo REQUIRES the wall-clock time to be UTC (it throws
            // ArgumentException otherwise). Our ETL timestamps come back as local
            // time, so convert here. Without this, GetDataSourceInfo throws and WPA
            // falls back to a default axis range — the chart shows the bars in the
            // wrong x-range (e.g. 0..8.5s instead of 0..1.29s).
            var startUtc = this.traceStartTime.Kind == DateTimeKind.Utc
                ? this.traceStartTime
                : this.traceStartTime.ToUniversalTime();
            var ns = (this.traceEndTime - this.traceStartTime).Ticks * 100;
            if (ns < 0) ns = 0;
            Diag.Log("GetDataSourceInfo: startUtc=" + startUtc.ToString("o") + " ns=" + ns);
            return new DataSourceInfo(0, ns, startUtc);
        }

        public Task ProcessAsync(IProgress<int> progress, CancellationToken cancellationToken)
        {
            this.events.Clear();
            this.trackOneItems = Array.Empty<TimelineItem>();
            this.pairedEvents = Array.Empty<PairedEvent>();
            this.processLabels = new Dictionary<int, string>();
            Diag.Reset();

            var etlFiles = this.dataSources
                .OfType<FileDataSource>()
                .Where(ds => Path.GetExtension(ds.FullPath).Equals(".etl", StringComparison.OrdinalIgnoreCase))
                .ToList();

            if (etlFiles.Count == 0)
            {
                Diag.Log("No .etl data sources found.");
                return Task.CompletedTask;
            }

            foreach (var etlFile in etlFiles)
            {
                cancellationToken.ThrowIfCancellationRequested();
                Diag.Log("Parsing ETL: " + etlFile.FullPath);
                var loader = new EtlEventLoader();
                var parsed = loader.Load(etlFile.FullPath);
                this.events.AddRange(parsed);
            }

            Diag.Log("Total events parsed: " + this.events.Count);

            if (this.events.Count > 0)
            {
                this.traceStartTime = this.events[0].Timestamp;
                this.traceEndTime = this.events[this.events.Count - 1].Timestamp;
                Diag.Log("Trace span: " + this.traceStartTime.ToString("o") + " -> " + this.traceEndTime.ToString("o"));
            }

            BuildTimeline();

            // Track 2 source data: pair Opcode==1/Opcode==2 events on
            // (baseEventName, ThreadId), mirroring WinUI SpecialChildrenBuilder.
            // Done once here so subsequent BuildTable calls are cheap.
            var pairer = new EventPairer();
            this.pairedEvents = pairer.Pair(this.events);

            // Process label map: "<ImageName> (<PID>)" by default, with the svchost
            // suffix from Microsoft-Windows-Kernel-Process/ProcessStart appended for
            // service-host PIDs. Built once here so each BuildTable call just does
            // dictionary lookups.
            this.processLabels = ProcessLabeler.Build(this.events);
            Diag.Log("Process labels built for " + this.processLabels.Count + " PIDs.");

            progress.Report(100);
            return Task.CompletedTask;
        }

        private void BuildTimeline()
        {
            if (this.events.Count == 0)
            {
                this.trackOneItems = Array.Empty<TimelineItem>();
                Diag.Log("BuildTimeline: no events; skipping.");
                return;
            }

            IReadOnlyList<PhaseDefinition> definitions;
            try
            {
                var loader = new RegionsLoader();
                definitions = loader.LoadEmbeddedDefault();
                Diag.Log("Loaded " + definitions.Count + " region definitions from shared embedded resource.");
            }
            catch (Exception ex)
            {
                Diag.Log("Failed to load embedded regions XML: " + ex.GetType().Name + " " + ex.Message);
                this.trackOneItems = Array.Empty<TimelineItem>();
                return;
            }

            // Multi-process Track 1: produce one set of Track 1 items per candidate
            // process and concatenate them. The Gantt's Process column is a pivot key,
            // so WPA renders one expandable row group per process.
            //
            // Uses the plugin-local MultiProcessTrackOneBuilder, which builds all
            // processes in a single set of passes over the events (O(events × regions))
            // instead of one full per-process scan (O(processes × events × regions)).
            // It is verified to produce the same items as calling the shared
            // TimelineBuilder.BuildTrackOne once per process.
            var candidates = CollectCandidateProcesses(this.events);
            Diag.Log("Candidate processes for Track 1: " + candidates.Count);

            var builder = new MultiProcessTrackOneBuilder();
            var allItems = builder.Build(this.events, definitions, candidates);
            Diag.Log("Track 1 items built across " + candidates.Count + " processes: " + allItems.Count);

            allItems.Sort((a, b) => a.Start.CompareTo(b.Start));
            this.trackOneItems = allItems;
            this.phaseDefinitions = definitions;
            Diag.Log("Track 1 items detected (across all processes): " + allItems.Count);

            // Shrink the displayed trace bounds to just the Track 1 window so the Gantt
            // graph isn't dominated by empty time before/after the launches. Span = min
            // start -> max end across ALL processes' items.
            if (allItems.Count > 0)
            {
                DateTime minStart = DateTime.MaxValue, maxEnd = DateTime.MinValue;
                foreach (var item in allItems)
                {
                    if (item.Start < minStart) minStart = item.Start;
                    var endWhen = item.End.HasValue ? item.End.Value : item.Start;
                    if (endWhen > maxEnd) maxEnd = endWhen;
                }

                if (maxEnd <= minStart)
                {
                    maxEnd = minStart.AddMilliseconds(1);
                }

                this.traceStartTime = minStart;
                this.traceEndTime = maxEnd;
                Diag.Log("Track 1 display window: " + minStart.ToString("o") + " -> " + maxEnd.ToString("o") + " (" + (maxEnd - minStart).TotalMilliseconds.ToString("F1") + " ms)");
            }
        }

        // Build the candidate process list for multi-process Track 1 expansion.
        // Lists EVERY process seen in the trace (kernel pseudo-PIDs 0/4 excluded),
        // sorted by event count descending so busy/launched processes appear first
        // in the Gantt's process groups.
        private static List<(string Name, int Id)> CollectCandidateProcesses(IReadOnlyList<TelemetryEvent> events)
        {
            var allCounts = new Dictionary<int, int>();
            var nameById = new Dictionary<int, string>();

            foreach (var ev in events)
            {
                if (ev.ProcessId == 0 || ev.ProcessId == 4) continue;  // Idle / System
                allCounts.TryGetValue(ev.ProcessId, out var c);
                allCounts[ev.ProcessId] = c + 1;
                if (!nameById.ContainsKey(ev.ProcessId) && !string.IsNullOrEmpty(ev.ProcessName))
                {
                    nameById[ev.ProcessId] = ev.ProcessName;
                }
            }

            if (allCounts.Count == 0)
            {
                return new List<(string Name, int Id)> { AutoSelectProcess(events) };
            }

            var result = new List<(string Name, int Id)>(allCounts.Count);
            foreach (var kv in allCounts.OrderByDescending(k => k.Value))
            {
                nameById.TryGetValue(kv.Key, out var n);
                result.Add((n ?? string.Empty, kv.Key));
            }
            return result;
        }

        // Pick the process with the most Microsoft-Windows-XAML provider events as
        // THE launched app. This is the simplest and most reliable signal: a freshly
        // launched XAML app dominates XAML event volume in its trace. If no process
        // has any XAML events, fall back to the busiest non-kernel process.
        private static (string Name, int Id) AutoSelectProcess(IReadOnlyList<TelemetryEvent> events)
        {
            var xamlCounts = new Dictionary<int, int>();
            var allCounts = new Dictionary<int, int>();
            var nameById = new Dictionary<int, string>();

            foreach (var ev in events)
            {
                allCounts.TryGetValue(ev.ProcessId, out var ac);
                allCounts[ev.ProcessId] = ac + 1;

                if (!nameById.ContainsKey(ev.ProcessId) && !string.IsNullOrEmpty(ev.ProcessName))
                {
                    nameById[ev.ProcessId] = ev.ProcessName;
                }

                if (ev.ProviderGuid == XamlProviderGuid)
                {
                    xamlCounts.TryGetValue(ev.ProcessId, out var xc);
                    xamlCounts[ev.ProcessId] = xc + 1;
                }
            }

            Diag.Log("Processes seen: " + allCounts.Count + ", XAML-active processes: " + xamlCounts.Count);

            if (xamlCounts.Count > 0)
            {
                int xamlPid = -1, best = -1;
                foreach (var kv in xamlCounts)
                {
                    if (kv.Value > best) { best = kv.Value; xamlPid = kv.Key; }
                }
                nameById.TryGetValue(xamlPid, out var xname);
                Diag.Log("Most XAML events: PID " + xamlPid + " '" + xname + "' (" + best + " XAML events)");
                return (xname ?? string.Empty, xamlPid);
            }

            // Fallback: busiest non-kernel process.
            int selectedPid = -1, bestCount = -1;
            foreach (var kv in allCounts)
            {
                if (kv.Key == 0 || kv.Key == 4) continue;
                if (!nameById.ContainsKey(kv.Key)) continue;
                if (kv.Value > bestCount) { bestCount = kv.Value; selectedPid = kv.Key; }
            }

            nameById.TryGetValue(selectedPid, out var name);
            Diag.Log("Fallback (busiest non-kernel): PID " + selectedPid + " '" + name + "' (" + bestCount + " events)");
            return (name ?? string.Empty, selectedPid);
        }

        // Uses Microsoft.Windows.EventTracing (TDH-based) — same library the WinUI
        // viewer app uses — so ProviderName / TaskName / OpcodeName / Payload come
        // back fully resolved and formatted identically to the WinUI viewer.
        // Implementation lives in Services/EtlEventLoader.cs.

        public void EnableTable(TableDescriptor tableDescriptor)
        {
            enabledTables.Add(tableDescriptor.Guid);
        }

        public bool TryEnableTable(TableDescriptor tableDescriptor)
        {
            enabledTables.Add(tableDescriptor.Guid);
            return true;
        }

        public ITableService CreateTableService(TableDescriptor tableDescriptor)
        {
            throw new NotImplementedException();
        }

        public bool DoesTableHaveData(TableDescriptor tableDescriptor)
        {
            return enabledTables.Contains(tableDescriptor.Guid);
        }

        public IEnumerable<TableDescriptor> GetEnabledTables()
        {
            var tables = new List<TableDescriptor>();

            if (enabledTables.Contains(TimelineTrack1Table.TableGuid))
            {
                tables.Add(TimelineTrack1Table.TableDescriptor);
            }

            return tables;
        }

        public void Dispose()
        {
            enabledTables.Clear();
            events.Clear();
            trackOneItems = Array.Empty<TimelineItem>();
        }
    }
}

