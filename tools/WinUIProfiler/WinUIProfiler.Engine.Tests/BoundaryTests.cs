using System.Buffers.Binary;
using System.Text;
using System.Text.Json;
using System.Text.Json.Serialization;
using WinUIProfiler.Contracts;
using Xunit;

namespace WinUIProfiler.Engine.Tests;

public sealed class BoundaryTests
{
    private static readonly JsonSerializerOptions Wire = new(JsonSerializerDefaults.Web)
    {
        Converters = { new JsonStringEnumConverter(allowIntegerValues: false) }
    };
    internal static EventEnvelope Envelope(IEventPayload body, Guid? provider = null)
    {
        var definition = EventCatalog.All.First(d => d.PayloadType == body.GetType() && (provider is null || d.Identity.ProviderId == provider));
        return new(CoreTests.Session, CoreTests.Process, provider == EventCatalog.CollectorProviderId ? 99u : 42u,
            7, 100, 1, 1, null, definition.Identity, JsonSerializer.SerializeToUtf8Bytes(body, body.GetType(), Wire));
    }
    internal static EventEnvelope ReplacePayload(EventEnvelope e, string payload) =>
        new(e.SessionId, e.Process, e.ProducerProcessId, e.ThreadId, e.TimestampQpc, e.IngestOrdinal, e.SourceSequence, e.ActivityId, e.Identity, Encoding.UTF8.GetBytes(payload));

    [Fact]
    public void Owned_codec_roundtrip_preserves_null_birth_and_payload()
    {
        var original = Envelope(new ObjectLifetimePayload(CoreTests.Key(), LifetimeOperation.Birth, 1, 0x1234, DataQuality.Complete));
        var captured = EventCodec.Capture(original);
        var bytes = EventCodec.Encode(captured);
        var restored = EventCodec.Decode(bytes);
        bytes.AsSpan().Clear();
        Assert.Equal(original.Process, restored.Process);
        Assert.Equal(original.Identity, restored.Identity);
        Assert.Equal(original.OwnedPayload.ToArray(), restored.OwnedPayload.ToArray());
    }

    [Theory]
    [InlineData("{}")]
    [InlineData("{\"object\":null,\"operation\":\"Birth\",\"typeId\":1,\"address\":null,\"quality\":null}")]
    [InlineData("{\"object\":{},\"operation\":\"Invented\",\"typeId\":null,\"address\":null,\"quality\":null}")]
    public void Codec_rejects_missing_and_malformed_required_payloads(string json)
    {
        var e = Envelope(new ObjectLifetimePayload(CoreTests.Key(), LifetimeOperation.Birth, 1, null, DataQuality.Complete));
        Assert.ThrowsAny<JsonException>(() => EventCodec.Capture(ReplacePayload(e, json)));
    }

    [Fact]
    public void Codec_rejects_unknown_version_and_cross_process_objects()
    {
        var e = Envelope(new ObjectLifetimePayload(CoreTests.Key(), LifetimeOperation.Birth, 1, null, DataQuality.Complete));
        var unsupported = new EventEnvelope(e.SessionId, e.Process, e.ProducerProcessId, e.ThreadId, e.TimestampQpc, e.IngestOrdinal,
            e.SourceSequence, e.ActivityId, new(e.Identity.ProviderId, e.Identity.Name, e.Identity.EventId, 2, e.Identity.Keywords), e.OwnedPayload);
        Assert.Throws<NotSupportedException>(() => EventCodec.Capture(unsupported));
        var foreign = new ObjectKey(new(42, null, CoreTests.Epoch), IdentityDomain.XamlCore, CoreTests.Epoch, 1);
        Assert.Throws<ArgumentException>(() => EventCodec.Capture(Envelope(new ObjectLifetimePayload(foreign, LifetimeOperation.Birth, 1, null, DataQuality.Complete))));
    }

    [Theory]
    [InlineData(0UL, FrameOutcome.Submitted)]
    [InlineData(1UL, FrameOutcome.Attempt)]
    [InlineData(1UL, FrameOutcome.StandaloneCommit)]
    public void Only_successful_submissions_have_nonzero_frame_ids(ulong submitted, FrameOutcome outcome)
    {
        Assert.Throws<ArgumentException>(() => EventCodec.Capture(Envelope(
            new FramePayload(new(CoreTests.Epoch, 1, CoreTests.Session, 1), submitted, outcome, null))));
    }

    [Fact]
    public void Missing_metric_is_not_zero_and_estimated_partial_roundtrips()
    {
        var quality = new DataQuality(MeasurementKind.Estimated, CoverageState.Partial, "sample interval has loss");
        var sample = new MetricPayload(MetricKind.CpuUtilization,
            new(ValueKind.Number, ValueUnit.Percent, null, 3.5, null, null, quality), new(0, 100), "OS process time", "1", null);
        var e = EventCodec.Decode(EventCodec.Encode(Envelope(sample, EventCatalog.CollectorProviderId)));
        Assert.Equal(99u, e.ProducerProcessId);
        Assert.Equal(42u, e.Process.ProcessId);
        var invalid = sample with { Value = new(ValueKind.Missing, ValueUnit.Percent, 0, null, null, null,
            new(MeasurementKind.Unavailable, CoverageState.Unavailable, "not supported")) };
        Assert.Throws<ArgumentException>(() => EventCodec.Capture(Envelope(invalid, EventCatalog.CollectorProviderId)));
    }

