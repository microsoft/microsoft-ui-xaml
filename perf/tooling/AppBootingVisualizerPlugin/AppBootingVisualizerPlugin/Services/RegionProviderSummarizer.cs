using System;
using System.Collections.Generic;
using AppBootingVisualizerPlugin.Models;
using XamlTimeline;

namespace AppBootingVisualizerPlugin.Services
{
    /// <summary>
    /// Mirrors the WinUI reference app's ProviderSummaryBuilder. For a given
    /// Track 1 region, returns the providers that should appear in that region's
    /// "Events by Provider" view: any provider whose cumulative event duration
    /// inside the region's time window meets or exceeds
    /// (parentDuration * ThresholdPercentage / 100).
    ///
    /// Two important rules carried over from the WinUI app:
    ///   1. The Microsoft-Windows-XAML provider is **excluded** from the
    ///      threshold computation when the region is NOT ProcessDependent.
    ///      Reason: XAML provider events are dense and would dominate every
    ///      non-XAML phase (e.g. Pre-Process, Packaged App Init) and crowd out
    ///      shell / kernel / appx providers we actually care about there.
    ///   2. For ProcessDependent regions the calculation is restricted to the
    ///      PID that fired that specific TimelineItem — so XAML phases only
    ///      reflect the launching app's own activity.
    ///
    /// An explicit SpecialProviders entry in the region XML always wins over
    /// these rules — even XAML can be force-included via SpecialProviders.
    /// </summary>
    public static class RegionProviderSummarizer
    {
        // Microsoft-Windows-XAML provider GUID. Same constant lives in
        // AppBootingVisualizerPluginProcessor; duplicated here so the service is
        // standalone (no circular dep on the processor type).
        private static readonly Guid XamlProviderGuid =
            Guid.Parse("{531a35ab-63ce-4bcf-aa98-f88c7a89e455}");

        /// <summary>
        /// Union (across every detected occurrence of this region) of provider
        /// names that exceeded the threshold inside at least one occurrence's
        /// window. Case-insensitive set.
        /// </summary>
        public static HashSet<string> ComputeThresholdProviders(
            PhaseDefinition region,
            IEnumerable<TimelineItem> regionItems,
            IReadOnlyList<PairedEvent> pairs)
        {
            var hits = new HashSet<string>(StringComparer.OrdinalIgnoreCase);

            if (region == null || regionItems == null || pairs == null || pairs.Count == 0)
            {
                return hits;
            }

            var thresholdFraction = region.ThresholdPercentage / 100.0;
            if (thresholdFraction <= 0.0)
            {
                return hits;
            }

            foreach (var item in regionItems)
            {
                if (item == null || !item.End.HasValue)
                {
                    continue;
                }

                var windowStart = item.Start;
                var windowEnd = item.End.Value;
                var windowDuration = windowEnd - windowStart;
                if (windowDuration <= TimeSpan.Zero)
                {
                    continue;
                }

                var thresholdTicks = (long)(windowDuration.Ticks * thresholdFraction);
                if (thresholdTicks <= 0)
                {
                    continue;
                }
                var threshold = TimeSpan.FromTicks(thresholdTicks);

                // Sum paired-event duration per provider for pairs whose Start
                // falls inside the window (matches the reference's
                // FilterEventsByTimeRange semantics — filter by event timestamp,
                // not by overlap).
                var byProvider = new Dictionary<string, TimeSpan>(
                    StringComparer.OrdinalIgnoreCase);

                foreach (var pair in pairs)
                {
                    if (pair == null || string.IsNullOrEmpty(pair.ProviderName))
                    {
                        continue;
                    }

                    if (region.ProcessDependent && pair.ProcessId != item.ProcessId)
                    {
                        continue;
                    }

                    if (!region.ProcessDependent && pair.ProviderGuid == XamlProviderGuid)
                    {
                        // Excluded from threshold computation — see class summary.
                        continue;
                    }

                    if (pair.Start < windowStart || pair.Start > windowEnd)
                    {
                        continue;
                    }

                    if (!byProvider.TryGetValue(pair.ProviderName, out var acc))
                    {
                        acc = TimeSpan.Zero;
                    }
                    byProvider[pair.ProviderName] = acc + pair.Duration;
                }

                foreach (var kvp in byProvider)
                {
                    if (kvp.Value >= threshold)
                    {
                        hits.Add(kvp.Key);
                    }
                }
            }

            return hits;
        }
    }
}
