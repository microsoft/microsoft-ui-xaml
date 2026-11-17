// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Globalization;

namespace WinUIProfiler.Mcp.Sessions;

/// <summary>How the tools show times, addresses and counts.</summary>
internal static class EventJson
{
    /// <summary>A value rounded half away from zero, so 91.25 shows as 91.3, as SQLite's ROUND shows it.</summary>
    internal static double Round(double value, int digits) => Math.Round(value, digits, MidpointRounding.AwayFromZero);

    /// <summary>A time in milliseconds rounded to microseconds, or null when it isn't finite.</summary>
    internal static double? Ms(double milliseconds) => double.IsFinite(milliseconds) ? Round(milliseconds, 3) : null;

    internal static double? Ms(double? milliseconds) => milliseconds is { } value ? Ms(value) : null;

    /// <summary>A UTC time in round-trip form. Session files store UTC times, but parsing them doesn't always mark them so.</summary>
    internal static string Utc(DateTime time)
    {
        var utc = time.Kind switch
        {
            DateTimeKind.Local => time.ToUniversalTime(),
            DateTimeKind.Unspecified => DateTime.SpecifyKind(time, DateTimeKind.Utc),
            _ => time,
        };
        return utc.ToString("O", CultureInfo.InvariantCulture);
    }

    internal static string? Utc(DateTime? time) => time is { } value ? Utc(value) : null;

    internal static string Hex(long address) => "0x" + address.ToString("X", CultureInfo.InvariantCulture);

    /// <summary>A count and its noun, such as "1 event" or "1,234 events".</summary>
    internal static string Count(long count, string one, string many) =>
        string.Create(CultureInfo.InvariantCulture, $"{count:N0} {(count == 1 ? one : many)}");

    /// <summary>The page of a list a view returned, as its computed text says it: "returned 20 from offset 40".</summary>
    internal static string Page(int offset, int returned) =>
        string.Create(CultureInfo.InvariantCulture, $"returned {returned:N0} from offset {offset:N0}");
}
