using System;
using System.Collections.Generic;

namespace XamlTimeline
{
    /// <summary>
    /// Builds Track 1 timeline items (markers + phases) from a list of
    /// <see cref="TelemetryEvent"/>s, a set of region definitions, and a selected
    /// process. This is the single shared detection algorithm used by both the
    /// WinUI viewer (which wraps the result in a TraceTimelineModel) and the WPA
    /// plugin (which concatenates results across candidate processes).
    /// </summary>
    /// <remarks>
    /// Implemented with plain netstandard2.0 string APIs (no Span / range operators)
    /// so the assembly needs no external packages and compiles for the C# 7.3 plugin.
    /// </remarks>
    public sealed class TimelineBuilder
    {
        /// <summary>
        /// Detects Track 1 (Trace-track) markers and phases for a single selected
        /// process. Track 2 (Process-track) definitions are skipped — those are built
        /// on demand by the viewer when the user drills into a Track 1 item.
        /// Each returned item is stamped with <paramref name="selectedProcessId"/> /
        /// <paramref name="selectedProcessName"/> so multi-process callers can pivot on it.
        /// </summary>
        public List<TimelineItem> BuildTrackOne(
            IReadOnlyList<TelemetryEvent> events,
            IReadOnlyList<PhaseDefinition> definitions,
            int selectedProcessId,
            string selectedProcessName)
        {
            if (events is null) throw new ArgumentNullException(nameof(events));
            if (definitions is null) throw new ArgumentNullException(nameof(definitions));

            var processNameToken = ExtractProcessNameToken(selectedProcessName ?? string.Empty);
            var trackOne = new List<TimelineItem>();

            foreach (var def in definitions)
            {
                // Skip Track 2 (Process timeline) definitions; those are built dynamically.
                if (def.Track == TimelineTrack.Process)
                {
                    continue;
                }

                var items = def.Kind == RegionKind.Marker
                    ? DetectMarkers(events, def, selectedProcessId, processNameToken)
                    : DetectPhases(events, def, selectedProcessId, processNameToken);

                foreach (var item in items)
                {
                    item.ProcessName = selectedProcessName ?? string.Empty;
                    item.ProcessId = selectedProcessId;
                    trackOne.Add(item);
                }
            }

            trackOne.Sort((a, b) => a.Start.CompareTo(b.Start));
            return trackOne;
        }

        private static IEnumerable<TimelineItem> DetectMarkers(
            IReadOnlyList<TelemetryEvent> events,
            PhaseDefinition def,
            int selectedProcessId,
            string processNameToken)
        {
            if (def.Start is null)
            {
                yield break;
            }

            // Track 1 markers may come from any process and use ${ProcessName} payload matching.
            // ProcessDependent Track 1 markers also filter by selected process.
            var requirePid = def.Track == TimelineTrack.Process || def.ProcessDependent;

            foreach (var ev in events)
            {
                if (requirePid && ev.ProcessId != selectedProcessId)
                {
                    continue;
                }

                if (!def.Start.Matches(ev, processNameToken))
                {
                    continue;
                }

                yield return new TimelineItem
                {
                    Name = def.Name,
                    Color = def.Color,
                    Track = def.Track,
                    Kind = RegionKind.Marker,
                    StartEvent = ev,
                    EndEvent = null,
                };

                yield break;  // Return first match only.
            }
        }

