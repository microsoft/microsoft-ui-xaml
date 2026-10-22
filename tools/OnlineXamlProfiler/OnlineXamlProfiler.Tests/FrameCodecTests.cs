// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Buffers.Binary;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using OnlineXamlProfiler.Core;

namespace OnlineXamlProfiler.Tests;

[TestClass]
public sealed class FrameCodecTests
{
    [TestMethod]
    public async Task FragmentedFramesPreserveRequestAndPayload()
    {
        using var stream = new FragmentedStream();
        byte[] first = [1, 2, 3, 4, 5];
        await FrameCodec.WriteAsync(stream, new(MessageKind.Capture, ulong.MaxValue, first), default);
        await FrameCodec.WriteAsync(stream, new(MessageKind.Disconnect, 0, []), default);
        stream.Position = 0;
        var capture = await FrameCodec.ReadAsync(stream, default);
        Assert.AreEqual(MessageKind.Capture, capture.Kind);
        Assert.AreEqual(ulong.MaxValue, capture.RequestId);
        CollectionAssert.AreEqual(first, capture.Payload);
        Assert.AreEqual(MessageKind.Disconnect, (await FrameCodec.ReadAsync(stream, default)).Kind);
    }

    [TestMethod]
    public async Task RejectsUnsupportedVersionAndOversizedPayloadBeforeAllocation()
    {
        using var stream = new MemoryStream();
        await FrameCodec.WriteAsync(stream, new(MessageKind.Hello, 0, []), default);
        byte[] header = stream.ToArray();
        header[4] = 2;
        await Assert.ThrowsExceptionAsync<InvalidDataException>(
            () => FrameCodec.ReadAsync(new MemoryStream(header), default));
        header[4] = 1;
        BinaryPrimitives.WriteUInt32LittleEndian(header.AsSpan(16), FrameCodec.MaximumPayloadSize + 1u);
        await Assert.ThrowsExceptionAsync<InvalidDataException>(
            () => FrameCodec.ReadAsync(new MemoryStream(header), default));
    }

    [TestMethod]
    public async Task RejectsUnknownKindAndTruncatedPayload()
    {
        using var stream = new MemoryStream();
        await FrameCodec.WriteAsync(stream, new(MessageKind.Snapshot, 7, [1, 2, 3]), default);
        byte[] bytes = stream.ToArray();
        await Assert.ThrowsExceptionAsync<EndOfStreamException>(() =>
            FrameCodec.ReadAsync(new MemoryStream(bytes[..^1]), default));
        bytes[6] = 99;
        await Assert.ThrowsExceptionAsync<InvalidDataException>(() =>
            FrameCodec.ReadAsync(new MemoryStream(bytes), default));
    }

    private sealed class FragmentedStream : MemoryStream
    {
        public override ValueTask<int> ReadAsync(Memory<byte> buffer, CancellationToken cancellationToken = default) =>
            base.ReadAsync(buffer[..Math.Min(buffer.Length, 3)], cancellationToken);
    }
}
