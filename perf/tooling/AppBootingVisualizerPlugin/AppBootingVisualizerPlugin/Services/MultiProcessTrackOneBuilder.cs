using System;
using System.Collections.Generic;
using XamlTimeline;

namespace AppBootingVisualizerPlugin.Services
{
    /// <summary>
    /// Plugin-only Track 1 builder that produces the SAME items as calling the shared
    /// <see cref="TimelineBuilder.BuildTrackOne(IReadOnlyList{TelemetryEvent}, IReadOnlyList{PhaseDefinition}, int, string)"/>
    /// once per candidate process, but in a single set of passes over the events instead
    /// of one full scan per process.
    ///
    /// Why this exists (and why it is NOT in the shared <see cref="TimelineBuilder"/>):
    /// the WinUI viewer only ever builds Track 1 for ONE selected process, so the shared
    /// per-process builder is optimal there. The WPA plugin expands EVERY process in the
    /// trace, so the shared approach is O(processes × events × regions) — expensive on
    /// large multi-process traces. This builder keeps the shared builder untouched while
    /// giving the plugin an O(events × regions) path (plus a tiny per-match × processes
    /// term that only fires on the rare events that match a region's Start/Stop pattern).
    ///
    /// Equivalence strategy:
    ///   * ProcessDependent regions (which include the tricky XAML end-heuristic phase)
    ///     only ever match events from the selected process, so we bucket events by
    ///     ProcessId ONCE and delegate each bucket to the shared, fully-tested
    ///     <see cref="TimelineBuilder.BuildTrackOne"/>. The heuristic logic is therefore
    ///     reused verbatim, never reimplemented.
    ///   * Non-ProcessDependent regions are payload-routed (they match events from any
    ///     process via a <c>${ProcessName}</c> payload token), so each process runs its
    ///     own little state machine over the shared event stream in a single pass. Payload
    ///     matching itself is delegated to <see cref="EventMatcher.Matches(TelemetryEvent, string?)"/>
    ///     (the same overload the shared builder uses), so it is never reimplemented here.
    /// </summary>
    public sealed class MultiProcessTrackOneBuilder
    {
        /// <summary>
        /// Builds Track 1 items for all <paramref name="processes"/> in one set of passes.
        /// The returned list is sorted by start time, matching the shared builder.
        /// </summary>
        public List<TimelineItem> Build(
            IReadOnlyList<TelemetryEvent> events,
            IReadOnlyList<PhaseDefinition> definitions,
            IReadOnlyList<(string Name, int Id)> processes)
        {
            if (events is null) throw new ArgumentNullException(nameof(events));
            if (definitions is null) throw new ArgumentNullException(nameof(definitions));
            if (processes is null) throw new ArgumentNullException(nameof(processes));

            // Classify the Track 1 region definitions.
            var processDependentDefs = new List<PhaseDefinition>();
            var nonPdMarkerDefs = new List<PhaseDefinition>();
            var nonPdPhaseDefs = new List<PhaseDefinition>();
            var fallbackDefs = new List<PhaseDefinition>();  // defensive: non-PD heuristic phases (none today)

            foreach (var def in definitions)
            {
                if (def.Track == TimelineTrack.Process)
                {
                    continue;  // Track 2 is built on demand, not here.
                }

                if (def.ProcessDependent)
                {
                    processDependentDefs.Add(def);
                }
                else if (def.Kind == RegionKind.Marker)
                {
                    nonPdMarkerDefs.Add(def);
                }
                else if (def.Stop != null && def.Stop.XamlEndHeuristic)
                {
                    // A non-ProcessDependent phase that uses the XAML end heuristic does
                    // not exist in the shipped regions file, but if one is ever added we
                    // must not mis-handle it: route it through the shared per-process
                    // builder so the heuristic is applied exactly.
                    fallbackDefs.Add(def);
                }
                else
                {
                    nonPdPhaseDefs.Add(def);
                }
            }

            var result = new List<TimelineItem>();
            var shared = new TimelineBuilder();

            // ---- ProcessDependent regions: bucket events by PID, reuse shared builder ----
            if (processDependentDefs.Count > 0)
            {
                var byPid = BucketByProcessId(events);
                var empty = (IReadOnlyList<TelemetryEvent>)Array.Empty<TelemetryEvent>();

                foreach (var proc in processes)
                {
                    var subEvents = byPid.TryGetValue(proc.Id, out var list)
                        ? (IReadOnlyList<TelemetryEvent>)list
                        : empty;

                    // BuildTrackOne already stamps ProcessId/Name and sorts; just collect.
                    result.AddRange(shared.BuildTrackOne(subEvents, processDependentDefs, proc.Id, proc.Name));
                }
            }

            // ---- Defensive fallback (non-PD heuristic phases): shared per-process ----
            if (fallbackDefs.Count > 0)
            {
                foreach (var proc in processes)
                {
                    result.AddRange(shared.BuildTrackOne(events, fallbackDefs, proc.Id, proc.Name));
                }
            }

            // ---- Non-ProcessDependent markers / phases: single pass per definition ----
            foreach (var def in nonPdMarkerDefs)
            {
                BatchDetectMarkers(events, def, processes, result);
            }

            foreach (var def in nonPdPhaseDefs)
            {
                BatchDetectPhases(events, def, processes, result);
            }

            result.Sort((a, b) => a.Start.CompareTo(b.Start));
            return result;
        }

