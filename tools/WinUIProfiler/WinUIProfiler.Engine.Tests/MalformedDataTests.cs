using System.Text;
using System.Text.Json;
using WinUIProfiler.Contracts;
using Xunit;

namespace WinUIProfiler.Engine.Tests;

public sealed class MalformedDataTests
{
    [Fact]
    public void Duplicate_json_properties_are_not_ambiguous_successes()
    {
        var e = BoundaryTests.Envelope(new ObjectLifetimePayload(CoreTests.Key(), LifetimeOperation.Birth, 1, null, DataQuality.Complete));
        var json = Encoding.UTF8.GetString(e.OwnedPayload.Span);
        Assert.Throws<JsonException>(() => EventCodec.Capture(BoundaryTests.ReplacePayload(e, json.Insert(1, "\"operation\":\"Destroy\","))));
    }

    [Fact]
    public void Numeric_values_do_not_masquerade_as_boolean_or_string_units()
    {
        Assert.Throws<ArgumentException>(() => new TypedValue(ValueKind.Integer, ValueUnit.Boolean, 1, null, null, null, DataQuality.Complete).Validate());
        Assert.Throws<ArgumentException>(() => new TypedValue(ValueKind.Number, ValueUnit.String, null, 1, null, null, DataQuality.Complete).Validate());
    }

    [Fact]
    public void Null_capability_entries_are_rejected_as_bad_data()
    {
        var body = new CapabilitiesBody(CoreTests.Epoch, 1, [null!]);
        Assert.ThrowsAny<ArgumentException>(() => TapCodec.Encode(TapMessageType.CapabilitiesResult, CoreTests.Session, body));
    }

    [Fact]
    public void Address_zero_is_not_a_substitute_for_unknown_address()
    {
        Assert.ThrowsAny<ArgumentException>(() => EventCodec.Capture(BoundaryTests.Envelope(
            new ObjectLifetimePayload(CoreTests.Key(), LifetimeOperation.Birth, 1, 0, DataQuality.Complete))));
    }

    [Fact]
    public void Reallocation_cannot_reuse_the_previous_lifetime_id()
    {
        Assert.Throws<ArgumentException>(() => EventCodec.Capture(BoundaryTests.Envelope(
            new AllocationPayload(1, AllocationOperation.Reallocate, 1, 64, 80, CoreTests.Epoch, null, DataQuality.Complete))));
    }

    [Fact]
    public void Completed_attempt_cannot_hide_a_failure_and_submission_requires_known_success()
    {
        Assert.Throws<ArgumentException>(() => EventCodec.Capture(BoundaryTests.Envelope(
            new FramePayload(1, null, FrameOutcome.AttemptCompleted, 1, CoreTests.Session, -1))));
        Assert.Throws<ArgumentException>(() => EventCodec.Capture(BoundaryTests.Envelope(
            new FramePayload(1, 1, FrameOutcome.Submitted, 1, CoreTests.Session, null))));
    }
}
