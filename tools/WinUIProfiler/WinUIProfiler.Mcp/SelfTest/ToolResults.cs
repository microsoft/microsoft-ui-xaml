// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Text.Json;
using ModelContextProtocol;

namespace WinUIProfiler.Mcp.SelfTest;

/// <summary>Runs tools as the server does, and reads their results as a client sees them: serialized to JSON.</summary>
internal static class ToolResults
{
    internal static JsonElement Run(Func<Task<object>> call) => Json(call().GetAwaiter().GetResult());

    internal static JsonElement Json(object result) => JsonSerializer.SerializeToElement(result, Program.ToolSerializerOptions);

    /// <summary>The message a tool call gives the client when it fails.</summary>
    internal static string Error(Func<Task<object>> call) => Error(() => { call().GetAwaiter().GetResult(); });

    internal static string Error(Action call)
    {
        try
        {
            call();
        }
        catch (McpException error)
        {
            return error.Message;
        }

        throw new SelfTestFailedException("The call failed with a message for the client.");
    }

    /// <summary>A property of an object, or an undefined element when it's missing, as a null property may be.</summary>
    internal static JsonElement Property(this JsonElement element, string name) =>
        element.ValueKind == JsonValueKind.Object && element.TryGetProperty(name, out var value) ? value : default;

    internal static bool IsNull(this JsonElement element, string name) =>
        element.Property(name).ValueKind is JsonValueKind.Null or JsonValueKind.Undefined;

    internal static long Long(this JsonElement element, string name)
    {
        var value = element.Property(name);
        Expect.True(value.ValueKind == JsonValueKind.Number && value.TryGetInt64(out _), $"{name} is an integer; it was {value}.");
        return value.GetInt64();
    }

    internal static double? Double(this JsonElement element, string name)
    {
        var value = element.Property(name);
        if (value.ValueKind is JsonValueKind.Null or JsonValueKind.Undefined)
        {
            return null;
        }

        Expect.True(value.ValueKind == JsonValueKind.Number, $"{name} is a number; it was {value}.");
        return value.GetDouble();
    }

    internal static bool Bool(this JsonElement element, string name)
    {
        var value = element.Property(name);
        Expect.True(value.ValueKind is JsonValueKind.True or JsonValueKind.False, $"{name} is true or false; it was {value}.");
        return value.GetBoolean();
    }

    internal static string? Text(this JsonElement element, string name)
    {
        var value = element.Property(name);
        if (value.ValueKind is JsonValueKind.Null or JsonValueKind.Undefined)
        {
            return null;
        }

        Expect.True(value.ValueKind == JsonValueKind.String, $"{name} is text; it was {value}.");
        return value.GetString();
    }

    internal static IReadOnlyList<JsonElement> Items(this JsonElement element, string name)
    {
        var value = element.Property(name);
        Expect.True(value.ValueKind == JsonValueKind.Array, $"{name} is a list; it was {value}.");
        return [.. value.EnumerateArray()];
    }

    internal static IReadOnlyList<string?> Strings(this JsonElement element, string name) =>
        [.. element.Items(name).Select(item => item.ValueKind == JsonValueKind.Null ? null : item.GetString())];

    /// <summary>Checks that some text contains each of the parts, ignoring case.</summary>
    internal static void Contains(string? text, string what, params string[] parts)
    {
        foreach (var part in parts)
        {
            Expect.True(text?.Contains(part, StringComparison.OrdinalIgnoreCase) == true,
                $"{what} contains \"{part}\"; it was \"{text}\".");
        }
    }

    /// <summary>Checks that one of the notes contains some text.</summary>
    internal static void HasNote(JsonElement result, string text) =>
        Expect.True(result.Strings("notes").Any(note => note?.Contains(text, StringComparison.Ordinal) == true),
            $"A note contains \"{text}\"; the notes were: {string.Join(" | ", result.Strings("notes"))}");

    /// <summary>Checks that none of the notes contains some text.</summary>
    internal static void LacksNote(JsonElement result, string text) =>
        Expect.True(!result.Strings("notes").Any(note => note?.Contains(text, StringComparison.Ordinal) == true),
            $"No note contains \"{text}\"; the notes were: {string.Join(" | ", result.Strings("notes"))}");
}
