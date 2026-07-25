using System;

namespace XamlTelemtryViewerWInui3.Models
{
    public sealed record ProcessInfo(
        int Id,
        string Name,
        string? ImagePath,
        string? CommandLine,
        DateTime? CreateTime,
        DateTime? ExitTime,
        int EventCount)
    {
        public override string ToString() => $"{Name} ({Id})";
    }
}
