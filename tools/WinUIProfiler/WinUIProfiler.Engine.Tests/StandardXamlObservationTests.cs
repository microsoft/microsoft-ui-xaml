using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Ingestion;
using WinUIProfiler.Engine.Storage;
using Xunit;

namespace WinUIProfiler.Engine.Tests;

public sealed class StandardXamlObservationTests
{
    private static readonly Guid XamlProvider = new("531a35ab-63ce-4bcf-aa98-f88c7a89e455");

    [Fact]
    public void Ordinary_TAP_can_report_no_private_runtime_epoch_without_fabricating_one()
    {
        var request = Guid.NewGuid();
        var bytes = System.Text.Encoding.UTF8.GetBytes("{\"epoch\":null,\"schemaVersion\":1,\"capabilities\":[]}");
        var frame = TapCodec.Encode(TapMessageType.CapabilitiesResult, request,
            new CapabilitiesBody(CoreTests.Epoch, 1, []));
        Array.Resize(ref frame, TapProtocol.HeaderBytes + bytes.Length);
        System.Buffers.Binary.BinaryPrimitives.WriteUInt32LittleEndian(frame, (uint)bytes.Length);
        bytes.CopyTo(frame, TapProtocol.HeaderBytes);
        var body = Assert.IsType<CapabilitiesBody>(TapCodec.Decode(frame).Body);
        Assert.Equal("null", System.Text.Json.JsonSerializer.Serialize(body.Epoch));
    }

    [Fact]
    public void Standard_XAML_is_retained_as_source_evidence_without_invented_frame_semantics()
    {
        var scope = new SessionScope(CoreTests.Session, new(42, 123, CoreTests.Session));
        var record = new NativeRecord(XamlProvider, "Layout/Start", 4, 0x40,
            scope.Process.ProcessId, 17, 1234, Guid.NewGuid(), new Dictionary<string, object>(),
            new byte[] { 1, 2, 3, 4 }, descriptorId: 57, descriptorVersion: 2, opcode: 1, task: 23);
        var decoded = new EventDecoder(scope, new(10, 10)).Decode(record);
        Assert.NotNull(decoded);
        Assert.Equal("ObservedXaml", decoded.Event.Envelope.Identity.Name);
        Assert.Null(decoded.Event.Envelope.SourceSequence);
        Assert.Equal(scope.Process, decoded.Event.Envelope.Process);
        Assert.Equal(1234, decoded.Provenance.OriginalTimestampQpc);
        Assert.Null(decoded.Provenance.OriginalTarget.CreationTimeFileTime);
        Assert.Equal(record.ActivityId, decoded.Event.Envelope.ActivityId);
        Assert.Equal(record.ThreadId, decoded.Event.Envelope.ThreadId);
        var payload = EventCodec.ReadPayload<ObservedXamlPayload>(decoded.Event.Envelope);
        Assert.Equal((ushort)57, payload.EventId);
        Assert.Equal((byte)2, payload.Version);
        Assert.Equal((byte)4, payload.Level);
        Assert.Equal(0x40UL, payload.Keywords);
        Assert.Equal((byte)1, payload.Opcode);
        Assert.Equal((ushort)23, payload.Task);
        Assert.Equal(CoverageState.Partial, payload.Quality.Coverage);
        using var json = System.Text.Json.JsonDocument.Parse(decoded.Event.Envelope.OwnedPayload);
        Assert.Equal(XamlProvider, json.RootElement.GetProperty("providerId").GetGuid());
        Assert.Equal("Layout/Start", json.RootElement.GetProperty("name").GetString());
        Assert.Equal(new byte[] { 1, 2, 3, 4 }, json.RootElement.GetProperty("rawPayload").GetBytesFromBase64());
    }

    [Fact]
    public void Standard_XAML_from_another_process_is_not_attributed_to_selected_target()
    {
        var scope = new SessionScope(CoreTests.Session, new(42, 123, CoreTests.Session));
        var record = new NativeRecord(XamlProvider, "Layout/Start", 4, 0x40,
            scope.Process.ProcessId + 1, 17, 1234, null, new Dictionary<string, object>());
        Assert.Null(new EventDecoder(scope, new(10, 10)).Decode(record));
    }

    [Theory]
    [InlineData(0)]
    [InlineData(257)]
    [InlineData(NativeRecord.MaximumWireBytes)]
    public void Standard_XAML_source_bytes_survive_real_SQLite_commit_and_reopen(int length)
    {
        using var fixture = new StoreFixture();
        var bytes = Enumerable.Range(0, length).Select(i => (byte)(i % 251)).ToArray();
        var record = new NativeRecord(XamlProvider, "SourceOnly", 5, 0x1044, 42, 17, 1234, null,
            new Dictionary<string, object>(), bytes);
        var decoded = new EventDecoder(fixture.Scope, new(10, 1_000_000)).Decode(record)!;
        using (var store = fixture.Create())
            store.CreateWriter().Commit(new([decoded.Event], [], fixture.Info with {
                Status = SessionStatus.Stopped, StopReason = "Source observation qualification completed." },
                StoreFixture.Partial, Provenance: [new(1, decoded.Provenance)]));
        using var reopened = SessionStore.OpenReadOnly(fixture.Path);
        Assert.Equal(1, reopened.Watermark.IngestOrdinal);
        using var raw = fixture.Raw();
        var envelope = EventCodec.Decode((byte[])StoreFixture.Scalar(raw, "SELECT Envelope FROM SourceEvents")!);
        var payload = EventCodec.ReadPayload<ObservedXamlPayload>(envelope);
        Assert.Equal(bytes, payload.RawPayload.ToArray());
        Assert.Equal(0L, StoreFixture.Scalar(raw, "SELECT count(*) FROM FrameSubmissions"));
        Assert.Equal(0L, StoreFixture.Scalar(raw, "SELECT count(*) FROM ObjectLifetimes"));
    }

    [Fact]
    public void Source_observation_cannot_claim_complete_coverage_or_an_unrelated_provider()
    {
        var payload = new ObservedXamlPayload(XamlProvider, "SourceOnly", 1, 0, 4, 0x40, 1, 0,
            ReadOnlyMemory<byte>.Empty, new(MeasurementKind.Measured, CoverageState.Partial, "Source evidence only."));
        var process = new ProcessInstanceKey(42, 123, CoreTests.Session);
        Assert.Throws<ArgumentException>(() => EventCodec.EncodePayload(payload with {
            Quality = new(MeasurementKind.Measured, CoverageState.Complete, null) }, process));
        Assert.Throws<ArgumentException>(() => EventCodec.EncodePayload(payload with { ProviderId = Guid.NewGuid() }, process));
        Assert.Throws<ArgumentException>(() => EventCodec.EncodePayload(payload with {
            RawPayload = new byte[NativeRecord.MaximumWireBytes + 1] }, process));
    }
}
