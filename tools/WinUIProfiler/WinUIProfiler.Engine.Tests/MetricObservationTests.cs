using WinUIProfiler.Contracts;
using System.Text.Json;
using WinUIProfiler.Engine.Storage;
using WinUIProfiler.Engine.Ingestion;
using WinUIProfiler.Engine.Processing;
using Xunit;

namespace WinUIProfiler.Engine.Tests;

public sealed class MetricObservationTests
{
    [Fact]
    public void Absent_source_observation_does_not_change_legacy_payload_bytes()
    {
        using var fixture = new StoreFixture();
        var envelope = fixture.Batch(1, 1).Sources[0].Envelope;
        using var json = JsonDocument.Parse(envelope.OwnedPayload);
        Assert.False(json.RootElement.TryGetProperty("sourceObservation", out _));
        Assert.Null(EventCodec.ReadPayload<MetricPayload>(envelope).SourceObservation);
    }

    [Fact]
    public void Native_source_clocks_remain_distinct_from_session_qpc()
    {
        var boot = Observation();
        boot.Validate();
        var filetime = boot with { Clock = MetricClock.UtcFileTime100ns, Begin = 134000000000000000, End = 134000000000100000 };
        filetime.Validate();
        Assert.NotEqual(boot.Clock, filetime.Clock);
        Assert.Equal(10_000_000, filetime.Frequency);
        Assert.Equal(100ul, filetime.AcquisitionBegin);
        Assert.Equal(200ul, filetime.AcquisitionEnd);
    }

    [Fact]
    public void Filetime_rollback_is_preserved_only_as_counter_reset()
    {
        var reset = Observation() with { Clock = MetricClock.UtcFileTime100ns, Begin = 201, End = 200, Status = MetricSampleStatus.CounterReset };
        reset.Validate();
        Assert.Equal(201ul, reset.Begin);
        Assert.Throws<ArgumentException>(() => (reset with { Status = MetricSampleStatus.Measured }).Validate());
        Assert.Throws<ArgumentException>(() => (reset with { Clock = MetricClock.Boot100ns }).Validate());
    }

    [Theory]
    [InlineData(0)]
    [InlineData(1)]
    [InlineData(2)]
    [InlineData(3)]
    [InlineData(4)]
    [InlineData(5)]
    [InlineData(6)]
    public void Native_statuses_are_retained_without_replacing_missing_values_with_zero(int status)
    {
        var value = Observation() with { Status = (MetricSampleStatus)status };
        value.Validate();
        Assert.Equal(status, (int)value.Status);
    }

    [Theory]
    [InlineData(0)]
    [InlineData(1)]
    [InlineData(2)]
    [InlineData(3)]
    [InlineData(4)]
    [InlineData(5)]
    [InlineData(6)]
    [InlineData(7)]
    [InlineData(8)]
    public void Invalid_clock_status_interval_and_error_domains_are_rejected(int fault)
    {
        var valid = Observation();
        var invalid = fault switch
        {
            0 => valid with { Clock = (MetricClock)1 },
            1 => valid with { Frequency = 1000 },
            2 => valid with { Begin = 201 },
            3 => valid with { End = ulong.MaxValue },
            4 => valid with { Status = (MetricSampleStatus)7 },
            5 => valid with { AcquisitionBegin = 201 },
            6 => valid with { AcquisitionEnd = ulong.MaxValue },
            7 => valid with { NativeError = 5, ErrorDomain = MetricErrorDomain.None },
            _ => valid with { ErrorDomain = (MetricErrorDomain)255 }
        };
        Assert.Throws<ArgumentException>(invalid.Validate);
    }

    internal static MetricSourceObservation Observation() => new(
        MetricClock.Boot100ns, 10_000_000, 100, 200, MetricSampleStatus.Measured,
        100, 200, 0, MetricErrorDomain.None, 0);

    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    public void Invalid_original_source_clock_cannot_enter_event_or_query_contract(bool query)
    {
        using var fixture = new StoreFixture();
        var invalid = Observation() with { Frequency = 1000 };
        var batch = fixture.Batch(1, 1);
        var payload = EventCodec.ReadPayload<MetricPayload>(batch.Sources[0].Envelope) with { SourceObservation = invalid };
        var row = (MetricSample)batch.Changes[0].Value with { SourceObservation = invalid };
        var context = new ResultContext(fixture.Scope, new(1, 10), StoreFixture.Partial);
        if (query)
            Assert.Throws<ArgumentException>(() => QueryResultValidation.Freeze(row, context));
        else
            Assert.Throws<ArgumentException>(() => EventCodec.EncodePayload(payload, fixture.Scope.Process));
    }

