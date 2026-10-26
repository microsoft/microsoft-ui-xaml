using System.Runtime.Versioning;
using WinUIProfiler.Engine.Target;
using Xunit;

namespace WinUIProfiler.Core.Tests;

[SupportedOSPlatform("windows")]
public class TargetOptionsTests
{
    private static TargetConnectionOptions Options => new(TargetLaunchKind.Attach, "123", "", null, @"C:\Payloads", true);

    [Fact]
    public void Requires_explicit_injection_trust() =>
        Assert.Throws<UnauthorizedAccessException>(() => TargetConnection.ValidateOptions(Options with { TrustLocalBinaries = false }));

    [Theory]
    [InlineData("0")]
    [InlineData("-1")]
    [InlineData("not a PID")]
    public void Rejects_invalid_process_ids(string pid) =>
        Assert.Throws<ArgumentException>(() => TargetConnection.ValidateOptions(Options with { Target = pid }));

    [Fact]
    public void Rejects_attaching_to_the_profiler_itself() =>
        Assert.Throws<ArgumentException>(() => TargetConnection.ValidateOptions(Options with { Target = Environment.ProcessId.ToString() }));

    [Fact]
    public void Process_creation_time_cannot_be_applied_to_a_new_launch() =>
        Assert.Throws<ArgumentException>(() => TargetConnection.ValidateOptions(
            Options with { Kind = TargetLaunchKind.Unpackaged, ExpectedCreationTimeFileTime = 123 }));
}
