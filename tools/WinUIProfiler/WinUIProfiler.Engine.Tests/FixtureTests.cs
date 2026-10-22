using System.Text;
using System.Text.Json;
using WinUIProfiler.Contracts;
using Xunit;

namespace WinUIProfiler.Engine.Tests;

public sealed class FixtureTests
{
    [Fact]
    public void Fake_clock_advances_only_when_requested()
    {
        var clock = new FakeClock(1_000_000, 50);
        Assert.Equal(50, clock.TimestampQpc);
        Assert.Equal(150, clock.Advance(100));
        Assert.Equal(150, clock.TimestampQpc);
        Assert.ThrowsAny<ArgumentException>(() => clock.Advance(-1));
    }

    [Fact]
    public void Synthetic_builder_is_deterministic_and_covers_all_observation_schemas()
    {
        var first = SyntheticEventBuilder.BuildMinimal();
        var second = SyntheticEventBuilder.BuildMinimal();
        Assert.NotEmpty(first);
        Assert.Equal(first.Select(EventCodec.Encode).Select(Convert.ToBase64String), second.Select(EventCodec.Encode).Select(Convert.ToBase64String));
        var observed = first.Select(e => (e.Identity.ProviderId, e.Identity.EventId)).ToHashSet();
        foreach (var provider in new[] { EventCatalog.RuntimeProviderId, EventCatalog.CollectorProviderId })
        {
            var loss = BoundaryTests.Envelope(new LossPayload(CoreTests.Epoch, new(100, 101), null, "synthetic loss"), provider);
            EventCodec.Capture(loss);
            observed.Add((loss.Identity.ProviderId, loss.Identity.EventId));
        }
        Assert.All(EventCatalog.All, d => Assert.Contains((d.Identity.ProviderId, d.Identity.EventId), observed));
    }

    [Fact]
    public void Minimal_fixture_contains_coherent_lifecycle_frame_tree_resource_and_collector_sequence()
    {
        using var doc = JsonDocument.Parse(File.ReadAllBytes(Path.Combine(AppContext.BaseDirectory, "TestData", "minimal-session.json")));
        Assert.Equal(1, doc.RootElement.GetProperty("schemaVersion").GetInt32());
        Assert.Equal(1_000_000, doc.RootElement.GetProperty("qpcFrequency").GetInt64());
        var actual = doc.RootElement.GetProperty("events").EnumerateArray()
            .Select(e => EventCodec.Decode(Encoding.UTF8.GetBytes(e.GetRawText()))).ToArray();
        var expected = SyntheticEventBuilder.BuildMinimal();
        Assert.Equal(expected.Select(EventCodec.Encode).Select(Convert.ToBase64String), actual.Select(EventCodec.Encode).Select(Convert.ToBase64String));
        Assert.Equal(Enumerable.Range(1, actual.Length).Select(i => (long)i), actual.Select(e => e.IngestOrdinal));
        var submitted = actual.Where(e => e.Identity.Name == "Frame").Select(EventCodec.ReadPayload<FramePayload>).Where(f => f.Outcome == FrameOutcome.Submitted).ToArray();
        Assert.Single(submitted);
        Assert.Equal(1UL, submitted[0].SubmittedFrameId);
        Assert.Contains(actual, e => e.Identity.Name == "TreeEdge");
        Assert.Contains(actual, e => e.Identity.Name == "Resource");
        Assert.Contains(actual, e => e.Identity.ProviderId == EventCatalog.CollectorProviderId && e.ProducerProcessId != e.Process.ProcessId);
    }
}
