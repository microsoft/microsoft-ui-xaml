// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Globalization;
using ModelContextProtocol;

namespace WinUIProfiler.Mcp.Sessions;

/// <summary>Checks tool arguments, so a bad one gets a message the client sees instead of a failed read.</summary>
internal static class ToolArguments
{
    internal static int Range(int value, int min, int max, string name) => value >= min && value <= max
        ? value
        : throw new McpException(string.Create(CultureInfo.InvariantCulture, $"{name} must be from {min} to {max}; it was {value}."));

    internal static int NotNegative(int value, string name) => value >= 0
        ? value
        : throw new McpException(string.Create(CultureInfo.InvariantCulture, $"{name} can't be negative; it was {value}."));

    /// <summary>The text of an optional argument, or null when it's empty.</summary>
    internal static string? Text(string? text) => string.IsNullOrWhiteSpace(text) ? null : text.Trim();

    /// <summary>
    /// The view a tool's view argument names, ignoring case and spaces; the tool's first view when it's empty.
    /// </summary>
    internal static string View(string? view, IReadOnlyList<string> views, string tool)
    {
        var text = Text(view);
        if (text is null)
        {
            return views[0];
        }

        return views.FirstOrDefault(name => SameIgnoringSpaces(name, text))
            ?? throw new McpException($"'{text}' isn't a view of {tool}. Use one of: {string.Join(", ", views)}.");
    }

    /// <summary>The choice a text argument names, ignoring case and spaces; null when it's empty.</summary>
    internal static string? Choice(string? value, IReadOnlyList<string> choices, string name)
    {
        var text = Text(value);
        if (text is null)
        {
            return null;
        }

        return choices.FirstOrDefault(choice => SameIgnoringSpaces(choice, text))
            ?? throw new McpException($"'{text}' isn't a valid {name}. Use one of: {string.Join(", ", choices)}.");
    }

    private static bool SameIgnoringSpaces(string name, string text) => string.Equals(
        name.Replace(" ", "", StringComparison.Ordinal), text.Replace(" ", "", StringComparison.Ordinal),
        StringComparison.OrdinalIgnoreCase);

    /// <summary>
    /// Rejects an argument that the chosen view doesn't read, so a call isn't answered as if the argument had applied.
    /// </summary>
    internal static void OnlyFor(bool given, string name, string tool, string view, params string[] views)
    {
        if (given && !views.Contains(view, StringComparer.Ordinal))
        {
            throw new McpException(views.Length == 1
                ? $"The {view} view of {tool} doesn't read {name}; only the {views[0]} view does."
                : $"The {view} view of {tool} doesn't read {name}; only the {string.Join(" and ", views)} views do.");
        }
    }

    /// <summary>The offset of the next page of a paged result, or null when the page was the last.</summary>
    internal static long? NextOffset(long offset, long returned, long total) =>
        offset + returned < total ? offset + returned : null;

    /// <summary>An address as tools show it, in hex with 0x, or as a decimal number, as the command line reads one.</summary>
    internal static long Address(string text)
    {
        var value = text.Trim();
        long? address = value.StartsWith("0x", StringComparison.OrdinalIgnoreCase)
            ? long.TryParse(value.AsSpan(2), NumberStyles.AllowHexSpecifier, CultureInfo.InvariantCulture, out var hex) ? hex : null
            : long.TryParse(value, NumberStyles.None, CultureInfo.InvariantCulture, out var number) ? number : null;
        return address ?? throw new McpException(
            $"'{text}' isn't an address. Give it in hex with 0x, as the tools show addresses, or as a decimal number.");
    }
}
