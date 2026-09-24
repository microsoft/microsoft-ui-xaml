using System;
using System.Collections.Generic;
using System.IO;
using AppBootingVisualizerPlugin.Models;
using XamlTimeline;

namespace AppBootingVisualizerPlugin.Services
{
    // Builds the display label for a process across all tables.
    //
    // Default form:               "<ImageName> (<PID>)"        e.g. "Photos.exe (10948)"
    // svchost special-case form:  "<ImageName> (<PID>) (<X>)"  e.g. "svchost.exe (3204) (svchost.exe)"
    //
    // The svchost suffix is pulled from the Microsoft-Windows-Kernel-Process /
    // ProcessStart event's "ImageName" payload field for that PID, reduced to just
    // the file name (no path). This helps disambiguate the many service-host
    // instances when the ProcessStart event was captured in the trace.
    //
    // Each PID gets a unique formatted label, so any column that pivots on the
    // returned string automatically treats two PIDs with the same ImageName as
    // distinct rows.
    internal static class ProcessLabeler
    {
        private const string KernelProcessProviderName = "Microsoft-Windows-Kernel-Process";
        private const int ProcessStartEventId = 1;
        private const string SvchostPrefix = "svchost";

        public static IReadOnlyDictionary<int, string> Build(IReadOnlyList<TelemetryEvent> events)
        {
            if (events == null || events.Count == 0)
            {
                return new Dictionary<int, string>();
            }

            // Pass 1: canonical ImageName per PID (first non-empty wins).
            var names = new Dictionary<int, string>();
            foreach (var ev in events)
            {
                if (!names.ContainsKey(ev.ProcessId) && !string.IsNullOrEmpty(ev.ProcessName))
                {
                    names[ev.ProcessId] = ev.ProcessName;
                }
            }

            // Pass 2: for svchost-named PIDs, scrape ProcessStart's ImageName payload.
            var svchostImage = new Dictionary<int, string>();
            foreach (var ev in events)
            {
                if (ev.EventId != ProcessStartEventId) continue;
                if (!string.Equals(ev.ProviderName, KernelProcessProviderName, StringComparison.OrdinalIgnoreCase)) continue;
                if (svchostImage.ContainsKey(ev.ProcessId)) continue;
                if (!names.TryGetValue(ev.ProcessId, out var procName)) continue;
                if (!procName.StartsWith(SvchostPrefix, StringComparison.OrdinalIgnoreCase)) continue;

                var imagePath = ev.Fields.TryGetValue("ImageName", out var img) ? img : null;
                if (string.IsNullOrEmpty(imagePath)) continue;

                string leaf;
                try { leaf = Path.GetFileName(imagePath); }
                catch (ArgumentException) { leaf = imagePath; }

                if (!string.IsNullOrEmpty(leaf))
                {
                    svchostImage[ev.ProcessId] = leaf;
                }
            }

            var labels = new Dictionary<int, string>(names.Count);
            foreach (var kv in names)
            {
                labels[kv.Key] = Format(kv.Value, kv.Key, svchostImage);
            }
            return labels;
        }

        // Resolve the label for one (name, pid) pair, consulting the prebuilt
        // labels map (preferred — already includes svchost suffix) or building
        // a fallback when the PID was never indexed (e.g., name-less rows).
        public static string Resolve(string processName, int processId, IReadOnlyDictionary<int, string> labels)
        {
            if (labels != null && labels.TryGetValue(processId, out var prebuilt))
            {
                return prebuilt;
            }
            return string.IsNullOrEmpty(processName)
                ? "(pid " + processId + ")"
                : processName + " (" + processId + ")";
        }

        private static string Format(string processName, int processId, Dictionary<int, string> svchostImages)
        {
            var baseLabel = string.IsNullOrEmpty(processName)
                ? "(pid " + processId + ")"
                : processName + " (" + processId + ")";

            if (!string.IsNullOrEmpty(processName) &&
                processName.StartsWith(SvchostPrefix, StringComparison.OrdinalIgnoreCase) &&
                svchostImages.TryGetValue(processId, out var image) &&
                !string.IsNullOrEmpty(image))
            {
                return baseLabel + " (" + image + ")";
            }
            return baseLabel;
        }
    }
}
