using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using XamlTelemtryViewerWInui3.Models;
using XamlTelemtryViewerWInui3.Models.Timeline;

namespace XamlTelemtryViewerWInui3.Services
{
    public class SpecialChildrenBuilder
    {
        private static readonly Guid XamlProviderGuid = new("531a35ab-63ce-4bcf-aa98-f88c7a89e455");
        
        public string DebugOutput { get; private set; } = "";

        public List<SpecialChild> Build(
                List<TelemetryEvent> events,
                DateTime parentStart,
                DateTime parentEnd,
                TimeSpan parentDuration,
                PhaseDefinition parentRegion,
                int? selectedProcessId = null)
        {
                // Read threshold percentage from XamlAppLaunch.regions.xml via PhaseDefinition
                var specialProviderSet = new HashSet<string>(parentRegion.SpecialProviders, StringComparer.OrdinalIgnoreCase);
                var thresholdPercentage = parentRegion.ThresholdPercentage;  // From XML: <ThresholdPercentage>5</ThresholdPercentage>
                var thresholdDuration = parentDuration.TotalMilliseconds * (thresholdPercentage / 100.0);

                // Dictionary: (EventName, ThreadId) -> Stack of Start events
                var eventStacks = new Dictionary<(string, int), Stack<TelemetryEvent>>();

                // Dictionary: (EventName, ProviderName) -> list of {start, stop, duration}
                var eventOccurrences = new Dictionary<(string, string), List<(DateTime Start, DateTime Stop, TimeSpan Duration)>>();

                var eventsInRange = events.Where(e => e.Timestamp >= parentStart && e.Timestamp <= parentEnd).ToList();

                

                // Process events in chronological order
                foreach (var evt in eventsInRange.OrderBy(e => e.Timestamp))
                {
                    // Skip this event if parent is process-dependent and event belongs to different process
                    if (parentRegion.ProcessDependent && selectedProcessId.HasValue && evt.ProcessId != selectedProcessId.Value)
                        continue;

                    // Skip if XAML provider event and parent is NOT process-dependent, but we still need to filter by selected process
                    // XAML provider events should always be filtered by selected process, even if parent region isn't process-dependent
                    if (!parentRegion.ProcessDependent && evt.ProviderGuid == XamlProviderGuid)
                        continue;

                    // Extract base event name (before "/win:" suffix)
                    var baseEventName = evt.EventName.Contains("/win:") 
                            ? evt.EventName.Substring(0, evt.EventName.IndexOf("/win:"))
                            : evt.EventName;

                    var stackKey = (baseEventName, evt.ThreadId);
                    var occKey = (baseEventName, evt.ProviderName);

                    // Opcode 1 = Start
                    if (evt.Opcode == 1)
                    {
                        if (!eventStacks.ContainsKey(stackKey))
                            eventStacks[stackKey] = new Stack<TelemetryEvent>();

                        eventStacks[stackKey].Push(evt);
                    }
                    // Opcode 2 = Stop
                    else if (evt.Opcode == 2)
                    {
                        if (eventStacks.TryGetValue(stackKey, out var stack) && stack.Count > 0)
                        {
                            var startEvent = stack.Pop();
                            var duration = evt.Timestamp - startEvent.Timestamp;

                            if (!eventOccurrences.ContainsKey(occKey))
                                eventOccurrences[occKey] = new List<(DateTime, DateTime, TimeSpan)>();

                            eventOccurrences[occKey].Add((startEvent.Timestamp, evt.Timestamp, duration));
                        }
                    }
                }

         
           

            // Build special children list - include ALL events (no filtering by criteria)
            var specialChildren = new List<SpecialChild>();

            foreach (var kvp in eventOccurrences)
            {
                var (eventName, providerName) = kvp.Key;
                var occurrences = kvp.Value;
                var cumulativeDuration = TimeSpan.FromMilliseconds(occurrences.Sum(o => o.Duration.TotalMilliseconds));
                var percentageOfParent = (cumulativeDuration.TotalMilliseconds / parentDuration.TotalMilliseconds) * 100.0;

                // Add all events as special children - grouping by provider is the organization mechanism
                specialChildren.Add(new SpecialChild
                {
                    EventName = eventName,
                    ProviderName = providerName,
                    Occurrences = occurrences.Select(o => (o.Start, o.Stop)).ToList(),
                    CumulativeDuration = cumulativeDuration,
                    PercentageOfParent = percentageOfParent,
                    Reason = "",  // No reason needed since all are included
                });
            }

            return specialChildren.OrderByDescending(c => c.CumulativeDuration).ToList();
        }

        /// <summary>
        /// Build special children and group them by provider.
        /// </summary>
        public List<SpecialChildrenByProvider> BuildGroupedByProvider(
                List<TelemetryEvent> events,
                DateTime parentStart,
                DateTime parentEnd,
                TimeSpan parentDuration,
                PhaseDefinition parentRegion,
                int? selectedProcessId = null)
        {
            var specialChildren = Build(events, parentStart, parentEnd, parentDuration, parentRegion, selectedProcessId);

            var groupedByProvider = specialChildren
                .GroupBy(sc => sc.ProviderName)
                .Select(group => new SpecialChildrenByProvider
                {
                    ProviderName = group.Key,
                    SpecialChildren = group.OrderByDescending(sc => sc.CumulativeDuration).ToList(),
                    TotalCumulativeDuration = TimeSpan.FromMilliseconds(
                        group.Sum(sc => sc.CumulativeDuration.TotalMilliseconds))
                })
                .OrderByDescending(g => g.TotalCumulativeDuration)
                .ToList();

            return groupedByProvider;
        }
    }
}


