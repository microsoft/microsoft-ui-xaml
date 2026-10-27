using System.Buffers.Binary;
using System.Text.Json;
using System.Text.Json.Serialization;
using WinUIProfiler.Contracts;

namespace WinUIProfiler.Engine;

public static class TapCodec
{
    private static readonly JsonSerializerOptions Options = new()
    {
        PropertyNamingPolicy = JsonNamingPolicy.CamelCase,
        UnmappedMemberHandling = JsonUnmappedMemberHandling.Disallow,
        MaxDepth = 16,
        Converters = { new JsonStringEnumConverter(allowIntegerValues: false) }
    };

    public static byte[] Encode(TapMessageType type, Guid requestId, ITapBody body)
    {
        Require.Id(requestId);
        if (Require.NotNull(body).GetType() != BodyType(type))
        {
            throw new ArgumentException("Command type and body do not match.");
        }

        Validate(body);
        var json = JsonSerializer.SerializeToUtf8Bytes(body, body.GetType(), Options);
        if (json.Length > TapProtocol.MaximumBodyBytes)
        {
            throw new ArgumentException("Command body exceeds the limit.");
        }

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
        if (bytes.Length < TapProtocol.HeaderBytes)
        {
            throw new ArgumentException("Truncated TAP header.");
        }

        var header = DecodeHeader(bytes[..TapProtocol.HeaderBytes]);
        if (bytes.Length != TapProtocol.HeaderBytes + header.BodyLength)
        {
            throw new ArgumentException("Exactly one complete TAP message is required.");
        }

        var body = (ITapBody?)JsonSerializer.Deserialize(bytes[TapProtocol.HeaderBytes..], BodyType(header.Type), Options)
                    ?? throw new JsonException("Command body cannot be null.");
        Validate(body);
        return new(header, body);
    }

    public static TapHeader DecodeHeader(ReadOnlySpan<byte> bytes)
    {
        if (bytes.Length != TapProtocol.HeaderBytes)
        {
            throw new ArgumentException("Exactly one TAP header is required.");
        }

        var length = BinaryPrimitives.ReadUInt32LittleEndian(bytes);
        var version = BinaryPrimitives.ReadUInt16LittleEndian(bytes[4..]);
        var type = (TapMessageType)BinaryPrimitives.ReadUInt16LittleEndian(bytes[6..]);
        var id = new Guid(bytes.Slice(8, 16), bigEndian: true);
        var schema = BinaryPrimitives.ReadUInt32LittleEndian(bytes[24..]);
        if (version != TapProtocol.Version || schema != ContractVersions.Schema)
        {
            throw new NotSupportedException("Unsupported TAP protocol/body schema version.");
        }

        if (length is 0 or > TapProtocol.MaximumBodyBytes)
        {
            throw new ArgumentException("Invalid bounded TAP body length.");
        }

        Require.Id(id);
        _ = BodyType(type);
        return new(length, version, type, id, schema);
    }

    private static Type BodyType(TapMessageType type) => type switch
    {
        TapMessageType.Handshake => typeof(HandshakeBody),
        TapMessageType.Hello => typeof(HelloBody),
        TapMessageType.Disconnect => typeof(DisconnectBody),
        TapMessageType.Acknowledgement => typeof(AcknowledgementBody),
        _ => throw new NotSupportedException("Unknown TAP message type.")
    };

    private static void Validate(ITapBody body)
    {
        switch (body)
        {
            case HelloBody hello:
                if (hello.Message != "hello")
                {
                    throw new ArgumentException("TAP hello message must be exactly 'hello'.");
                }

                break;
            case HandshakeBody handshake:
                Require.Id(handshake.SessionId);
                handshake.Process.Validate();
                if (handshake.Process.CreationTimeFileTime is not > 0)
                {
                    throw new ArgumentException("Handshake requires process creation time.");
                }

                if (handshake.SchemaVersion != ContractVersions.Schema)
                {
                    throw new NotSupportedException("Unsupported handshake schema.");
                }

                Span<byte> nonce = stackalloc byte[TapProtocol.NonceBytes];
                if (handshake.NonceBase64 is null ||
!Convert.TryFromBase64String(handshake.NonceBase64, nonce, out var written) ||
                    written != TapProtocol.NonceBytes)
                {
                    throw new ArgumentException("Handshake nonce must be exactly 32 bytes.");
                }

                break;
            case DisconnectBody disconnect:
                Require.Text(disconnect.Reason);
                break;
            case AcknowledgementBody acknowledgement:
                Require.Enum(acknowledgement.Status);
                if (acknowledgement.CaptureId is { } id)
                {
                    Require.Id(id);
                }

                if (acknowledgement.Status is CommandStatus.Unsupported or CommandStatus.Rejected or CommandStatus.Failed)
                {
                    Require.Text(acknowledgement.Reason);
                }

                break;
        }
    }
}
