namespace XamlTimeline
{
    /// <summary>
    /// Optional payload-field filter. <see cref="Value"/> may contain the
    /// <c>${ProcessName}</c> placeholder which the builder substitutes at evaluation time.
    /// </summary>
    public sealed record PayloadFilter(string Field, string Value);
}
