namespace XamlTimeline
{
    /// <summary>Which lifecycle track a region renders on.</summary>
    public enum TimelineTrack
    {
        /// <summary>Full-trace timeline (out-of-proc lifecycle: shell/kernel/app).</summary>
        Trace = 1,

        /// <summary>Process-scoped timeline (in-proc phases of the selected process).</summary>
        Process = 2,
    }

    /// <summary>Whether a region renders as a point marker or a duration phase.</summary>
    public enum RegionKind
    {
        Marker,
        Phase,
    }
}
