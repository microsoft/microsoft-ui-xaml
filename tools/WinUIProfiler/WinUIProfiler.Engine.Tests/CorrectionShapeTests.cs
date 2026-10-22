using WinUIProfiler.Contracts;
using Xunit;

namespace WinUIProfiler.Engine.Tests;

public sealed class CorrectionShapeTests
{
    [Theory]
    [InlineData("AttemptReference")]
    [InlineData("ChangeCause")]
    [InlineData("XamlPropertyChangedPayload")]
    [InlineData("PropertyObservation")]
    [InlineData("MembershipObservation")]
    [InlineData("FrameChange")]
    [InlineData("SnapshotArtifact")]
    [InlineData("CaptureExtent")]
    [InlineData("SnapshotComparisonKey")]
    [InlineData("CompositionTreeEntry")]
    [InlineData("ResourceClassification")]
    [InlineData("SequenceDomainKey")]
    [InlineData("ListenerDomainInterval")]
    public void Required_correction_representation_exists(string name) =>
        Assert.NotNull(typeof(ObjectKey).Assembly.GetType($"WinUIProfiler.Contracts.{name}"));
}
