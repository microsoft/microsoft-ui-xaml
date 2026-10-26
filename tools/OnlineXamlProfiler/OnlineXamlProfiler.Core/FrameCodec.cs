// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Buffers.Binary;

namespace OnlineXamlProfiler.Core;

public enum MessageKind : ushort
{
    Hello = 1,
    Capture = 2,
    Snapshot = 3,
    Error = 4,
    Disconnect = 5,
}

public sealed record ProtocolFrame(MessageKind Kind, ulong RequestId, byte[] Payload);

public static class FrameCodec
{
    public const uint Magic = 0x504D584F;
    public const ushort Version = 1;
    public const int HeaderSize = 20;
    public const int MaximumPayloadSize = 64 * 1024 * 1024;

    public static async Task<ProtocolFrame> ReadAsync(Stream stream, CancellationToken cancellationToken)
    {
        byte[] header = new byte[HeaderSize];
        await stream.ReadExactlyAsync(header, cancellationToken).ConfigureAwait(false);
        if (BinaryPrimitives.ReadUInt32LittleEndian(header) != Magic ||
            BinaryPrimitives.ReadUInt16LittleEndian(header.AsSpan(4)) != Version)
        {
            throw new InvalidDataException("Unrecognized OnlineXamlProfiler protocol or version.");
        }

        var kind = (MessageKind)BinaryPrimitives.ReadUInt16LittleEndian(header.AsSpan(6));
        if (!Enum.IsDefined(kind))
        {
            throw new InvalidDataException("Unrecognized OnlineXamlProfiler message kind.");
        }
        uint length = BinaryPrimitives.ReadUInt32LittleEndian(header.AsSpan(16));
        if (length > MaximumPayloadSize)
        {
            throw new InvalidDataException("The snapshot exceeds the protocol's 64 MiB limit.");
        }

        byte[] payload = new byte[(int)length];
        await stream.ReadExactlyAsync(payload, cancellationToken).ConfigureAwait(false);
        return new(kind, BinaryPrimitives.ReadUInt64LittleEndian(header.AsSpan(8)), payload);
    }

    public static async Task WriteAsync(Stream stream, ProtocolFrame frame, CancellationToken cancellationToken)
    {
        if (!Enum.IsDefined(frame.Kind) || frame.Payload.Length > MaximumPayloadSize)
        {
            throw new InvalidDataException("Invalid OnlineXamlProfiler frame.");
        }

        byte[] header = new byte[HeaderSize];
        BinaryPrimitives.WriteUInt32LittleEndian(header, Magic);
        BinaryPrimitives.WriteUInt16LittleEndian(header.AsSpan(4), Version);
        BinaryPrimitives.WriteUInt16LittleEndian(header.AsSpan(6), (ushort)frame.Kind);
        BinaryPrimitives.WriteUInt64LittleEndian(header.AsSpan(8), frame.RequestId);
        BinaryPrimitives.WriteUInt32LittleEndian(header.AsSpan(16), (uint)frame.Payload.Length);
        await stream.WriteAsync(header, cancellationToken).ConfigureAwait(false);
        await stream.WriteAsync(frame.Payload, cancellationToken).ConfigureAwait(false);
        await stream.FlushAsync(cancellationToken).ConfigureAwait(false);
    }
}
