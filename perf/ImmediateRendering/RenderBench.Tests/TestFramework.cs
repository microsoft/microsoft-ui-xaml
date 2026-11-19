using System.Runtime.CompilerServices;

namespace RenderBench.Tests;

sealed class TestFailure(string message) : Exception(message);

sealed class TestSkipped(string message) : Exception(message);

static class Check
{
    public static void Expect(bool condition, string message, [CallerArgumentExpression(nameof(condition))] string? expression = null)
    {
        if (!condition)
        {
            throw new TestFailure($"{message} (expected: {expression})");
        }
    }

    public static void Equal<T>(T expected, T actual, string message)
    {
        if (!EqualityComparer<T>.Default.Equals(expected, actual))
        {
            throw new TestFailure($"{message}: expected <{expected}>, actual <{actual}>");
        }
    }

    public static void Near(double expected, double? actual, double tolerance, string message)
    {
        if (actual is null || Math.Abs(expected - actual.Value) > tolerance)
        {
            throw new TestFailure($"{message}: expected {expected}±{tolerance}, actual {actual?.ToString() ?? "null"}");
        }
    }

    public static T NotNull<T>(T? value, string message) where T : class
        => value ?? throw new TestFailure($"{message}: was null");

    public static T NotNull<T>(T? value, string message) where T : struct
        => value ?? throw new TestFailure($"{message}: was null");

    public static void Skip(string reason) => throw new TestSkipped(reason);
}

sealed record TestCase(string Name, Action Body);

static class TestRunner
{
    public static int Run(IEnumerable<TestCase> tests, string? filter)
    {
        int passed = 0, failed = 0, skipped = 0;
        foreach (var test in tests)
        {
            if (filter is not null && !test.Name.Contains(filter, StringComparison.OrdinalIgnoreCase))
            {
                continue;
            }
            try
            {
                test.Body();
                passed++;
                Console.WriteLine($"PASS  {test.Name}");
            }
            catch (TestSkipped skip)
            {
                skipped++;
                Console.WriteLine($"SKIP  {test.Name}: {skip.Message}");
            }
            catch (Exception ex)
            {
                failed++;
                Console.WriteLine($"FAIL  {test.Name}: {(ex is TestFailure ? ex.Message : ex.ToString())}");
            }
        }
        Console.WriteLine($"Summary: Passed={passed} Failed={failed} Skipped={skipped}");
        return failed == 0 ? 0 : 1;
    }
}
