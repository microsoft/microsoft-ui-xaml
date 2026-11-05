// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Globalization;
using WinUIProfiler.Engine.Events;

namespace WinUIProfiler.Mcp.Sessions;

/// <summary>A provider whose events session files hold, with the name the tools show.</summary>
internal sealed record ProviderInfo(XamlProvider Provider, string Name, Guid Guid);

/// <summary>How the tools show providers, times and addresses.</summary>
internal static class EventJson
{
    /// <summary>The providers, in the order of <see cref="XamlProvider"/>.</summary>
    internal static IReadOnlyList<ProviderInfo> Providers { get; } =
    [
        new(XamlProvider.Xaml, "Microsoft-Windows-XAML", XamlProviders.Xaml),
        new(XamlProvider.XamlDiagnostics, "Microsoft-Windows-XAML-Diagnostics", XamlProviders.XamlDiagnostics),
        new(XamlProvider.XamlProfiler, "Microsoft-Windows-XAML-Profiler", XamlProviders.XamlProfiler),
        new(XamlProvider.MicrosoftUIXaml, "Microsoft.UI.Xaml", XamlProviders.MicrosoftUIXaml),
        new(XamlProvider.MicrosoftUIXamlControls, "Microsoft.UI.Xaml.Controls", XamlProviders.MicrosoftUIXamlControls),
        new(XamlProvider.DxgKrnl, "Microsoft-Windows-DxgKrnl", XamlProviders.DxgKrnl),
        new(XamlProvider.Win32k, "Microsoft-Windows-Win32k", XamlProviders.Win32k),
    ];

    internal static ProviderInfo Provider(XamlProvider provider) =>
        Providers.FirstOrDefault(info => info.Provider == provider) ?? new(provider, provider.ToString(), Guid.Empty);

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
}
