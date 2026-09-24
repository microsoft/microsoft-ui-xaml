using System;
using System.Collections.Generic;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Media;

namespace OSFrameworkLens.UI;

/// <summary>Row in the Analyze report — one framework's slice of a window.</summary>
public sealed class FrameworkSliceVm
{
    public string DisplayName { get; init; } = "";
    public string RightStats { get; init; } = "";
    public Brush Brush { get; init; } = HexColor.GetBrush(null);
    public double BarWidth { get; init; }
    public IReadOnlyList<string> SampleClasses { get; init; } = Array.Empty<string>();
    public string WhereDescription { get; init; } = "";
    public Visibility WhereDescriptionVisibility =>
        string.IsNullOrEmpty(WhereDescription) ? Visibility.Collapsed : Visibility.Visible;
}
