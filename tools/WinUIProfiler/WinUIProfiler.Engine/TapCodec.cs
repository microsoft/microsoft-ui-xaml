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
        var header = DecodeHeader(bytes[..TapProtocol.HeaderBytes]);
        if (bytes.Length != TapProtocol.HeaderBytes + header.BodyLength)
            throw new ArgumentException("Exactly one complete TAP message is required.");
        var body = (ITapBody?)JsonSerializer.Deserialize(bytes[TapProtocol.HeaderBytes..], BodyType(header.Type), WireJson.Options)
            ?? throw new JsonException("Command body cannot be null.");
        Validate(body);
        return new(header, body);
    }

    public static TapHeader DecodeHeader(ReadOnlySpan<byte> bytes)
    {
        if (bytes.Length != TapProtocol.HeaderBytes) throw new ArgumentException("Exactly one TAP header is required.");
        var length = BinaryPrimitives.ReadUInt32LittleEndian(bytes);
        var version = BinaryPrimitives.ReadUInt16LittleEndian(bytes[4..]);
        var type = (TapMessageType)BinaryPrimitives.ReadUInt16LittleEndian(bytes[6..]);
        var id = new Guid(bytes.Slice(8, 16), bigEndian: true);
        var schema = BinaryPrimitives.ReadUInt32LittleEndian(bytes[24..]);
        if (version != TapProtocol.Version || schema != ContractVersions.Schema)
            throw new NotSupportedException("Unsupported TAP protocol/body schema version.");
        if (length is 0 or > TapProtocol.MaximumBodyBytes)
            throw new ArgumentException("Invalid bounded TAP body length.");
        Require.Id(id);
        _ = BodyType(type);
        return new(length, version, type, id, schema);
    }

    private static Type BodyType(TapMessageType type) => type switch
    {
        TapMessageType.Handshake => typeof(HandshakeBody),
        TapMessageType.Hello => typeof(HelloBody),
        TapMessageType.TakeSnapshot => typeof(TakeSnapshotBody),
        TapMessageType.SnapshotChunk => typeof(SnapshotChunkBody),
        TapMessageType.SnapshotResult => typeof(SnapshotResultBody),
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
            case TakeSnapshotBody p:
                Require.Id(p.CaptureId);
                break;
            case SnapshotChunkBody p:
                Require.Id(p.CaptureId);
                if (p.Index is < 0 or >= TapProtocol.MaximumSnapshotChunks || p.DataBase64 is null ||
                    p.DataBase64.Length > (TapProtocol.SnapshotChunkBytes + 2) / 3 * 4)
                    throw new ArgumentException("Snapshot chunk exceeds its bound.");
                var chunk = Convert.FromBase64String(p.DataBase64);
                if (chunk.Length is 0 or > TapProtocol.SnapshotChunkBytes ||
                    Convert.ToBase64String(chunk) != p.DataBase64)
                    throw new ArgumentException("Snapshot chunks require bounded canonical base64.");
                break;
            case SnapshotResultBody p:
                Require.Id(p.CaptureId);
                if (p.ChunkCount is < 1 or > TapProtocol.MaximumSnapshotChunks ||
                    p.ByteCount is < 1 or > TapProtocol.MaximumSnapshotBytes)
                    throw new ArgumentException("Snapshot result exceeds its bound.");
                break;
            case HelloBody p:
                if (p.Message != "hello") throw new ArgumentException("TAP hello message must be exactly 'hello'.");
                break;
            case HandshakeBody p:
                Require.Id(p.SessionId); p.Process.Validate();
                if (p.SchemaVersion != ContractVersions.Schema) throw new NotSupportedException("Unsupported handshake schema.");
                Span<byte> nonce = stackalloc byte[TapProtocol.NonceBytes];
                if (p.NonceBase64 is null || !Convert.TryFromBase64String(p.NonceBase64, nonce, out var written) || written != TapProtocol.NonceBytes)
                    throw new ArgumentException("Handshake nonce must be exactly 32 bytes.");
                break;
            case CapabilitiesBody p:
                if (p.Epoch is { } epoch) Require.Id(epoch);
                if (p.SchemaVersion != ContractVersions.Schema) throw new NotSupportedException("Unsupported capability schema.");
                EventValidation.ValidateCapabilities(p.Capabilities);
                if (p.Epoch is null && p.Capabilities.Count != 0)
                    throw new ArgumentException("A TAP without a private runtime epoch cannot advertise private runtime capabilities.");
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
