using System.Text.Json;
using System.Xml.Linq;
using Xunit;

namespace WinUIProfiler.Core.Tests;

public sealed class DeploymentTests
{
    [Fact]
    public void BothHostRuntimesAreDeclaredForStableCrossArchitectureRestore()
    {
        var project = XDocument.Load(Path.Combine(AppContext.BaseDirectory, "Deployment", "WinUIProfiler.App.csproj"));
        var runtimes = project.Descendants("RuntimeIdentifiers").SingleOrDefault()?.Value.Split(';') ?? [];
        Assert.Contains("win-x64", runtimes);
        Assert.Contains("win-arm64", runtimes);
    }

    [Theory]
    [InlineData("win-x64")]
    [InlineData("win-arm64")]
    public void BothHostRuntimeDependenciesAreLocked(string runtime)
    {
        using var manifest = JsonDocument.Parse(File.ReadAllText(
            Path.Combine(AppContext.BaseDirectory, "Deployment", "packages.lock.json")));
        Assert.Contains(manifest.RootElement.GetProperty("dependencies").EnumerateObject(),
            framework => framework.Name.EndsWith("/" + runtime, StringComparison.Ordinal));
    }
}
