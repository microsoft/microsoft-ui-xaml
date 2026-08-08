using System;
using System.Collections.Generic;

namespace XamlTelemtryViewerWInui3.Models;

public record SpecialChild
{
    public string EventName { get; init; }
    public string ProviderName { get; init; }
    public List<(DateTime Start, DateTime Stop)> Occurrences { get; init; } = new();
    public TimeSpan CumulativeDuration { get; init; }
    public double PercentageOfParent { get; init; }
    public string Reason { get; init; } // "Special Provider" or ">X% threshold"
}
