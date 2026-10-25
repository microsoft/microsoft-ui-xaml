using System.Reflection;
using Xunit;

namespace WinUIProfiler.Engine.Tests;

public sealed class FoundationTests
{
    [Theory]
    [InlineData("ProcessInstanceKey")]
    [InlineData("ObjectKey")]
    [InlineData("EventEnvelope")]
    [InlineData("DataQuality")]
    [InlineData("IProfilerQueryService")]
    [InlineData("TapProtocol")]
    public void Required_contract_is_present(string name)
    {
        Assert.NotNull(Assembly.Load("WinUIProfiler.Contracts").GetType($"WinUIProfiler.Contracts.{name}"));
    }
}
