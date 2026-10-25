using System.Buffers.Binary;
using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Target;
using Xunit;

namespace WinUIProfiler.Engine.Tests;

public sealed class TapTransportTests
{
    [Fact]
    public void Hello_wire_type_ten_decodes_exact_message_and_request_identity()
    {
        var id = Guid.NewGuid();
        var message = TapCodec.Decode(HelloFrame(id, "{\"message\":\"hello\"}"));
        Assert.Equal(TapMessageType.Hello, message.Header.Type);
        Assert.Equal(id, message.Header.RequestId);
        Assert.Equal("hello", Assert.IsType<HelloBody>(message.Body).Message);
        Assert.Equal(HelloFrame(id, "{\"message\":\"hello\"}"),
            TapCodec.Encode(TapMessageType.Hello, id, new HelloBody("hello")));
    }

    [Theory]
    [InlineData(null)]
    [InlineData("")]
    [InlineData("Hello")]
    [InlineData("hello ")]
    public void Hello_encoder_rejects_other_messages(string? message) =>
        Assert.Throws<ArgumentException>(() =>
            TapCodec.Encode(TapMessageType.Hello, Guid.NewGuid(), new HelloBody(message!)));

    [Theory]
    [InlineData("{}")]
    [InlineData("null")]
    [InlineData("{\"message\":null}")]
    [InlineData("{\"message\":\"Hello\"}")]
    [InlineData("{\"message\":\"hello \"}")]
    [InlineData("{\"message\":1}")]
    [InlineData("{\"message\":\"hello\",\"message\":\"hello\"}")]
    [InlineData("{\"message\":\"hello\",\"\\u006dessage\":\"hello\"}")]
    [InlineData("{\"message\":\"hello\",\"extra\":null}")]
    public void Hello_wire_body_is_strict(string json)
    {
        var error = Record.Exception(() => TapCodec.Decode(HelloFrame(Guid.NewGuid(), json)));
        Assert.NotNull(error);
        Assert.True(error is ArgumentException or System.Text.Json.JsonException, error.ToString());
    }

    private static byte[] HelloFrame(Guid id, string json)
    {
        var body = System.Text.Encoding.UTF8.GetBytes(json);
        var frame = new byte[TapProtocol.HeaderBytes + body.Length];
        // Reuse the existing codec for the header, then supply the future wire type.
        TapCodec.Encode(TapMessageType.Disconnect, id, new DisconnectBody("test"))
            .AsSpan(0, TapProtocol.HeaderBytes).CopyTo(frame);
        BinaryPrimitives.WriteUInt32LittleEndian(frame, (uint)body.Length);
        BinaryPrimitives.WriteUInt16LittleEndian(frame.AsSpan(6), 10);
        body.CopyTo(frame, TapProtocol.HeaderBytes);
        return frame;
    }

    [Fact]
    public async Task Fragmented_frames_roundtrip_without_consuming_the_next_message()
    {
        var first = Guid.NewGuid();
        var second = Guid.NewGuid();
        await using var output = new MemoryStream();
        await TapFraming.WriteAsync(output, TapMessageType.Disconnect, first, new DisconnectBody("complete"), default);
        await TapFraming.WriteAsync(output, TapMessageType.Cancel, second, new CancelBody(first), default);
        await using var input = new FragmentedStream(output.ToArray(), 1);
        var message = await TapFraming.ReadAsync(input, default);
        Assert.Equal(first, message.Header.RequestId);
        Assert.Equal("complete", Assert.IsType<DisconnectBody>(message.Body).Reason);
        Assert.Equal(second, (await TapFraming.ReadAsync(input, default)).Header.RequestId);
        Assert.Equal(input.Length, input.Position);
    }

    [Theory]
    [InlineData(0)]
    [InlineData(1)]
    [InlineData(27)]
    [InlineData(28)]
    [InlineData(29)]
    public async Task Disconnect_before_a_complete_frame_is_explicit(int bytes)
    {
        var frame = TapCodec.Encode(TapMessageType.Disconnect, Guid.NewGuid(), new DisconnectBody("complete"));
        await using var input = new FragmentedStream(frame[..bytes], 3);
        await Assert.ThrowsAsync<EndOfStreamException>(async () => await TapFraming.ReadAsync(input, default));
    }

    [Theory]
    [InlineData(0)]
    [InlineData(TapProtocol.MaximumBodyBytes + 1)]
    [InlineData(int.MaxValue)]
    public async Task Untrusted_body_length_is_rejected_before_reading_or_allocating_the_body(int length)
    {
        var bytes = TapCodec.Encode(TapMessageType.Disconnect, Guid.NewGuid(), new DisconnectBody("complete"));
        BinaryPrimitives.WriteUInt32LittleEndian(bytes, (uint)length);
        await using var input = new FragmentedStream(bytes, 28);
        await Assert.ThrowsAsync<ArgumentException>(async () => await TapFraming.ReadAsync(input, default));
        Assert.Equal(TapProtocol.HeaderBytes, input.Position);
    }

    [Theory]
    [InlineData(4)]
    [InlineData(6)]
    [InlineData(24)]
    public async Task Unsupported_header_is_rejected_without_reading_the_body(int offset)
    {
        var bytes = TapCodec.Encode(TapMessageType.Disconnect, Guid.NewGuid(), new DisconnectBody("complete"));
        BinaryPrimitives.WriteUInt16LittleEndian(bytes.AsSpan(offset), ushort.MaxValue);
        await using var input = new FragmentedStream(bytes, 28);
        await Assert.ThrowsAsync<NotSupportedException>(async () => await TapFraming.ReadAsync(input, default));
        Assert.Equal(TapProtocol.HeaderBytes, input.Position);
    }

    [Fact]
    public async Task Cancellation_is_not_an_empty_or_successful_response()
    {
        using var cancelled = new CancellationTokenSource();
        cancelled.Cancel();
        await using var input = new MemoryStream();
        await Assert.ThrowsAnyAsync<OperationCanceledException>(async () =>
            await TapFraming.ReadAsync(input, cancelled.Token));
        await Assert.ThrowsAnyAsync<OperationCanceledException>(async () =>
            await TapFraming.WriteAsync(input, TapMessageType.Cancel, Guid.NewGuid(),
                new CancelBody(Guid.NewGuid()), cancelled.Token));
        Assert.Equal(0, input.Length);
    }

    private sealed class FragmentedStream(byte[] bytes, int fragment) : MemoryStream(bytes)
    {
        public override ValueTask<int> ReadAsync(Memory<byte> buffer, CancellationToken cancellationToken = default) =>
            base.ReadAsync(buffer[..Math.Min(buffer.Length, fragment)], cancellationToken);
    }
}
