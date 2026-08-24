using System;

namespace XamlTelemtryViewerWInui3.Helpers;

/// <summary>
/// Pure, UI-independent math helpers for timeline axes: choosing "nice" tick
/// intervals and formatting millisecond durations. These contain no XAML or
/// view-state dependencies and are safe to unit test in isolation.
/// </summary>
public static class TimelineMath
{
    /// <summary>
    /// Returns a "nice" tick interval (1/2/5 * 10^n) for an absolute span in milliseconds.
    /// </summary>
    public static double NiceInterval(double span)
    {
        if (span <= 0) return 0.001;

        var exponent = Math.Floor(Math.Log10(span));
        var mantissa = span / Math.Pow(10, exponent);

        double interval;
        if (mantissa < 1.5) interval = 1;
        else if (mantissa < 3) interval = 2;
        else if (mantissa < 7) interval = 5;
        else interval = 10;

        return interval * Math.Pow(10, exponent);
    }

    /// <summary>
    /// Formats a millisecond duration as a short label (e.g., "1.5s" or "12.345ms").
    /// </summary>
    public static string FormatMs(double ms)
    {
        if (ms >= 1000)
            return $"{ms / 1000:F1}s";
        else
            return $"{ms:F3}ms";
    }
}
