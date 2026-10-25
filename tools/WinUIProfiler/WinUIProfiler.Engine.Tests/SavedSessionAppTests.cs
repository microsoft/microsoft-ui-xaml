using System.Diagnostics;
using System.Xml.Linq;
using WinUIProfiler.Engine.Storage;
using Xunit;

namespace WinUIProfiler.Engine.Tests;

public sealed class SavedSessionAppTests
{
    [Fact]
    public void Saved_session_consumers_can_discover_the_validated_public_scope()
    {
        using var fixture = new StoreFixture();
        using (var writer = fixture.Create()) writer.CreateWriter().Commit(fixture.Batch(1, 2));
        using var reader = SessionStore.OpenReadOnly(fixture.Path);
        Assert.NotNull(typeof(SessionStore).GetProperty(nameof(SessionStore.Scope)));
        Assert.Equal(fixture.Scope, reader.Scope);
        Assert.Throws<InvalidOperationException>(() => reader.CreateWriter());
    }

    [Fact]
    public void Application_is_unpacked_selfcontained_and_architecture_explicit()
    {
        var path = Path.GetFullPath(Path.Combine(AppContext.BaseDirectory,
            @"..\..\..\..\WinUIProfiler.App\WinUIProfiler.App.csproj"));
        Assert.True(File.Exists(path), "The approved profiler requires a real WinUIProfiler.App project.");
        var project = XDocument.Load(path);
        string? Property(string name) => project.Descendants(name).FirstOrDefault()?.Value;
        Assert.Equal("true", Property("UseWinUI"));
        Assert.Equal("true", Property("WindowsAppSDKSelfContained"));
        Assert.Equal("None", Property("WindowsPackageType"));
        Assert.Equal("x64;ARM64", Property("Platforms"));
        Assert.Equal("win-x64;win-arm64", Property("RuntimeIdentifiers"));
        Assert.Equal("net10.0-windows10.0.26100.0", Property("TargetFramework"));
    }

    [Theory]
    [InlineData(null)]
    [InlineData("Microsoft.UI.Xaml.dll")]
    [InlineData("Microsoft.UI.Xaml.Resources.19H1.dll")]
    [InlineData("Microsoft.UI.Xaml.resources.common.dll")]
    [InlineData(@"en-US\Microsoft.UI.Xaml.dll.mui")]
    [InlineData("Microsoft.UI.Xaml.Controls.pri")]
    public async Task Local_runtime_validation_requires_core_templates_and_satellite_resources(string? missing)
    {
        var tool = Path.GetFullPath(Path.Combine(AppContext.BaseDirectory, @"..\..\..\.."));
        var directory = Directory.CreateTempSubdirectory("WinUIProfiler.RuntimeValidation.");
        try
        {
            string[] required =
            [
                "Microsoft.UI.Xaml.dll",
                "Microsoft.UI.Xaml.Resources.19H1.dll",
                "Microsoft.UI.Xaml.resources.common.dll",
                @"en-US\Microsoft.UI.Xaml.dll.mui",
                "Microsoft.UI.Xaml.Controls.pri"
            ];
            foreach (var relative in required.Where(file => file != missing))
            {
                var path = Path.Combine(directory.FullName, relative);
                Directory.CreateDirectory(Path.GetDirectoryName(path)!);
                File.WriteAllBytes(path, []);
            }
            var project = Path.Combine(directory.FullName, "Validate.proj");
            new XDocument(new XElement("Project", new XElement("Import",
                new XAttribute("Project", Path.Combine(tool, @"build\LocalWinUI.targets"))))).Save(project);
            using var process = new Process
            {
                StartInfo = new ProcessStartInfo("dotnet")
                {
                    WorkingDirectory = tool,
                    RedirectStandardOutput = true,
                    RedirectStandardError = true,
                    UseShellExecute = false
                }
            };
            foreach (var argument in new[]
            {
                "msbuild", project, "/t:ValidateProfilerLocalWinUI",
                $"/p:ProfilerLocalProduct={directory.FullName}\\", "/v:minimal", "/nologo"
            }) process.StartInfo.ArgumentList.Add(argument);
            Assert.True(process.Start());
            var stdout = process.StandardOutput.ReadToEndAsync();
            var stderr = process.StandardError.ReadToEndAsync();
            using var timeout = new CancellationTokenSource(TimeSpan.FromSeconds(30));
            try { await process.WaitForExitAsync(timeout.Token); }
            catch (OperationCanceledException)
            {
                process.Kill(entireProcessTree: true);
                await process.WaitForExitAsync();
                throw;
            }
            var output = await stdout + await stderr;
            if (missing is null) Assert.True(process.ExitCode == 0, output);
            else Assert.True(process.ExitCode != 0, $"The deployment accepted a Product missing {missing}.\n{output}");
        }
        finally { directory.Delete(recursive: true); }
    }
}