    public static IEnumerable<object[]> Commands()
    {
        yield return [TapMessageType.Handshake, new HandshakeBody(CoreTests.Session, CoreTests.Process, 1, Convert.ToBase64String(new byte[32]))];
        yield return [TapMessageType.Capabilities, new CapabilitiesRequestBody(1)];
        yield return [TapMessageType.CapabilitiesResult, new CapabilitiesBody(CoreTests.Epoch, 1, [new(Capability.LightweightSnapshot, 1, DataQuality.Complete)])];
        yield return [TapMessageType.Rundown, new RundownBody(CoreTests.Session, null, null)];
        yield return [TapMessageType.LightweightSnapshot, new LightweightSnapshotBody(CoreTests.Session, CoreTests.Epoch, 1)];
        yield return [TapMessageType.OwnershipGraph, new OwnershipGraphBody(CoreTests.Session, 100, 200)];
        yield return [TapMessageType.Cancel, new CancelBody(CoreTests.Session)];
        yield return [TapMessageType.Disconnect, new DisconnectBody("finished")];
        yield return [TapMessageType.Acknowledgement, new AcknowledgementBody(CommandStatus.Completed, CoreTests.Session, null)];
    }

    [Theory]
    [MemberData(nameof(Commands))]
    public void All_typed_commands_roundtrip(TapMessageType type, ITapBody body)
    {
        var bytes = TapCodec.Encode(type, CoreTests.Session, body);
        var message = TapCodec.Decode(bytes);
        Assert.Equal(type, message.Header.Type);
        Assert.Equal(CoreTests.Session, message.Header.RequestId);
        Assert.Equal(1u, message.Header.SchemaVersion);
        Assert.Equal(body.GetType(), message.Body.GetType());
        Assert.Equal(JsonSerializer.Serialize(body, body.GetType(), Wire), JsonSerializer.Serialize(message.Body, message.Body.GetType(), Wire));
        Assert.Equal(bytes.Length - TapProtocol.HeaderBytes, (int)BinaryPrimitives.ReadUInt32LittleEndian(bytes));
        Assert.Equal(Convert.FromHexString("10000000000000000000000000000001"), bytes.AsSpan(8, 16).ToArray());
    }

    [Fact]
    public void Protocol_rejects_versions_truncation_excess_length_and_type_mismatch()
    {
        var bytes = TapCodec.Encode(TapMessageType.Disconnect, CoreTests.Session, new DisconnectBody("done"));
        BinaryPrimitives.WriteUInt16LittleEndian(bytes.AsSpan(4), 2);
        Assert.Throws<NotSupportedException>(() => TapCodec.Decode(bytes));
        BinaryPrimitives.WriteUInt16LittleEndian(bytes.AsSpan(4), 1);
        BinaryPrimitives.WriteUInt32LittleEndian(bytes.AsSpan(24), 2);
        Assert.Throws<NotSupportedException>(() => TapCodec.Decode(bytes));
        BinaryPrimitives.WriteUInt32LittleEndian(bytes.AsSpan(24), 1);
        Assert.Throws<ArgumentException>(() => TapCodec.Decode(bytes.AsSpan(0, bytes.Length - 1)));
        BinaryPrimitives.WriteUInt32LittleEndian(bytes, TapProtocol.MaximumBodyBytes + 1u);
        Assert.Throws<ArgumentException>(() => TapCodec.Decode(bytes));
        Assert.Throws<ArgumentException>(() => TapCodec.Encode(TapMessageType.Cancel, CoreTests.Session, new DisconnectBody("wrong type")));
        Assert.Throws<ArgumentException>(() => TapCodec.Encode(TapMessageType.Handshake, CoreTests.Session, new HandshakeBody(CoreTests.Session, CoreTests.Process, 1, "bad nonce")));
    }

    [Fact]
    public void Queries_are_typed_paged_cancellable_and_have_context()
    {
        Assert.Equal(12, typeof(IProfilerQueryService).GetMethods().Length);
        foreach (var method in typeof(IProfilerQueryService).GetMethods())
        {
            Assert.Equal(typeof(CancellationToken), method.GetParameters()[1].ParameterType);
            Assert.False(method.GetParameters()[1].IsOptional);
            var result = method.ReturnType.GenericTypeArguments[0];
            Assert.Equal(typeof(ResultContext), result.GetProperty("Context")!.PropertyType);
        }
    }

    [Theory]
    [InlineData(0, 0)]
    [InlineData(4097, 0)]
    [InlineData(1, -1)]
    [InlineData(1, 1)]
    public void Page_limits_and_stable_watermark_are_required(int limit, long offset) =>
        Assert.Throws<ArgumentException>(() => QueryValidation.Validate(
            new GetFramesRequest(new(CoreTests.Session, CoreTests.Process), new(0, 100), new(limit, offset)), default));

    [Fact]
    public void Query_scope_filters_snapshot_comparisons_and_cancellation_are_validated()
    {
        var scope = new SessionScope(CoreTests.Session, CoreTests.Process);
        QueryValidation.Validate(new GetFramesRequest(scope, new(0, 100), new(10, 10, new(20, 100))), default);
        Assert.Throws<ArgumentException>(() => QueryValidation.Validate(new GetSessionRequest(default), default));
        Assert.Throws<ArgumentException>(() => QueryValidation.Validate(new CompareSnapshotsRequest(scope, CoreTests.Session, [], new(10)), default));
        Assert.Throws<ArgumentException>(() => QueryValidation.Validate(new CompareSnapshotsRequest(scope, CoreTests.Session, [CoreTests.Session], new(10)), default));
        Assert.Throws<ArgumentException>(() => QueryValidation.Validate(new GetRetentionPathsRequest(scope, CoreTests.Session, CoreTests.Key(), 0, new(10)), default));
        using var cts = new CancellationTokenSource();
        cts.Cancel();
        Assert.Throws<OperationCanceledException>(() => QueryValidation.Validate(new GetSessionRequest(scope), cts.Token));
    }
}
