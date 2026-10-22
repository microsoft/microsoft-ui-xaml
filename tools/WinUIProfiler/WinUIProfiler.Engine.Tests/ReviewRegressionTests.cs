using WinUIProfiler.Contracts;
using Xunit;

namespace WinUIProfiler.Engine.Tests;

public sealed class ReviewRegressionTests
{
    private static EventEnvelope Chunk(SnapshotObject[] objects, TreeEdgePayload[] tree) =>
        BoundaryTests.Envelope(new CaptureChunkPayload(CoreTests.Session, 0, CoreTests.Session, 1,
            objects, [], tree, [], [], [], [], [], []));

    [Theory]
    [InlineData(ValueKind.Boolean, ValueUnit.Boolean, null, true)]
    [InlineData(ValueKind.Integer, ValueUnit.Bytes, -1L, null)]
    [InlineData(ValueKind.Integer, ValueUnit.Count, 10L, null)]
    public void R6_Object_sizes_are_nonnegative_numeric_bytes(ValueKind kind, ValueUnit unit, long? integer, bool? boolean)
    {
        var size = new TypedValue(kind, unit, integer, null, boolean, null, DataQuality.Complete);
        Assert.Throws<ArgumentException>(() => EventCodec.Normalize(Chunk([new(CoreTests.Key(), null, null, size)], []),
            new(EventCatalog.RuntimeProviderId, 42, CoreTests.Epoch)));
    }

    [Fact]
    public void R6_Description_size_cannot_be_claimed_measured()
    {
        var resource = new ObjectKey(CoreTests.Process, IdentityDomain.Resource, CoreTests.Epoch, 1);
        Assert.Throws<ArgumentException>(() => EventCodec.Capture(BoundaryTests.Envelope(
            new ResourcePayload(resource, ResourceOperation.Create, CoreTests.Epoch, null, 16,
                SizeProvenance.ApiDescriptionEstimate, null, 2, 2, DataQuality.Complete,
                new(ResourceKind.Unknown, ResourceKindEvidence.Unavailable, "not classified")))));
    }

    [Theory]
    [InlineData(true)]
    [InlineData(false)]
    public void R7_Nested_chunk_scope_cannot_claim_another_root_or_dispatcher(bool wrongRoot)
    {
        var edge = new TreeEdgePayload(null, CoreTests.Key(), RelationshipKind.Root, 0,
            wrongRoot ? CoreTests.Epoch : CoreTests.Session, wrongRoot ? 1UL : 2UL, true);
        Assert.Throws<ArgumentException>(() => EventCodec.Normalize(Chunk([], [edge]),
            new(EventCatalog.RuntimeProviderId, 42, CoreTests.Epoch)));
    }

    [Fact]
    public void R5_Published_retention_edges_cannot_be_mutated_by_the_caller()
    {
        var edge = new OwnershipEdge(CoreTests.Key(), CoreTests.Key(2), "child", OwnershipEvidence.Proven, DataQuality.Complete);
        var edges = new[] { edge };
        var path = new RetentionPath(CoreTests.Key(2), edges, "XamlRoot", OwnershipEvidence.Proven, DataQuality.Complete);
        var page = new QueryPage<RetentionPath>(new(new(CoreTests.Session, CoreTests.Process), new(100, 100), DataQuality.Complete),
            [path], new(10, 0, null, false));
        edges[0] = null!;
        Assert.Equal(edge, page.Items[0].Edges[0]);
    }

    [Fact]
    public void R5_Result_boundary_rejects_invalid_metric_rows()
    {
        var row = new MetricSample(-1, (MetricKind)999,
            new(ValueKind.Boolean, ValueUnit.Percent, null, null, true, null, DataQuality.Complete), new(0, 10), "", "");
        Assert.ThrowsAny<ArgumentException>(() => new QueryPage<MetricSample>(
            new(new(CoreTests.Session, CoreTests.Process), new(100, 100), DataQuality.Complete),
            [row], new(10, 0, null, false)));
    }
}
