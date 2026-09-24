using System;
using System.Collections.Generic;
using System.Globalization;
using Microsoft.UI;
using Microsoft.UI.Xaml.Media;
using Windows.UI;

namespace OSFrameworkLens.UI;

/// <summary>Parses <c>#RRGGBB</c> / <c>#AARRGGBB</c> into WinRT <see cref="Color"/>, Win32 <c>COLORREF</c> (<c>0x00BBGGRR</c>), and cached <see cref="SolidColorBrush"/>.</summary>
internal static class HexColor
{
    private const byte OpaqueAlpha = 0xFF;

    public static bool TryParse(string? hex, out Color color)
    {
        color = default;
        if (string.IsNullOrEmpty(hex) || hex[0] != '#') return false;
        var v = hex.AsSpan(1);
        if (v.Length != 6 && v.Length != 8) return false;

        Span<byte> bytes = stackalloc byte[4] { OpaqueAlpha, 0, 0, 0 };
        var offset = v.Length == 8 ? 0 : 1;
        for (int i = 0; i < v.Length / 2; i++)
            if (!byte.TryParse(v.Slice(i * 2, 2), NumberStyles.HexNumber, CultureInfo.InvariantCulture, out bytes[offset + i]))
                return false;

        color = Color.FromArgb(bytes[0], bytes[1], bytes[2], bytes[3]);
        return true;
    }

    /// <summary>Parse to Win32 <c>COLORREF</c> (<c>0x00BBGGRR</c>); alpha is dropped (GDI <c>CreatePen</c> ignores it).</summary>
    public static uint ParseColorRef(string? hex, uint fallbackColorRef) =>
        TryParse(hex, out var c) ? (uint)(c.R | (c.G << 8) | (c.B << 16)) : fallbackColorRef;

    // 30 Hz hot path. ~20 distinct colors across all fingerprints; UI thread only — no locking.
    private static readonly Dictionary<string, SolidColorBrush> s_brushCache = new(StringComparer.Ordinal);
    private static readonly SolidColorBrush s_fallbackBrush = new(Colors.DimGray);

    public static SolidColorBrush GetBrush(string? hex)
    {
        if (string.IsNullOrEmpty(hex)) return s_fallbackBrush;
        if (s_brushCache.TryGetValue(hex, out var cached)) return cached;
        if (!TryParse(hex, out var color)) return s_fallbackBrush;
        return s_brushCache[hex] = new SolidColorBrush(color);
    }
}
