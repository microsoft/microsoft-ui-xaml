using System;
using System.Collections.Generic;
using CommunityToolkit.WinUI.Helpers;
using XamlTelemtryViewerWInui3.Models;

namespace XamlTelemtryViewerWInui3.Helpers;

/// <summary>
/// Pure, UI-independent color helpers for the timeline: parsing hex color
/// strings into <see cref="Windows.UI.Color"/> values and assigning perceptually
/// distinct colors to provider events. <see cref="Windows.UI.Color"/> is a value
/// type (not a UI element), so these methods carry no XAML/layout dependencies.
/// </summary>
public static class TimelineColorHelper
{
    /// <summary>
    /// Parses a XAML color string (hex "#RGB"/"#ARGB"/"#RRGGBB"/"#AARRGGBB", scRGB,
    /// or a named color) into a color via CommunityToolkit's <see cref="ColorHelper.ToColor"/>,
    /// falling back to grey for null/empty/invalid input.
    /// </summary>
    public static Windows.UI.Color ParseColor(string colorStr)
    {
        if (string.IsNullOrEmpty(colorStr))
            return Windows.UI.Color.FromArgb(255, 128, 128, 128);

        try
        {
            return colorStr.ToColor();
        }
        catch
        {
            return Windows.UI.Color.FromArgb(255, 128, 128, 128);
        }
    }

    /// <summary>
    /// Assign colors per provider using degree-based heuristic
    /// </summary>
    public static Dictionary<string, string> AssignColorsPerProvider(List<SpecialChildrenByProvider> groupedByProvider)
    {
        var flatColorMap = new Dictionary<string, string>();

        foreach (var providerGroup in groupedByProvider)
        {
            var providerColorMap = AssignColorsWithinProvider(providerGroup.SpecialChildren);
            foreach (var (eventName, color) in providerColorMap)
            {
                flatColorMap[eventName] = color;
            }
        }

        return flatColorMap;
    }

    /// <summary>
    /// Assign distinct colors to all events in a provider using Kelly colors.
    /// - If ≤22 events: use Kelly palette directly
    /// - If >22 events: use Kelly palette + HSL variants
    /// </summary>
    public static Dictionary<string, string> AssignColorsWithinProvider(List<SpecialChild> specialChildren)
    {
        if (specialChildren.Count == 0)
            return new Dictionary<string, string>();

        // Generate N distinct colors (N = number of events in this provider)
        var colors = DistinctColorProvider.GenerateDistinctColors(specialChildren.Count);

        // Map each event name to a color sequentially
        var colorMap = new Dictionary<string, string>();
        for (int i = 0; i < specialChildren.Count && i < colors.Count; i++)
        {
            colorMap[specialChildren[i].EventName] = colors[i];
        }

        return colorMap;
    }
}
