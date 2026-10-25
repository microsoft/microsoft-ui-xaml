using System.Text;
using System.Text.Json;
using WinUIProfiler.Contracts;
using Xunit;

namespace WinUIProfiler.Engine.Tests;

public sealed class CoreTests
{
    internal static readonly Guid Session = Guid.Parse("10000000-0000-0000-0000-000000000001");
    internal static readonly Guid Epoch = Guid.Parse("20000000-0000-0000-0000-000000000001");
    internal static ProcessInstanceKey Process => new(42, null, Session);
    internal static ObjectKey Key(ulong id = 1) => new(Process, IdentityDomain.XamlCore, Epoch, id);

    [Fact]
    public void Pid_reuse_and_unknown_birth_remain_distinct()
    {
        Assert.NotEqual(Process, new ProcessInstanceKey(42, null, Epoch));
        Assert.NotEqual(new ProcessInstanceKey(42, 100, Session), new ProcessInstanceKey(42, 101, Session));
        Assert.Null(Process.CreationTimeFileTime);
        Assert.Contains("\"CreationTimeFileTime\":null", JsonSerializer.Serialize(Process));
    }

    [Fact]
    public void Lifetime_ids_and_epochs_not_addresses_define_identity()
    {
        Assert.NotEqual(Key(), Key(2));
        Assert.NotEqual(Key(), new ObjectKey(Process, IdentityDomain.XamlCore, Session, 1));
        Assert.NotEqual(Key(), new ObjectKey(Process, IdentityDomain.CompositionVisual, Epoch, 1));
        var oldObject = new ObjectLifetimePayload(Key(), LifetimeOperation.Birth, 1, 0x1234, DataQuality.Complete);
        var reusedAddress = new ObjectLifetimePayload(Key(2), LifetimeOperation.Birth, 1, 0x1234, DataQuality.Complete);
        var oldCaptured = EventCodec.ReadPayload<ObjectLifetimePayload>(BoundaryTests.Envelope(oldObject));
        var newCaptured = EventCodec.ReadPayload<ObjectLifetimePayload>(BoundaryTests.Envelope(reusedAddress));
        Assert.Equal(oldCaptured.Address, newCaptured.Address);
        Assert.NotEqual(oldCaptured.Object, newCaptured.Object);
    }

    [Theory]
    [InlineData(0, null)]
    [InlineData(42, 0L)]
    [InlineData(42, -1L)]
    public void Invalid_process_fields_are_rejected(uint pid, long? birth) =>
        Assert.ThrowsAny<ArgumentException>(() => new ProcessInstanceKey(pid, birth, Session));

    [Fact]
    public void Empty_instance_is_rejected() =>
        Assert.ThrowsAny<ArgumentException>(() => new ProcessInstanceKey(42, null, Guid.Empty));

    [Fact]
    public void Invalid_object_fields_are_rejected()
    {
        Assert.ThrowsAny<ArgumentException>(() => new ObjectKey(default, IdentityDomain.XamlCore, Epoch, 1));
        Assert.ThrowsAny<ArgumentException>(() => new ObjectKey(Process, (IdentityDomain)99, Epoch, 1));
        Assert.ThrowsAny<ArgumentException>(() => new ObjectKey(Process, IdentityDomain.XamlCore, Guid.Empty, 1));
        Assert.ThrowsAny<ArgumentException>(() => Key(0));
    }

    [Fact]
    public void Estimated_and_partial_are_independent()
    {
        var quality = new DataQuality(MeasurementKind.Estimated, CoverageState.Partial, "late attach");
        Assert.Equal(MeasurementKind.Estimated, quality.Kind);
        Assert.Equal(CoverageState.Partial, quality.Coverage);
    }

    [Fact]
    public void Invalid_quality_and_unexplained_missing_data_are_rejected()
    {
        Assert.ThrowsAny<ArgumentException>(() => new DataQuality((MeasurementKind)99, CoverageState.Complete, null));
        Assert.ThrowsAny<ArgumentException>(() => new DataQuality(MeasurementKind.Measured, (CoverageState)99, null));
        Assert.ThrowsAny<ArgumentException>(() => new DataQuality(MeasurementKind.Unavailable, CoverageState.Unavailable, null));
    }

    [Fact]
    public void Ranges_and_qpc_conversion_use_session_frequency()
    {
        Assert.ThrowsAny<ArgumentException>(() => new TimeRange(20, 19));
        Assert.ThrowsAny<ArgumentException>(() => new TimeRange(-1, 20));
        Assert.Equal(TimeSpan.FromMilliseconds(250), QpcTime.Elapsed(new(7, 250_007), 1_000_000));
        Assert.Equal(TimeSpan.FromSeconds(1), QpcTime.Elapsed(new(0, 24_000_000), 24_000_000));
        Assert.ThrowsAny<ArgumentException>(() => QpcTime.Elapsed(new(0, 1), 0));
        Assert.Throws<OverflowException>(() => QpcTime.Elapsed(new(0, long.MaxValue), 1));
    }

    [Fact]
    public void Envelope_owns_recycled_source_and_preserves_collector_pid()
    {
        var source = Encoding.UTF8.GetBytes("{\"sample\":1}");
        var envelope = new EventEnvelope(Session, Process, 99, 7, 100, 1, 1, null,
            new EventIdentity(Epoch, "Test", 1, 1, 1), source);
        source[0] = 0;
        Assert.Equal((byte)'{', envelope.OwnedPayload.Span[0]);
        Assert.True(System.Runtime.InteropServices.MemoryMarshal.TryGetArray(envelope.OwnedPayload, out var exposedCopy));
        exposedCopy.Array![exposedCopy.Offset] = 0;
        Assert.Equal((byte)'{', envelope.OwnedPayload.Span[0]);
        Assert.Equal(99u, envelope.ProducerProcessId);
        Assert.Equal(42u, envelope.Process.ProcessId);
    }

    [Fact]
    public void Required_envelope_and_event_fields_are_checked()
    {
        Assert.ThrowsAny<ArgumentException>(() => new EventIdentity(Guid.Empty, "Event", 1, 1, 1));
        Assert.ThrowsAny<ArgumentException>(() => new EventIdentity(Epoch, "", 1, 1, 1));
        Assert.ThrowsAny<ArgumentException>(() => new EventIdentity(Epoch, "Event", 0, 1, 1));
        Assert.ThrowsAny<ArgumentException>(() => new EventIdentity(Epoch, "Event", 1, 0, 1));
        Assert.ThrowsAny<ArgumentException>(() => new EventEnvelope(Session, default, 99, 1, 1, 1, null, null,
            new EventIdentity(Epoch, "Event", 1, 1, 1), "{}"u8.ToArray()));
    }
}