        private static IEnumerable<TimelineItem> DetectPhases(
            IReadOnlyList<TelemetryEvent> events,
            PhaseDefinition def,
            int selectedProcessId,
            string processNameToken)
        {
            if (def.Start is null || def.Stop is null)
            {
                yield break;
            }

            // Track 1 phases are PID-agnostic: their Start/Stop events may fire in
            // different processes, so we rely on payload filters and temporal pairing.
            // ProcessDependent Track 1 phases also filter by selected process.
            var requirePid = def.Track == TimelineTrack.Process || def.ProcessDependent;

            TelemetryEvent? pendingStart = null;

            for (var i = 0; i < events.Count; i++)
            {
                var ev = events[i];

                if (requirePid && ev.ProcessId != selectedProcessId)
                {
                    continue;
                }

                if (pendingStart is null)
                {
                    if (!def.Start.Matches(ev, processNameToken))
                    {
                        continue;
                    }

                    if (def.Start.ProcessDependentEnd && ev.ProcessId != selectedProcessId)
                    {
                        continue;
                    }

                    pendingStart = ev;

                    // If XamlEndHeuristic is enabled, scan from just after the start
                    // event's index (the end can never precede the start).
                    if (def.Stop.XamlEndHeuristic)
                    {
                        var stopEvent = FindingHeuristicEndpointForXaml(events, i, def, requirePid, selectedProcessId);

                        // No endpoint found — treat the phase as nonexistent.
                        if (stopEvent is null)
                        {
                            pendingStart = null;
                            continue;
                        }

                        yield return new TimelineItem
                        {
                            Name = def.Name,
                            Color = def.Color,
                            Track = def.Track,
                            Kind = RegionKind.Phase,
                            StartEvent = pendingStart,
                            EndEvent = stopEvent,
                        };

                        pendingStart = null;
                    }
                }
                else if (def.Stop.Matches(ev, processNameToken))
                {
                    if (def.Stop.ProcessDependentEnd && ev.ProcessId != selectedProcessId)
                    {
                        continue;
                    }

                    yield return new TimelineItem
                    {
                        Name = def.Name,
                        Color = def.Color,
                        Track = def.Track,
                        Kind = RegionKind.Phase,
                        StartEvent = pendingStart,
                        EndEvent = ev,
                    };

                    pendingStart = null;
                }
            }

            // Unmatched trailing Start (no Stop seen) is intentionally dropped: a phase
            // without a definite End is not emitted.
        }

        /// <summary>
        /// Stack-based frame-pairing strategy to find the proper stop endpoint of a
        /// XAML phase. Builds start/stop patterns from def.Stop.EventName
        /// (+"/win:Start" / +"/win:Stop"), pairs them on a stack, and returns the stop
        /// event after 15 synchronized pairs (else the last stop seen, or null if none).
        /// </summary>
        private static TelemetryEvent? FindingHeuristicEndpointForXaml(
            IReadOnlyList<TelemetryEvent> events,
            int startIndex,
            PhaseDefinition def,
            bool requirePid,
            int selectedProcessId)
        {
            var stopMatcher = def.Stop!;
            string startEventNamePattern = stopMatcher.EventName + "/win:Start";
            string stopEventNamePattern = stopMatcher.EventName + "/win:Stop";

            var frameStartStack = new Stack<TelemetryEvent>();
            int synchronizedPairCount = 0;
            TelemetryEvent? lastFrameStop = null;

            for (var i = startIndex + 1; i < events.Count; i++)
            {
                var ev = events[i];

                bool needsProcessFilter = requirePid || stopMatcher.ProcessDependentEnd;
                if (needsProcessFilter && ev.ProcessId != selectedProcessId)
                {
                    continue;
                }

                if (ev.ProviderGuid == stopMatcher.ProviderGuid &&
                    ev.EventName.Equals(startEventNamePattern, StringComparison.OrdinalIgnoreCase))
                {
                    frameStartStack.Push(ev);
                    continue;
                }

                if (ev.ProviderGuid == stopMatcher.ProviderGuid &&
                    ev.EventName.Equals(stopEventNamePattern, StringComparison.OrdinalIgnoreCase))
                {
                    lastFrameStop = ev;

                    if (frameStartStack.Count > 0)
                    {
                        frameStartStack.Pop();
                        synchronizedPairCount++;

                        if (synchronizedPairCount == 15)
                        {
                            return ev;
                        }
                    }
                }
            }

            return lastFrameStop;
        }

        /// <summary>
        /// Extracts the matchable token from a process image name: strips a trailing
        /// ".exe" and a leading "ms" prefix (e.g. "MyApp.exe" -> "MyApp", "msfoo" -> "foo").
        /// </summary>
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
