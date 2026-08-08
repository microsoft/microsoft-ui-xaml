using System;
using System.Collections.Generic;

namespace XamlTelemtryViewerWInui3.Models;

public record SpecialChildrenByProvider
{
    public string ProviderName { get; init; }
    public List<SpecialChild> SpecialChildren { get; init; } = new();
    public TimeSpan TotalCumulativeDuration { get; init; }
}
