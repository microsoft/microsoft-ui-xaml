using System;

namespace XamlTimeline
{
    /// <summary>
    /// A concrete region detected in a trace: either a point-in-time marker
    /// (Kind = Marker; Start has value, End is null) or a duration phase
    /// (Kind = Phase; Start and End both set).
    /// </summary>
    /// <remarks>
    /// Uses plain get/set with a nullable <see cref="StartEvent"/> so the
    /// netstandard2.0 / C# 7.3 WPA plugin can construct placeholder items
    /// (e.g. the "no regions detected" row) without language-version friction.
    /// </remarks>
    public sealed class TimelineItem
    {
        public string Name { get; set; } = string.Empty;
        public string Color { get; set; } = string.Empty;
        public TimelineTrack Track { get; set; }
        public RegionKind Kind { get; set; }

        /// <summary>The raw event that triggered the start (or the only event for a marker). May be null for synthetic placeholder items.</summary>
        public TelemetryEvent? StartEvent { get; set; }

        /// <summary>The raw event that triggered the end. Null for markers and for unmatched phases.</summary>
        public TelemetryEvent? EndEvent { get; set; }

        /// <summary>Start time, taken from <see cref="StartEvent"/>. Read-only.</summary>
        public DateTime Start => StartEvent?.Timestamp ?? default;

        /// <summary>End time, taken from <see cref="EndEvent"/>; null for markers / unmatched phases. Read-only.</summary>
        public DateTime? End => EndEvent?.Timestamp;

        /// <summary>Owning process image name (set by the multi-process Track 1 builder; unused by the viewer).</summary>
        public string ProcessName { get; set; } = string.Empty;

        /// <summary>Owning process id (set by the multi-process Track 1 builder; unused by the viewer).</summary>
        public int ProcessId { get; set; }

        public TimeSpan? Duration => End.HasValue ? End.Value - Start : (TimeSpan?)null;
    }
}
