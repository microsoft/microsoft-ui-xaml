using System.Buffers.Binary;
using WinUIProfiler.Contracts;
using WinUIProfiler.Engine;
using Xunit;

namespace WinUIProfiler.Core.Tests;

public class TapCodecTests
{
    [Fact]
    public void Hello_preserves_exact_message_and_request_correlation()
    {
        var id = Guid.NewGuid();
        var frame = TapCodec.Encode(TapMessageType.Hello, id, new HelloBody("hello"));
        var message = TapCodec.Decode(frame);
        Assert.Equal(id, message.Header.RequestId);
        Assert.Equal(TapMessageType.Hello, message.Header.Type);
        Assert.Equal("hello", Assert.IsType<HelloBody>(message.Body).Message);
    }

    [Theory]
    [InlineData("Hello")]
    [InlineData("")]
    [InlineData("hello!")]
    public void Hello_is_literal(string text) =>
        Assert.Throws<ArgumentException>(() => TapCodec.Encode(TapMessageType.Hello, Guid.NewGuid(), new HelloBody(text)));

    [Fact]
    public void Handshake_requires_exact_nonce_and_schema()
    {
        var body = new HandshakeBody(Guid.NewGuid(), new(42, 123, Guid.NewGuid()), 1, Convert.ToBase64String(new byte[32]));
        Assert.IsType<HandshakeBody>(TapCodec.Decode(TapCodec.Encode(TapMessageType.Handshake, Guid.NewGuid(), body)).Body);
        Assert.Throws<ArgumentException>(() => TapCodec.Encode(TapMessageType.Handshake, Guid.NewGuid(),
            body with { NonceBase64 = Convert.ToBase64String(new byte[31]) }));
        Assert.Throws<NotSupportedException>(() => TapCodec.Encode(TapMessageType.Handshake, Guid.NewGuid(), body with { SchemaVersion = 2 }));
    }

    [Fact]
    public void Unsupported_schema_and_truncated_frames_are_rejected()
    {
        var frame = TapCodec.Encode(TapMessageType.Hello, Guid.NewGuid(), new HelloBody("hello"));
        Assert.Throws<ArgumentException>(() => TapCodec.Decode(frame.AsSpan(0, frame.Length - 1)));
        BinaryPrimitives.WriteUInt32LittleEndian(frame.AsSpan(24), 2);
        Assert.Throws<NotSupportedException>(() => TapCodec.Decode(frame));
    }

    [Fact]
    public void Wrong_body_type_and_unknown_commands_are_rejected()
    {
        Assert.Throws<ArgumentException>(() => TapCodec.Encode(TapMessageType.Hello, Guid.NewGuid(), new DisconnectBody("stop")));
        Assert.Throws<NotSupportedException>(() => TapCodec.Encode((TapMessageType)11, Guid.NewGuid(), new HelloBody("hello")));
    }
}
