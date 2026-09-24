using System;
using System.Collections.Generic;
using CommunityToolkit.WinUI.Helpers;

namespace XamlTelemtryViewerWInui3.Helpers;

/// <summary>
/// Generates perceptually distinct colors using Kelly color base set.
/// For ≤22 events: uses Kelly colors directly.
/// For >22 events: generates variants using HSL space rotation.
/// </summary>
public class DistinctColorProvider
{
    /// <summary>
    /// Kelly color palette - scientifically optimized for maximum perceptual difference
    /// Reference: https://en.wikipedia.org/wiki/Help:IPA/English#Consonants
    /// </summary>
    private static readonly string[] KellyColors = new[]
    {
        "#FFB300",  // Vivid Yellow
        "#803E75",  // Strong Purple
        "#FF6B6B",  // Vivid Red
        "#4A90E2",  // Vivid Blue
        "#F78FB3",  // Strong Pink
        "#56B4E9",  // Strong Sky Blue
        "#009E73",  // Strong Green
        "#E69F00",  // Vivid Orange
        "#D55E00",  // Vivid Orange-Red
        "#CC79A7",  // Moderate Magenta
        "#50C878",  // Emerald Green
        "#FFD700",  // Gold
        "#FF1493",  // Deep Pink
        "#00CED1",  // Dark Turquoise
        "#9932CC",  // Dark Orchid
        "#FF4500",  // Orange Red
        "#2F4F4F",  // Dark Slate Gray
        "#DC143C",  // Crimson
        "#00FA9A",  // Medium Spring Green
        "#FF69B4",  // Hot Pink
        "#1E90FF",  // Dodger Blue
        "#32CD32"   // Lime Green
    };

    /// <summary>
    /// Generate N distinct colors using Kelly palette + HSL variants.
    /// </summary>
    public static List<string> GenerateDistinctColors(int count)
    {
        if (count <= 0)
            return new List<string>();

        var colors = new List<string>();

        // Add Kelly colors first
        for (int i = 0; i < Math.Min(count, KellyColors.Length); i++)
        {
            colors.Add(KellyColors[i]);
        }

        // If we need more than Kelly palette, generate variants using HSL
        if (count > KellyColors.Length)
        {
            int variantsNeeded = count - KellyColors.Length;
            var variants = GenerateHslVariants(variantsNeeded);
            colors.AddRange(variants);
        }

        return colors;
    }

    /// <summary>
    /// Generate color variants by rotating Kelly colors through HSL space
    /// </summary>
    private static List<string> GenerateHslVariants(int count)
    {
        var variants = new List<string>();
        int variantIndex = 0;

        // For each Kelly color, create variations with different HSL adjustments
        for (int i = 0; i < count; i++)
        {
            int kellyIndex = variantIndex % KellyColors.Length;
            int adjustmentIndex = i / KellyColors.Length;

            var baseColor = HexToRgb(KellyColors[kellyIndex]);
            var (h, s, l) = RgbToHsl(baseColor.r, baseColor.g, baseColor.b);

            // Apply HSL variations to create perceptually distinct variants
            var (h2, s2, l2) = ApplyHslAdjustment(h, s, l, adjustmentIndex);
            var variantRgb = HslToRgb(h2, s2, l2);
            var hexColor = RgbToHex(variantRgb.r, variantRgb.g, variantRgb.b);

            variants.Add(hexColor);
            variantIndex++;
        }

        return variants;
    }

    /// <summary>
    /// Apply HSL adjustments to create perceptually distinct variants
    /// </summary>
    private static (double h, double s, double l) ApplyHslAdjustment(double h, double s, double l, int adjustmentIndex)
    {
        // Variation 0: Increase saturation
        if (adjustmentIndex == 0)
            return (h, Math.Min(100, s + 15), l);

        // Variation 1: Decrease saturation, increase lightness
        if (adjustmentIndex == 1)
            return (h, Math.Max(20, s - 10), Math.Min(80, l + 15));

        // Variation 2: Rotate hue by 30 degrees
        if (adjustmentIndex == 2)
            return ((h + 30) % 360, s, l);

        // Variation 3: Rotate hue by 120 degrees, adjust saturation
        if (adjustmentIndex == 3)
            return ((h + 120) % 360, Math.Max(40, s - 15), l);

        // Variation 4+: Rotate hue by 240 degrees
        return ((h + 240) % 360, s, Math.Max(30, l - 10));
    }

    /// <summary>
    /// Convert hex color to RGB via CommunityToolkit's <see cref="ColorHelper.ToColor"/>.
    /// </summary>
    private static (int r, int g, int b) HexToRgb(string hex)
    {
        var color = hex.ToColor();
        return (color.R, color.G, color.B);
    }

    /// <summary>
    /// Convert RGB to HSL (Hue, Saturation, Lightness)
    /// Returns: Hue (0-360), Saturation (0-100), Lightness (0-100)
    /// </summary>
    private static (double h, double s, double l) RgbToHsl(int r, int g, int b)
    {
        double rd = r / 255.0;
        double gd = g / 255.0;
        double bd = b / 255.0;

        double max = Math.Max(Math.Max(rd, gd), bd);
        double min = Math.Min(Math.Min(rd, gd), bd);
        double l = (max + min) / 2.0;

        double h = 0, s = 0;

        if (max != min)
        {
            double d = max - min;
            s = l > 0.5 ? d / (2.0 - max - min) : d / (max + min);

            if (max == rd)
                h = ((gd - bd) / d + (gd < bd ? 6 : 0)) / 6.0;
            else if (max == gd)
                h = ((bd - rd) / d + 2) / 6.0;
            else if (max == bd)
                h = ((rd - gd) / d + 4) / 6.0;
        }

        return (h * 360, s * 100, l * 100);
    }

    /// <summary>
    /// Convert HSL to RGB
    /// Input: Hue (0-360), Saturation (0-100), Lightness (0-100)
    /// </summary>
    private static (int r, int g, int b) HslToRgb(double h, double s, double l)
    {
        h = h % 360;
        s = Math.Max(0, Math.Min(100, s)) / 100.0;
        l = Math.Max(0, Math.Min(100, l)) / 100.0;

        double c = (1 - Math.Abs(2 * l - 1)) * s;
        double h1 = h / 60.0;
        double x = c * (1 - Math.Abs(h1 % 2 - 1));

        double r1 = 0, g1 = 0, b1 = 0;

        if (h1 < 1)
            (r1, g1, b1) = (c, x, 0);
        else if (h1 < 2)
            (r1, g1, b1) = (x, c, 0);
        else if (h1 < 3)
            (r1, g1, b1) = (0, c, x);
        else if (h1 < 4)
            (r1, g1, b1) = (0, x, c);
        else if (h1 < 5)
            (r1, g1, b1) = (x, 0, c);
        else
            (r1, g1, b1) = (c, 0, x);

        double m = l - c / 2.0;
        int r = (int)Math.Round((r1 + m) * 255);
        int g = (int)Math.Round((g1 + m) * 255);
        int b = (int)Math.Round((b1 + m) * 255);

        return (Math.Clamp(r, 0, 255), Math.Clamp(g, 0, 255), Math.Clamp(b, 0, 255));
    }

    /// <summary>
    /// Convert RGB to hex color string
    /// </summary>
    private static string RgbToHex(int r, int g, int b)
    {
        return $"#{r:X2}{g:X2}{b:X2}";
    }
}
