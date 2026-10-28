// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Globalization;

namespace WinUIProfiler.Engine.Events;

// Field values as the CLI and the app show them.
public static class EventFormat
{
    // Element IDs, pointers and handles arrive as Int64; print them in hex like a debugger would.
    public static string Value(string name, object? value) => value switch
    {
        null => "null",
        string text => $"\"{text}\"",
        long number when IsIdentity(name) => $"0x{number:X}",
        IFormattable formattable => formattable.ToString(null, CultureInfo.InvariantCulture),
        _ => value.ToString() ?? ""
    };

    public static string Fields(ProfilerEvent profilerEvent) =>
        string.Join(" ", profilerEvent.Fields.Select(field => $"{field.Name}={Value(field.Name, field.Value)}"));

    private static bool IsIdentity(string name) =>
        name.EndsWith("Id", StringComparison.OrdinalIgnoreCase) ||
        name.EndsWith("Pointer", StringComparison.OrdinalIgnoreCase) ||
        name.EndsWith("Handle", StringComparison.OrdinalIgnoreCase);
}
