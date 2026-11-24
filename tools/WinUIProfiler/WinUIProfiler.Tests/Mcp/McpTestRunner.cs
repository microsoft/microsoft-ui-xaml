// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Diagnostics;

namespace WinUIProfiler.Tests.Mcp;

/// <summary>A named test of the MCP server.</summary>
internal sealed record McpTestCase(string Name, Action<McpTestContext> Run);

/// <summary>
/// Runs the MCP server's tests. Prints one PASS, FAIL or SKIP line per test and a summary, and returns 0 when no test
/// failed.
/// </summary>
internal static class McpTestRunner
{
    private static IEnumerable<McpTestCase> AllTests =>
        ServerTests.Tests
            .Concat(ValueTests.Tests)
            .Concat(SessionToolTests.Tests)
            .Concat(FrameToolTests.Tests).Concat(LaunchToolTests.Tests).Concat(ObjectToolTests.Tests)
            .Concat(QueryTests.Tests);

    public static int Run(TextWriter output)
    {
        var tests = AllTests.ToList();
        var duplicate = tests.GroupBy(test => test.Name, StringComparer.OrdinalIgnoreCase).FirstOrDefault(group => group.Count() > 1);
        if (duplicate is not null)
        {
            output.WriteLine($"FAIL two tests are named '{duplicate.Key}'.");
            return 1;
        }

        int passed = 0, failed = 0, skipped = 0;
        foreach (var test in tests)
        {
            var stopwatch = Stopwatch.StartNew();
            string? failure = null;
            string? skipReason = null;
            using (var context = new McpTestContext())
            {
                try
                {
                    test.Run(context);
                }
                catch (McpTestSkippedException exception)
                {
                    skipReason = exception.Message;
                }
                catch (McpTestFailedException exception)
                {
                    failure = exception.Message;
                }
                catch (Exception exception)
                {
                    failure = exception.ToString();
                }
            }

            if (failure is not null)
            {
                failed++;
                output.WriteLine($"FAIL {test.Name}: {failure}");
            }
            else if (skipReason is not null)
            {
                skipped++;
                output.WriteLine($"SKIP {test.Name}: {skipReason}");
            }
            else
            {
                passed++;
                output.WriteLine($"PASS {test.Name} ({stopwatch.ElapsedMilliseconds} ms)");
            }
        }

        output.WriteLine($"{passed} passed, {failed} failed, {skipped} skipped.");
        return failed == 0 ? 0 : 1;
    }
}

/// <summary>Per-test state: a temporary folder that's deleted when the test ends.</summary>
internal sealed class McpTestContext : IDisposable
{
    private string? directory;

    /// <summary>An empty folder for this test's files, created on first use.</summary>
    public string TempDirectory => directory ??= Directory.CreateDirectory(
        Path.Combine(Path.GetTempPath(), "WinUIProfiler.Tests.Mcp", Guid.NewGuid().ToString("N"))).FullName;

    public void Dispose()
    {
        if (directory is null)
        {
            return;
        }

        try
        {
            Directory.Delete(directory, recursive: true);
        }
        catch (IOException)
        {
        }
        catch (UnauthorizedAccessException)
        {
        }
    }
}

internal static class Expect
{
    public static void True(bool condition, string message)
    {
        if (!condition)
        {
            throw new McpTestFailedException(message);
        }
    }

    public static void Equal<T>(T expected, T actual, string what)
    {
        if (!EqualityComparer<T>.Default.Equals(expected, actual))
        {
            throw new McpTestFailedException($"{what}: expected {Format(expected)}, got {Format(actual)}.");
        }
    }

    /// <summary>Ends the test as skipped. Declared to return an exception so callers can write <c>?? throw Expect.Skip(...)</c>.</summary>
    public static Exception Skip(string reason) => throw new McpTestSkippedException(reason);

    private static string Format<T>(T value) => value switch
    {
        null => "null",
        string text => $"\"{text}\"",
        _ => value.ToString() ?? string.Empty,
    };
}

internal sealed class McpTestFailedException(string message) : Exception(message);

internal sealed class McpTestSkippedException(string message) : Exception(message);
