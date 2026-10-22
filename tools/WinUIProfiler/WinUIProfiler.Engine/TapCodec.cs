using System.Buffers.Binary;
using System.Text.Json;
using WinUIProfiler.Contracts;

namespace WinUIProfiler.Engine;

public static class TapCodec
{
    public static byte[] Encode(TapMessageType type, Guid requestId, ITapBody body)
    {
        Require.Id(requestId);
        if (Require.NotNull(body).GetType() != BodyType(type)) throw new ArgumentException("Command type and body do not match.");
        Validate(body);
        var json = JsonSerializer.SerializeToUtf8Bytes(body, body.GetType(), WireJson.Options);
        if (json.Length > TapProtocol.MaximumBodyBytes) throw new ArgumentException("Command body exceeds the limit.");
        var bytes = new byte[TapProtocol.HeaderBytes + json.Length];
        BinaryPrimitives.WriteUInt32LittleEndian(bytes, (uint)json.Length);
        BinaryPrimitives.WriteUInt16LittleEndian(bytes.AsSpan(4), TapProtocol.Version);
        BinaryPrimitives.WriteUInt16LittleEndian(bytes.AsSpan(6), (ushort)type);
        requestId.TryWriteBytes(bytes.AsSpan(8, 16), bigEndian: true, out _);
        BinaryPrimitives.WriteUInt32LittleEndian(bytes.AsSpan(24), ContractVersions.Schema);
        json.CopyTo(bytes.AsSpan(TapProtocol.HeaderBytes));
        return bytes;
    }

    public static TapMessage Decode(ReadOnlySpan<byte> bytes)
    {
        if (bytes.Length < TapProtocol.HeaderBytes) throw new ArgumentException("Truncated TAP header.");
        var length = BinaryPrimitives.ReadUInt32LittleEndian(bytes);
        var version = BinaryPrimitives.ReadUInt16LittleEndian(bytes[4..]);
        var type = (TapMessageType)BinaryPrimitives.ReadUInt16LittleEndian(bytes[6..]);
        var id = new Guid(bytes.Slice(8, 16), bigEndian: true);
        var schema = BinaryPrimitives.ReadUInt32LittleEndian(bytes[24..]);
        if (version != TapProtocol.Version || schema != ContractVersions.Schema)
            throw new NotSupportedException("Unsupported TAP protocol/body schema version.");
        if (length is 0 or > TapProtocol.MaximumBodyBytes || bytes.Length != TapProtocol.HeaderBytes + length)
            throw new ArgumentException("Invalid bounded body length; exactly one complete message is required.");
        Require.Id(id);
        var body = (ITapBody?)JsonSerializer.Deserialize(bytes[TapProtocol.HeaderBytes..], BodyType(type), WireJson.Options)
            ?? throw new JsonException("Command body cannot be null.");
        Validate(body);
        return new(new(length, version, type, id, schema), body);
    }

    private static Type BodyType(TapMessageType type) => type switch
    {
        TapMessageType.Handshake => typeof(HandshakeBody),
        TapMessageType.Capabilities => typeof(CapabilitiesRequestBody),
        TapMessageType.CapabilitiesResult => typeof(CapabilitiesBody),
        TapMessageType.Rundown => typeof(RundownBody),
        TapMessageType.LightweightSnapshot => typeof(LightweightSnapshotBody),
        TapMessageType.OwnershipGraph => typeof(OwnershipGraphBody),
        TapMessageType.Cancel => typeof(CancelBody),
        TapMessageType.Disconnect => typeof(DisconnectBody),
        TapMessageType.Acknowledgement => typeof(AcknowledgementBody),
        _ => throw new NotSupportedException("Unknown TAP message type.")
    };

    private static void Validate(ITapBody body)
    {
        static void Scope(Guid? root, ulong? dispatcher)
        {
            if (root is { } id) Require.Id(id);
            if (dispatcher is 0) throw new ArgumentException("Dispatcher identity cannot be zero.");
        }
        switch (body)
        {
            case HandshakeBody p:
                Require.Id(p.SessionId); p.Process.Validate();
                if (p.SchemaVersion != ContractVersions.Schema) throw new NotSupportedException("Unsupported handshake schema.");
                Span<byte> nonce = stackalloc byte[TapProtocol.NonceBytes];
                if (p.NonceBase64 is null || !Convert.TryFromBase64String(p.NonceBase64, nonce, out var written) || written != TapProtocol.NonceBytes)
                    throw new ArgumentException("Handshake nonce must be exactly 32 bytes.");
                break;
            case CapabilitiesBody p:
                Require.Id(p.Epoch);
                if (p.SchemaVersion != ContractVersions.Schema) throw new NotSupportedException("Unsupported capability schema.");
                EventValidation.ValidateCapabilities(p.Capabilities);
                break;
            case CapabilitiesRequestBody p:
                if (p.SchemaVersion != ContractVersions.Schema) throw new NotSupportedException("Unsupported capability request schema.");
                break;
            case RundownBody p:
                Require.Id(p.CaptureId); Scope(p.RootId, p.DispatcherId);
                break;
            case LightweightSnapshotBody p:
                Require.Id(p.CaptureId); Scope(p.RootId, p.DispatcherId);
                break;
            case OwnershipGraphBody p:
                Require.Id(p.CaptureId);
                if (p.MaximumObjects is < 1 or > 1_000_000 || p.MaximumEdges is < 1 or > 4_000_000)
                    throw new ArgumentException("Ownership graph bounds are out of range.");
                break;
            case CancelBody p:
                Require.Id(p.TargetRequestId);
                break;
            case DisconnectBody p:
                Require.Text(p.Reason);
                break;
            case AcknowledgementBody p:
                Require.Enum(p.Status);
                if (p.CaptureId is { } id) Require.Id(id);
                if (p.Status is CommandStatus.Unsupported or CommandStatus.Rejected or CommandStatus.Failed) Require.Text(p.Reason);
                break;
        }
    }
}
