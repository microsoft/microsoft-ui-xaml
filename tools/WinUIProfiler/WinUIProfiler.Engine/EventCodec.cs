using System.Text.Json;
using WinUIProfiler.Contracts;

namespace WinUIProfiler.Engine;

public static class EventCodec
{
    public const int MaximumEncodedBytes = 2 * ContractVersions.MaximumPayloadBytes;
    public static NormalizedEvent Normalize(EventEnvelope envelope, ProducerStreamKey stream)
    {
        stream.Validate();
        var captured = Capture(envelope);
        if (stream.ProviderId != captured.Identity.ProviderId || stream.ProducerProcessId != captured.ProducerProcessId)
            throw new ArgumentException("The envelope does not belong to the authoritative producer stream.");
        var definition = EventCatalog.Get(captured.Identity.ProviderId, captured.Identity.EventId);
        var body = (IEventPayload)JsonSerializer.Deserialize(captured.OwnedPayload.Span, definition.PayloadType, WireJson.Options)!;
        EventValidation.Validate(body, captured.Process, stream.ProducerEpoch);
        return new(stream, captured);
    }

    public static EventEnvelope Capture(EventEnvelope envelope)
    {
        ArgumentNullException.ThrowIfNull(envelope);
        var definition = EventCatalog.Get(envelope.Identity.ProviderId, envelope.Identity.EventId);
        if (envelope.Identity.Version != definition.Identity.Version)
            throw new NotSupportedException("Unsupported event schema version.");
        if (envelope.Identity != definition.Identity)
            throw new ArgumentException("Event name, keyword and identity do not match the schema.");
        if (envelope.Identity.ProviderId == EventCatalog.RuntimeProviderId && envelope.ProducerProcessId != envelope.Process.ProcessId)
            throw new ArgumentException("Runtime events must originate in the semantic target process.");
        var payload = (IEventPayload?)JsonSerializer.Deserialize(envelope.OwnedPayload.Span, definition.PayloadType, WireJson.Options)
            ?? throw new JsonException("A payload cannot be null.");
        EventValidation.Validate(payload, envelope.Process);
        return new(envelope.SessionId, envelope.Process, envelope.ProducerProcessId, envelope.ThreadId,
            envelope.TimestampQpc, envelope.IngestOrdinal, envelope.SourceSequence, envelope.ActivityId, envelope.Identity, envelope.OwnedPayload);
    }

    public static byte[] Encode(EventEnvelope envelope) =>
        JsonSerializer.SerializeToUtf8Bytes(Capture(envelope), WireJson.Options);

    public static EventEnvelope Decode(ReadOnlySpan<byte> bytes)
    {
        if (bytes.Length is 0 or > MaximumEncodedBytes)
            throw new ArgumentOutOfRangeException(nameof(bytes));
        return Capture(JsonSerializer.Deserialize<EventEnvelope>(bytes, WireJson.Options)
            ?? throw new JsonException("An envelope cannot be null."));
    }

    public static byte[] EncodePayload(IEventPayload payload, ProcessInstanceKey process)
    {
        EventValidation.Validate(payload, process);
        return JsonSerializer.SerializeToUtf8Bytes(payload, payload.GetType(), WireJson.Options);
    }

    public static T ReadPayload<T>(EventEnvelope envelope) where T : IEventPayload
    {
        var captured = Capture(envelope);
        if (EventCatalog.Get(captured.Identity.ProviderId, captured.Identity.EventId).PayloadType != typeof(T))
            throw new ArgumentException("The requested payload type does not match the event identity.");
        return JsonSerializer.Deserialize<T>(captured.OwnedPayload.Span, WireJson.Options)
            ?? throw new JsonException("A payload cannot be null.");
    }
}