        private static Dictionary<int, List<TelemetryEvent>> BucketByProcessId(IReadOnlyList<TelemetryEvent> events)
        {
            var byPid = new Dictionary<int, List<TelemetryEvent>>();
            for (var i = 0; i < events.Count; i++)
            {
                var ev = events[i];
                if (!byPid.TryGetValue(ev.ProcessId, out var list))
                {
                    list = new List<TelemetryEvent>();
                    byPid[ev.ProcessId] = list;
                }

                list.Add(ev);
            }

            return byPid;
        }

        /// <summary>
        /// Single-pass equivalent of running the shared DetectMarkers for a
        /// non-ProcessDependent marker against every process. For each process, emits the
        /// first event that matches the Start pattern and (if present) whose payload
        /// contains the process's <c>${ProcessName}</c> token.
        /// </summary>
        private static void BatchDetectMarkers(
            IReadOnlyList<TelemetryEvent> events,
            PhaseDefinition def,
            IReadOnlyList<(string Name, int Id)> processes,
            List<TimelineItem> result)
        {
            if (def.Start == null)
            {
                return;
            }

            var n = processes.Count;
            if (n == 0)
            {
                return;
            }

            var token = new string[n];
            var emitted = new bool[n];
            for (var k = 0; k < n; k++)
            {
                token[k] = ExtractProcessNameToken(processes[k].Name ?? string.Empty);
            }

            var remaining = n;

            for (var i = 0; i < events.Count && remaining > 0; i++)
            {
                var ev = events[i];
                if (!def.Start.Matches(ev))
                {
                    continue;
                }

                for (var k = 0; k < n; k++)
                {
                    if (emitted[k])
                    {
                        continue;
                    }

                    if (!def.Start.Matches(ev, token[k]))
                    {
                        continue;
                    }

                    result.Add(new TimelineItem
                    {
                        Name = def.Name,
                        Color = def.Color,
                        Track = def.Track,
                        Kind = RegionKind.Marker,
                        StartEvent = ev,
                        EndEvent = null,
                        ProcessName = processes[k].Name ?? string.Empty,
                        ProcessId = processes[k].Id,
                    });

                    emitted[k] = true;
                    remaining--;
                }
            }
        }

        /// <summary>
        /// Single-pass equivalent of running the shared DetectPhases (non-heuristic
        /// branch) for a non-ProcessDependent phase against every process. Each process
        /// keeps its own pending-start slot; Start/Stop payload tokens and any
        /// <c>ProcessDependentEnd</c> filters are applied per process exactly as the
        /// shared builder does.
        /// </summary>
        private static void BatchDetectPhases(
            IReadOnlyList<TelemetryEvent> events,
            PhaseDefinition def,
            IReadOnlyList<(string Name, int Id)> processes,
            List<TimelineItem> result)
        {
            if (def.Start == null || def.Stop == null)
            {
                return;
            }

            var n = processes.Count;
            if (n == 0)
            {
                return;
            }

            var startPdEnd = def.Start.ProcessDependentEnd;
            var stopPdEnd = def.Stop.ProcessDependentEnd;

            var pendingStart = new TelemetryEvent[n];
            var token = new string[n];
            for (var k = 0; k < n; k++)
            {
                token[k] = ExtractProcessNameToken(processes[k].Name ?? string.Empty);
            }

            for (var i = 0; i < events.Count; i++)
            {
                var ev = events[i];
                var isStart = def.Start.Matches(ev);
                var isStop = def.Stop.Matches(ev);
                if (!isStart && !isStop)
                {
                    continue;
                }

                for (var k = 0; k < n; k++)
                {
                    if (pendingStart[k] == null)
                    {
                        if (!isStart)
                        {
                            continue;
                        }

                        if (!def.Start.Matches(ev, token[k]))
                        {
                            continue;
                        }

                        if (startPdEnd && ev.ProcessId != processes[k].Id)
                        {
                            continue;
                        }

                        pendingStart[k] = ev;
                    }
                    else
                    {
                        if (!isStop)
                        {
                            continue;
                        }

                        if (!def.Stop.Matches(ev, token[k]))
                        {
                            continue;
                        }

                        if (stopPdEnd && ev.ProcessId != processes[k].Id)
                        {
                            continue;
                        }

                        var start = pendingStart[k];
                        result.Add(new TimelineItem
                        {
                            Name = def.Name,
                            Color = def.Color,
                            Track = def.Track,
                            Kind = RegionKind.Phase,
                            StartEvent = start,
                            EndEvent = ev,
                            ProcessName = processes[k].Name ?? string.Empty,
                            ProcessId = processes[k].Id,
                        });

                        pendingStart[k] = null;
                    }
                }
            }

            // Unmatched trailing starts (no Stop seen) are intentionally dropped, exactly
            // like the shared DetectPhases.
        }

        // ---------------------------------------------------------------------------
        // Process-name token extraction — intentional port of the private helper in
        // XamlTimeline.TimelineBuilder. Payload matching itself lives on EventMatcher
        // (EventMatcher.Matches(ev, token)); this only resolves the token to pass in.
        // ---------------------------------------------------------------------------

        private static string ExtractProcessNameToken(string imageName)
        {
            if (string.IsNullOrEmpty(imageName))
            {
                return string.Empty;
            }

            var result = imageName;

            if (result.EndsWith(".exe", StringComparison.OrdinalIgnoreCase))
            {
                result = result.Substring(0, result.Length - 4);
            }

            if (result.StartsWith("ms", StringComparison.OrdinalIgnoreCase) && result.Length > 2)
            {
                result = result.Substring(2);
            }

            return result;
        }
    }
}
