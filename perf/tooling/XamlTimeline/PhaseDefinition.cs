using System.Collections.Generic;

namespace XamlTimeline
{
    /// <summary>
    /// Region definition loaded from the regions XML file. Static description; the
    /// concrete instances detected in a trace are <see cref="TimelineItem"/>s built
    /// by <see cref="TimelineBuilder"/>.
    /// </summary>
    /// <remarks>
    /// Uses plain get/set (no <c>required</c>/<c>init</c>) so the netstandard2.0 /
    /// C# 7.3 WPA plugin can consume it without language-version friction.
    /// </remarks>
    public sealed class PhaseDefinition
    {
        public string Name { get; set; } = string.Empty;
        public string Color { get; set; } = string.Empty;
        public TimelineTrack Track { get; set; }
        public RegionKind Kind { get; set; }
        public EventMatcher? Start { get; set; }
        public EventMatcher? Stop { get; set; }
        public string? Parent { get; set; }
        public List<string> SpecialProviders { get; set; } = new List<string>();
        public double ThresholdPercentage { get; set; } = 5.0;
        public bool ProcessDependent { get; set; }
    }
}
