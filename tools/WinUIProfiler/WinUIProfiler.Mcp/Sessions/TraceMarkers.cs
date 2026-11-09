// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Globalization;
using WinUIProfiler.Engine.Events;
using WinUIProfiler.Engine.Launch;

namespace WinUIProfiler.Mcp.Sessions;

/// <summary>
/// The marker providers whose events record_trace reads from a trace with the XAML providers', as the WinUI Profiler app
/// reads them: the marker configuration's, the user's (MarkerConfig.UserPath) when it exists and is valid, else the
/// default's. Error says why the user's couldn't be used.
/// </summary>
internal sealed record TraceMarkers(IReadOnlyList<MarkerProviderSettings> Providers, string? Error = null)
{
    /// <summary>The default marker configuration's providers.</summary>
    internal static TraceMarkers Default => new(MarkerConfig.Default.Providers);

    /// <summary>
    /// The providers of the marker configuration at path, or without one, of the user's when it exists; the default's,
    /// with the error, when that configuration can't be read or isn't valid. As in the app, a mistake in it never stops a
    /// read.
    /// </summary>
    internal static TraceMarkers Load(string? path = null)
    {
        try
        {
            return new(MarkerConfig.Load(path).Providers);
        }
        catch (MarkerConfigException error)
        {
            return Default with { Error = error.Message };
        }
    }

    /// <summary>
    /// What a session file read from a trace depends on of the providers, in a fixed order: each one's GUID, the name its
    /// events are stored under, and whether the launcher's events are read too. A trace is read as it is, whatever the
    /// providers' levels and keywords, which only a live recording uses, so those aren't part of it.
    /// </summary>
    internal IReadOnlyList<string> Key => [.. Providers.Select(Describe).Order(StringComparer.Ordinal)];

    /// <summary>The note a result gives when the user's configuration couldn't be used; null when it could.</summary>
    internal string? Note => Error is null ? null
        : $"The default marker configuration's providers were read instead of the user's, which couldn't be used: {Error}";

    private static string Describe(MarkerProviderSettings settings) => string.Create(CultureInfo.InvariantCulture,
        $"{settings.Provider.Name} {settings.Provider.Id:B}{(settings.Launcher ? " launcher" : "")}");
}
