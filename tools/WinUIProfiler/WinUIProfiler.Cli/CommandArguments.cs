// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Globalization;

namespace WinUIProfiler.Cli;

internal sealed class CommandArguments
{
    private readonly Dictionary<string, string?> options = new(StringComparer.Ordinal);
    private readonly List<string> positionals = [];

    internal IReadOnlyList<string> Positionals => positionals;

    internal static CommandArguments Parse(IEnumerable<string> args, string[] valueOptions, string[]? flagOptions = null)
    {
        var parsed = new CommandArguments();
        using var tokens = args.GetEnumerator();
        var positionalOnly = false;
        while (tokens.MoveNext())
        {
            var token = tokens.Current;
            if (!positionalOnly && token == "--")
            {
                positionalOnly = true;
                continue;
            }

            if (positionalOnly || !token.StartsWith("--", StringComparison.Ordinal))
            {
                parsed.positionals.Add(token);
                continue;
            }

            if (parsed.options.ContainsKey(token))
            {
                throw new ArgumentException($"Option {token} was specified more than once.");
            }

            if (flagOptions?.Contains(token, StringComparer.Ordinal) == true)
            {
                parsed.options.Add(token, null);
            }
            else if (valueOptions.Contains(token, StringComparer.Ordinal))
            {
                if (!tokens.MoveNext() || tokens.Current.StartsWith("--", StringComparison.Ordinal))
                {
                    throw new ArgumentException($"Missing value for {token}.");
                }

                parsed.options.Add(token, tokens.Current);
            }
            else
            {
                throw new ArgumentException($"Unknown option {token}.");
            }
        }

        return parsed;
    }

    internal string? Get(string name) => options.GetValueOrDefault(name);

    internal bool Has(string name) => options.ContainsKey(name);

    internal string Required(string name) => Get(name) is { Length: > 0 } value
        ? value : throw new ArgumentException($"Required option {name} is missing.");

    internal void RequirePositionals(int count)
    {
        if (positionals.Count != count)
        {
            throw new ArgumentException($"Expected {count} positional argument(s), received {positionals.Count}.");
        }
    }

    internal int Integer(string name, int defaultValue, int min = 0, int max = int.MaxValue) =>
        Get(name) is not { } text ? defaultValue
            : int.TryParse(text, NumberStyles.Integer, CultureInfo.InvariantCulture, out var value) && value >= min && value <= max
                ? value : throw new ArgumentException($"{name} must be an integer between {min} and {max}.");
}
