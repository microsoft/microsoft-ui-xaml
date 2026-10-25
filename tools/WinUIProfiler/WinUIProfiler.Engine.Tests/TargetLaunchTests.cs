using System.Diagnostics;
using System.Runtime.Versioning;
using WinUIProfiler.Engine.Target;
using Xunit;

namespace WinUIProfiler.Engine.Tests;

[SupportedOSPlatform("windows")]
public sealed class TargetLaunchTests
{
    [Fact]
    public async Task Unpackaged_launch_passes_arguments_and_uses_executable_directory()
    {
        var executable = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.System),
            @"WindowsPowerShell\v1.0\powershell.exe");
        using var process = TargetLauncher.LaunchUnpackaged(executable, "-NoProfile -NonInteractive -Command \"exit 17\"", null);
        try
        {
            await process.WaitForExitAsync().WaitAsync(TimeSpan.FromSeconds(20));
            Assert.Equal(17, process.ExitCode);
            Assert.Equal(Path.GetDirectoryName(executable), process.StartInfo.WorkingDirectory);
        }
        finally
        {
            if (!process.HasExited) process.Kill();
        }
    }

    [Fact]
    public void Missing_executable_is_an_explicit_error() =>
        Assert.Throws<FileNotFoundException>(() =>
            TargetLauncher.LaunchUnpackaged(@"C:\WinUIProfiler-missing-target.exe", "", null));

    [Theory]
    [InlineData("")]
    [InlineData("Family")]
    [InlineData("!Application")]
    [InlineData("Family!")]
    [InlineData("Family!App!Other")]
    public void Invalid_packaged_application_identity_is_rejected_before_activation(string identity) =>
        Assert.Throws<ArgumentException>(() => TargetLauncher.LaunchPackaged(identity, ""));
}
