using WinUIProfiler.Contracts;
using Xunit;

namespace WinUIProfiler.Engine.Tests;

public sealed class StreamTests
{
    [Fact]
    public void Normalization_requires_authoritative_matching_source_and_epoch()
    {
        var e = BoundaryTests.Envelope(new ObjectLifetimePayload(CoreTests.Key(), LifetimeOperation.Birth, 1, null, DataQuality.Complete));
        var stream = new ProducerStreamKey(e.Identity.ProviderId, 42, CoreTests.Epoch);
        Assert.Equal(stream, EventCodec.Normalize(e, stream).Stream);
        Assert.Throws<ArgumentException>(() => EventCodec.Normalize(e, new(e.Identity.ProviderId, 99, CoreTests.Epoch)));
        Assert.Throws<ArgumentException>(() => EventCodec.Normalize(e, new(e.Identity.ProviderId, 42, CoreTests.Session)));
    }

    [Fact]
    public void Baseline_chunks_validate_nested_topology_instead_of_silently_accepting_it()
    {
        var badEdge = new TreeEdgePayload(CoreTests.Key(), CoreTests.Key(), RelationshipKind.Child, 0, CoreTests.Session, 1, true);
        var chunk = new CaptureChunkPayload(CoreTests.Session, 0, CoreTests.Session, 1, [], [], [badEdge], [], [], [], [], [], []);
        Assert.Throws<ArgumentException>(() => EventCodec.Capture(BoundaryTests.Envelope(chunk)));
    }
}
