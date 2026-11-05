// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Reflection;

namespace WinUIProfiler.Mcp;

/// <summary>The server's name and version, as reported in the MCP handshake.</summary>
internal static class ServerVersion
{
    public const string ServerName = "winui-profiler";

    /// <summary>
    /// The version from the project's <c>VersionPrefix</c>, without build metadata (the <c>+commit</c>
    /// suffix), so it compares equal to the version in <c>plugin.json</c>.
    /// </summary>
    public static string Value { get; } = Resolve();

    private static string Resolve()
    {
        var informational = typeof(ServerVersion).Assembly
            .GetCustomAttribute<AssemblyInformationalVersionAttribute>()?.InformationalVersion;
        if (!string.IsNullOrWhiteSpace(informational))
        {
            var plus = informational.IndexOf('+', StringComparison.Ordinal);
            return plus >= 0 ? informational[..plus] : informational;
        }

        return typeof(ServerVersion).Assembly.GetName().Version?.ToString(3) ?? "0.0.0";
    }
}