    [Theory]
    [InlineData(1)]
    [InlineData(2)]
    [InlineData(4)]
    [InlineData(5)]
    [InlineData(6)]
    public void Nonmeasured_source_status_cannot_claim_a_numeric_measurement(int status)
    {
        using var fixture = new StoreFixture();
        var payload = EventCodec.ReadPayload<MetricPayload>(fixture.Batch(1, 1).Sources[0].Envelope)
            with { SourceObservation = Observation() with { Status = (MetricSampleStatus)status } };
        Assert.Throws<ArgumentException>(() => EventCodec.EncodePayload(payload, fixture.Scope.Process));
    }

    [Fact]
    public async Task Original_clock_and_error_provenance_survive_envelope_sqlite_and_reopen()
    {
        using var fixture = new StoreFixture();
        var observation = Observation() with
        {
            Clock = MetricClock.UtcFileTime100ns,
            Begin = 134000000000100000,
            End = 134000000000000000,
            Status = MetricSampleStatus.CounterReset,
            NativeError = 5,
            ErrorDomain = MetricErrorDomain.Win32,
            PdhError = 0xC0000BB8
        };
        var missing = new TypedValue(ValueKind.Missing, ValueUnit.Bytes, null, null, null, null,
            new(MeasurementKind.Unavailable, CoverageState.Unavailable, "Native counter reset."));
        var original = fixture.Batch(1, 1);
        var source = original.Sources[0];
        var payload = EventCodec.ReadPayload<MetricPayload>(source.Envelope) with { Value = missing, SourceObservation = observation };
        var envelope = new EventEnvelope(fixture.Scope.SessionId, fixture.Scope.Process, 99, 7, 10, 1, 1,
            null, source.Envelope.Identity, EventCodec.EncodePayload(payload, fixture.Scope.Process));
        var roundtrip = EventCodec.Decode(EventCodec.Encode(envelope));
        Assert.Equal(observation, EventCodec.ReadPayload<MetricPayload>(roundtrip).SourceObservation);
        var row = (MetricSample)original.Changes[0].Value with { Value = missing, SourceObservation = observation };
        using (var store = fixture.Create())
            store.CreateWriter().Commit(original with
            {
                Sources = [EventCodec.Normalize(roundtrip, source.Stream)],
                Changes = [new(1, row)]
            });
        using var reopened = SessionStore.OpenReadOnly(fixture.Path);
        var persisted = Assert.Single((await reopened.Queries.GetMetrics(fixture.Metrics(), default)).Items);
        Assert.Equal(observation, persisted.SourceObservation);
        Assert.Equal(new TimeRange(9, 10), persisted.Interval);
        Assert.Equal(10, persisted.TimestampQpc);
    }

    [Fact]
    public async Task Bounded_owner_projects_metric_source_clock_without_using_it_as_session_qpc()
    {
        using var fixture = new StoreFixture();
        var source = fixture.Batch(1, 1).Sources[0];
        var observation = Observation() with { Clock = MetricClock.UtcFileTime100ns, Begin = 134000000000000000, End = 134000000000100000 };
        var payload = EventCodec.ReadPayload<MetricPayload>(source.Envelope) with { SourceObservation = observation };
        var envelope = new EventEnvelope(fixture.Scope.SessionId, fixture.Scope.Process, 99, 7, 10, 1, 1,
            null, source.Envelope.Identity, EventCodec.EncodePayload(payload, fixture.Scope.Process));
        using (var store = fixture.Create())
        {
            await using var worker = new ProcessingWorker(store, fixture.Info, processors: [new MetricProcessor()]);
            Assert.True(worker.TryEnqueue(new(EventCodec.Normalize(envelope, source.Stream),
                new SourceProvenance(fixture.Scope.Process, 10))));
            worker.Complete(SourceCompletion.FiniteEnd);
            var result = await worker.Completion.WaitAsync(TimeSpan.FromSeconds(10));
            Assert.Equal(SessionStatus.Completed, result.Status);
            Assert.Equal(1, result.DurableBoundary.IngestOrdinal);
        }
        using var reopened = SessionStore.OpenReadOnly(fixture.Path);
        var persisted = Assert.Single((await reopened.Queries.GetMetrics(fixture.Metrics(), default)).Items);
        Assert.Equal(observation, persisted.SourceObservation);
        Assert.Equal(10, persisted.TimestampQpc);
        Assert.Equal(new TimeRange(9, 10), persisted.Interval);
    }
}
