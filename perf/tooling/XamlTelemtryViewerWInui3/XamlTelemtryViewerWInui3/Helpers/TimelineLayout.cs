using System;
using System.Collections.Generic;
using XamlTelemtryViewerWInui3.Models.Timeline;

namespace XamlTelemtryViewerWInui3.Helpers;

/// <summary>
/// Pure, UI-independent layout heuristics for Track 1 items: grouping items that
/// share a timestamp and computing vertical stagger offsets for overlapping
/// markers. Contains no XAML dependencies.
/// </summary>
public static class TimelineLayout
{
    /// <summary>
    /// Groups items by start timestamp to handle multiple markers at the same time.
    /// </summary>
    public static Dictionary<DateTime, List<TimelineItem>> GroupItemsByTimestamp(IReadOnlyList<TimelineItem> items)
    {
        var groups = new Dictionary<DateTime, List<TimelineItem>>();
        foreach (var item in items)
        {
            if (!groups.ContainsKey(item.Start))
            {
                groups[item.Start] = new List<TimelineItem>();
            }
            groups[item.Start].Add(item);
        }
        return groups;
    }

    /// <summary>
    /// Calculates vertical offset for markers when multiple markers share the same timestamp.
    /// Returns: the Y-offset and index within the group (for positioning).
    /// </summary>
    public static (double yOffset, int indexInGroup) CalculateMarkerYOffset(
        DateTime itemStart,
        TimelineItem item,
        Dictionary<DateTime, List<TimelineItem>> timestampGroups,
        double markerZone)
    {
        if (!timestampGroups.TryGetValue(itemStart, out var itemsAtTime) || itemsAtTime.Count <= 1)
        {
            return (0, 0);  // No offset if only one item at this time
        }

        var indexInGroup = itemsAtTime.IndexOf(item);
        if (indexInGroup < 0) indexInGroup = 0;

        // Stagger vertically: each item offset by 8 pixels maximum
        const double maxStagger = 8.0;
        var yOffset = (indexInGroup * maxStagger) % markerZone;

        return (yOffset, indexInGroup);
    }
}
