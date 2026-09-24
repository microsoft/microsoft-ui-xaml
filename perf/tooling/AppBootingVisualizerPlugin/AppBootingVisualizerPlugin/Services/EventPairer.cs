using System;
using System.Collections.Generic;
using AppBootingVisualizerPlugin.Models;
using XamlTimeline;

namespace AppBootingVisualizerPlugin.Services
{
    // Pairs ETW Start/Stop events into phases with real durations.
    //
    // Mirrors the WinUI reference app's SpecialChildrenBuilder (perf/tooling/
    // XamlTelemtryViewerWInui3/.../Services/SpecialChildrenBuilder.cs) exactly:
    //
    //   - Process events in chronological order.
    //   - Stack key = (baseEventName, ThreadId). 
    //     in Windows so we don't need to include ProcessId in the key.
    //   - baseEventName = strip everything from "/win:" onward (e.g.
    //     "AppCreation/win:Start" -> "AppCreation").
    //   - Opcode == 1 (win:Start): push onto the stack for that key.
    //   - Opcode == 2 (win:Stop): pop the most recent matching start, emit a pair.
    //   - All other opcodes (notably 0 = win:Info) are dropped.
    //   - Unmatched starts (no stop) and unmatched stops (no start) are dropped.
    //
    // The output is sorted by Start time so downstream consumers (the Track 2
    // Gantt) get rows in chronological order.
    public sealed class EventPairer
    {
        public IReadOnlyList<PairedEvent> Pair(IReadOnlyList<TelemetryEvent> events)
        {
            var pairs = new List<PairedEvent>();
            if (events == null || events.Count == 0)
            {
                Diag.Log("EventPairer: no events to pair.");
                return pairs;
            }

            var stacks = new Dictionary<(string Name, int Tid), Stack<TelemetryEvent>>();
            int unmatchedStops = 0;
            int infoDropped = 0;

            foreach (var evt in events)
            {
                if (evt.Opcode != 1 && evt.Opcode != 2)
                {
                    infoDropped++;
                    continue;
                }

                var baseName = StripOpcodeSuffix(evt.EventName);
                if (string.IsNullOrEmpty(baseName))
                {
                    continue;
                }

                var key = (baseName, evt.ThreadId);

                if (evt.Opcode == 1)
                {
                    if (!stacks.TryGetValue(key, out var stack))
                    {
                        stack = new Stack<TelemetryEvent>();
                        stacks[key] = stack;
                    }
                    stack.Push(evt);
                }
                else  // Opcode == 2
                {
                    if (stacks.TryGetValue(key, out var stack) && stack.Count > 0)
                    {
                        var startEvt = stack.Pop();
                        pairs.Add(new PairedEvent
                        {
                            EventName = baseName,
                            ProviderName = startEvt.ProviderName,
                            ProviderGuid = startEvt.ProviderGuid,
                            ProcessName = startEvt.ProcessName,
                            ProcessId = startEvt.ProcessId,
                            ThreadId = startEvt.ThreadId,
                            Level = startEvt.Level,
                            Start = startEvt.Timestamp,
                            Stop = evt.Timestamp,
                            StartPayload = startEvt.PayloadText,
                            StopPayload = evt.PayloadText,
                        });
                    }
                    else
                    {
                        unmatchedStops++;
                    }
                }
            }

            int unmatchedStarts = 0;
            foreach (var kv in stacks)
            {
                unmatchedStarts += kv.Value.Count;
            }

            pairs.Sort((a, b) => a.Start.CompareTo(b.Start));

            Diag.Log("EventPairer: " + pairs.Count + " pairs, "
                + unmatchedStarts + " unmatched starts, "
                + unmatchedStops + " unmatched stops, "
                + infoDropped + " non-Start/Stop events dropped.");

            return pairs;
        }

        private static string StripOpcodeSuffix(string eventName)
        {
            if (string.IsNullOrEmpty(eventName))
            {
                return string.Empty;
            }

            var idx = eventName.IndexOf("/win:", StringComparison.OrdinalIgnoreCase);
            return idx >= 0 ? eventName.Substring(0, idx) : eventName;
        }
    }
}
