using System.Diagnostics;
using System.IO.Pipes;
using System.Runtime.Versioning;
using System.Security.Cryptography;
using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Target;
using Xunit;

namespace WinUIProfiler.Engine.Tests;

[SupportedOSPlatform("windows")]
public sealed class TapClientTests
{
    private static readonly TimeSpan Deadline = TimeSpan.FromSeconds(5);
    [Theory]
    [InlineData("valid")]
    [InlineData("valid-multi")]
    [InlineData("wrong-request")]
    [InlineData("wrong-capture")]
    [InlineData("chunk-order")]
    [InlineData("wrong-total")]
    public async Task Snapshot_receives_only_complete_correlated_tree_chunks(string response)
    {
        var scope = Scope();
        await using var server = new PipeFixture(async (pipe, token) =>
        {
            var handshake = await TapFraming.ReadAsync(pipe, token);
            await Ack(pipe, handshake, CommandStatus.Completed, null, token);
            var request = await TapFraming.ReadAsync(pipe, token);
            Assert.Equal(TapMessageType.TakeSnapshot, request.Header.Type);
            var capture = Assert.IsType<TakeSnapshotBody>(request.Body).CaptureId;
            var json = TapTreeSnapshotTests.SnapshotJson(
                scope.Process.ProcessId, scope.Process.CreationTimeFileTime!.Value, capture)
                .Replace("\"name\":\"Root\"", "\"name\":\"Root\u2603\"");
            var bytes = System.Text.Encoding.UTF8.GetBytes(json);
            var firstLength = response == "valid-multi" ? Array.IndexOf(bytes, (byte)0xE2) + 1 : bytes.Length;
            await TapFraming.WriteAsync(pipe, TapMessageType.SnapshotChunk,
                response == "wrong-request" ? Guid.NewGuid() : request.Header.RequestId,
                new SnapshotChunkBody(response == "wrong-capture" ? Guid.NewGuid() : capture,
                    response == "chunk-order" ? 1 : 0, Convert.ToBase64String(bytes, 0, firstLength)), token);
            if (response == "valid-multi")
                await TapFraming.WriteAsync(pipe, TapMessageType.SnapshotChunk, request.Header.RequestId,
                    new SnapshotChunkBody(capture, 1, Convert.ToBase64String(bytes, firstLength, bytes.Length - firstLength)), token);
            if (response is "valid" or "valid-multi" or "wrong-total")
                await TapFraming.WriteAsync(pipe, TapMessageType.SnapshotResult, request.Header.RequestId,
                    new SnapshotResultBody(capture, response == "valid-multi" ? 2 : 1,
                        bytes.Length + (response == "wrong-total" ? 1 : 0)), token);
            if (response is "valid" or "valid-multi")
            {
                var hello = await TapFraming.ReadAsync(pipe, token);
                await TapFraming.WriteAsync(pipe, TapMessageType.Hello, hello.Header.RequestId, new HelloBody("hello"), token);
            }
        });
        await using var client = await TapClient.ConnectAsync(server.Name, scope, new byte[32], Deadline, default);
        var operation = client.TakeSnapshotAsync(Deadline, CancellationToken.None);
        if (response is "valid" or "valid-multi")
        {
            Assert.Equal(2, (await operation).Correlations.Count);
            Assert.Equal("hello", await client.HelloAsync(Deadline, default));
        }
        else
        {
            await Assert.ThrowsAsync<InvalidDataException>(() => operation);
            await Assert.ThrowsAsync<ObjectDisposedException>(() => client.HelloAsync(Deadline, default));
        }
    }

    [Theory]
    [InlineData(CommandStatus.Unsupported)]
    [InlineData(CommandStatus.Failed)]
    public async Task Snapshot_rejection_is_explicit_and_preserves_healthy_hello(CommandStatus status)
    {
        await using var server = new PipeFixture(async (pipe, token) =>
        {
            var handshake = await TapFraming.ReadAsync(pipe, token);
            await Ack(pipe, handshake, CommandStatus.Completed, null, token);
            var request = await TapFraming.ReadAsync(pipe, token);
            await TapFraming.WriteAsync(pipe, TapMessageType.Acknowledgement, request.Header.RequestId,
                new AcknowledgementBody(status, ((TakeSnapshotBody)request.Body).CaptureId, "No accessible visual roots."), token);
            var hello = await TapFraming.ReadAsync(pipe, token);
            await TapFraming.WriteAsync(pipe, TapMessageType.Hello, hello.Header.RequestId, new HelloBody("hello"), token);
        });
        await using var client = await TapClient.ConnectAsync(server.Name, Scope(), new byte[32], Deadline, default);
        if (status == CommandStatus.Unsupported)
            await Assert.ThrowsAsync<NotSupportedException>(() => client.TakeSnapshotAsync(Deadline, default));
        else
            await Assert.ThrowsAsync<IOException>(() => client.TakeSnapshotAsync(Deadline, default));
        Assert.Equal("hello", await client.HelloAsync(Deadline, default));
    }

    [Fact]
    public async Task Cancelling_partial_snapshot_closes_the_pipe_without_publishing_a_document()
    {
        var received = new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);
        await using var server = new PipeFixture(async (pipe, token) =>
        {
            var handshake = await TapFraming.ReadAsync(pipe, token);
            await Ack(pipe, handshake, CommandStatus.Completed, null, token);
            var request = await TapFraming.ReadAsync(pipe, token);
            await TapFraming.WriteAsync(pipe, TapMessageType.SnapshotChunk, request.Header.RequestId,
                new SnapshotChunkBody(((TakeSnapshotBody)request.Body).CaptureId, 0, "ew=="), token);
            received.SetResult();
            await Task.Delay(Timeout.InfiniteTimeSpan, token);
        });
        await using var client = await TapClient.ConnectAsync(server.Name, Scope(), new byte[32], Deadline, default);
        using var cancel = new CancellationTokenSource();
        var capture = client.TakeSnapshotAsync(Deadline, cancel.Token);
        await received.Task.WaitAsync(Deadline);
        cancel.Cancel();
        await Assert.ThrowsAnyAsync<OperationCanceledException>(() => capture);
        await Assert.ThrowsAsync<ObjectDisposedException>(() => client.HelloAsync(Deadline, default));
    }
    private static SessionScope Scope()
    {
        using var process = Process.GetCurrentProcess();
        return new(Guid.NewGuid(), new((uint)process.Id, process.StartTime.ToFileTimeUtc(), Guid.NewGuid()));
    }

    [Fact]
    public async Task Hello_returns_only_exact_correlated_reply_and_keeps_connection_usable()
    {
        await using var server = new PipeFixture(async (pipe, token) =>
        {
            var handshake = await TapFraming.ReadAsync(pipe, token);
            await Ack(pipe, handshake, CommandStatus.Completed, null, token);
            for (var i = 0; i < 2; ++i)
            {
                var request = await TapFraming.ReadAsync(pipe, token);
                Assert.Equal(TapMessageType.Hello, request.Header.Type);
                Assert.Equal("hello", Assert.IsType<HelloBody>(request.Body).Message);
                await TapFraming.WriteAsync(pipe, TapMessageType.Hello, request.Header.RequestId,
                    new HelloBody("hello"), token);
            }
        });
        await using var client = await TapClient.ConnectAsync(server.Name, Scope(), new byte[32], Deadline, default);
        Assert.Equal("hello", await client.HelloAsync(Deadline, default));
        Assert.Equal("hello", await client.HelloAsync(Deadline, default));
        await server.Completed.WaitAsync(Deadline);
    }

    [Theory]
    [InlineData("completed")]
    [InlineData("rejected")]
    [InlineData("capabilities")]
    [InlineData("wrong-id")]
    public async Task Hello_rejects_unexpected_reply_and_closes_transport(string response)
    {
        await using var server = new PipeFixture(async (pipe, token) =>
        {
            var handshake = await TapFraming.ReadAsync(pipe, token);
            await Ack(pipe, handshake, CommandStatus.Completed, null, token);
            var request = await TapFraming.ReadAsync(pipe, token);
            if (response is "completed" or "rejected")
                await TapFraming.WriteAsync(pipe, TapMessageType.Acknowledgement, request.Header.RequestId,
                    new AcknowledgementBody(response == "completed" ? CommandStatus.Completed : CommandStatus.Rejected,
                        null, response == "rejected" ? "Rejected hello." : null), token);
            else if (response == "wrong-id")
                await TapFraming.WriteAsync(pipe, TapMessageType.Hello, Guid.NewGuid(), new HelloBody("hello"), token);
            else
                await TapFraming.WriteAsync(pipe, TapMessageType.CapabilitiesResult, request.Header.RequestId,
                    new CapabilitiesBody(Guid.NewGuid(), 1, []), token);
        });
        await using var client = await TapClient.ConnectAsync(server.Name, Scope(), new byte[32], Deadline, default);
        await Assert.ThrowsAsync<InvalidDataException>(() => client.HelloAsync(Deadline, default));
        await Assert.ThrowsAsync<ObjectDisposedException>(() => client.HelloAsync(Deadline, default));
        await server.Completed.WaitAsync(Deadline);
    }

    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    public async Task Hello_deadline_or_cancellation_closes_partial_reply(bool cancel)
    {
        var received = new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);
        await using var server = new PipeFixture(async (pipe, token) =>
        {
            var handshake = await TapFraming.ReadAsync(pipe, token);
            await Ack(pipe, handshake, CommandStatus.Completed, null, token);
            await TapFraming.ReadAsync(pipe, token);
            await pipe.WriteAsync(new byte[] { 19, 0, 0 }, token);
            received.SetResult();
            await Task.Delay(Timeout.InfiniteTimeSpan, token);
        });
        await using var client = await TapClient.ConnectAsync(server.Name, Scope(), new byte[32], Deadline, default);
        using var cancelled = new CancellationTokenSource();
        var hello = client.HelloAsync(cancel ? Deadline : TimeSpan.FromMilliseconds(100), cancelled.Token);
        await received.Task.WaitAsync(Deadline);
        if (cancel)
        {
            cancelled.Cancel();
            await Assert.ThrowsAnyAsync<OperationCanceledException>(() => hello);
        }
        else await Assert.ThrowsAsync<TimeoutException>(() => hello);
        await Assert.ThrowsAsync<ObjectDisposedException>(() => client.HelloAsync(Deadline, default));
    }

    [Fact]
    public async Task Hello_detects_pipe_loss_instead_of_returning_a_local_greeting()
    {
        await using var server = new PipeFixture(async (pipe, token) =>
        {
            var handshake = await TapFraming.ReadAsync(pipe, token);
            await Ack(pipe, handshake, CommandStatus.Completed, null, token);
            await TapFraming.ReadAsync(pipe, token);
            await pipe.DisposeAsync();
        });
        await using var client = await TapClient.ConnectAsync(server.Name, Scope(), new byte[32], Deadline, default);
        await Assert.ThrowsAsync<EndOfStreamException>(() => client.HelloAsync(Deadline, default));
        await Assert.ThrowsAsync<ObjectDisposedException>(() => client.HelloAsync(Deadline, default));
        await server.Completed.WaitAsync(Deadline);
    }

    [Fact]
    public async Task Actual_current_user_pipe_authenticates_and_matches_typed_requests()
    {
        var scope = Scope();
        var nonce = RandomNumberGenerator.GetBytes(TapProtocol.NonceBytes);
        await using var server = new PipeFixture(async (pipe, token) =>
        {
            var handshake = await TapFraming.ReadAsync(pipe, token);
            var body = Assert.IsType<HandshakeBody>(handshake.Body);
            Assert.Equal(scope.SessionId, body.SessionId);
            Assert.Equal(scope.Process, body.Process);
            Assert.Equal(nonce, Convert.FromBase64String(body.NonceBase64));
            await Ack(pipe, handshake, CommandStatus.Completed, null, token);
            var request = await TapFraming.ReadAsync(pipe, token);
            Assert.Equal(TapMessageType.Capabilities, request.Header.Type);
            await TapFraming.WriteAsync(pipe, TapMessageType.CapabilitiesResult, request.Header.RequestId,
                new CapabilitiesBody(Guid.NewGuid(), ContractVersions.Schema, []), token);
        });
        await using var client = await TapClient.ConnectAsync(server.Name, scope, nonce, Deadline, default);
        var response = await client.SendAsync(TapMessageType.Capabilities,
            new CapabilitiesRequestBody(ContractVersions.Schema), Deadline, default);
        Assert.IsType<CapabilitiesBody>(response.Body);
        await server.Completed.WaitAsync(Deadline);
    }

    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    public async Task Kernel_server_identity_must_match_pid_and_creation_time(bool creationMismatch)
    {
        var scope = Scope();
        scope = new(scope.SessionId, creationMismatch ?
            new(scope.Process.ProcessId, scope.Process.CreationTimeFileTime + 1, scope.Process.InstanceId) :
            new(scope.Process.ProcessId + 1, scope.Process.CreationTimeFileTime, scope.Process.InstanceId));
        await using var server = new PipeFixture((_, _) => Task.CompletedTask);
        await Assert.ThrowsAsync<UnauthorizedAccessException>(() => TapClient.ConnectAsync(server.Name,
            scope, new byte[32], Deadline, default));
    }

    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    public async Task Rejected_nonce_or_mismatched_response_id_cannot_authenticate(bool wrongId)
    {
        await using var server = new PipeFixture(async (pipe, token) =>
        {
            var request = await TapFraming.ReadAsync(pipe, token);
            await TapFraming.WriteAsync(pipe, TapMessageType.Acknowledgement,
                wrongId ? Guid.NewGuid() : request.Header.RequestId,
                new AcknowledgementBody(wrongId ? CommandStatus.Completed : CommandStatus.Rejected,
                    null, wrongId ? null : "Nonce does not match this session."), token);
        });
        if (wrongId)
            await Assert.ThrowsAsync<InvalidDataException>(() =>
                TapClient.ConnectAsync(server.Name, Scope(), new byte[32], Deadline, default));
        else
        {
            var rejected = await Assert.ThrowsAsync<UnauthorizedAccessException>(() =>
                TapClient.ConnectAsync(server.Name, Scope(), new byte[32], Deadline, default));
            Assert.Contains("Nonce does not match this session.", rejected.Message);
        }
    }

    [Fact]
    public async Task Deadline_closes_a_partial_response_instead_of_reusing_desynchronized_transport()
    {
        await using var server = new PipeFixture(async (pipe, token) =>
        {
            var handshake = await TapFraming.ReadAsync(pipe, token);
            await Ack(pipe, handshake, CommandStatus.Completed, null, token);
            var request = await TapFraming.ReadAsync(pipe, token);
            var bytes = TapCodec.Encode(TapMessageType.Acknowledgement, request.Header.RequestId,
                new AcknowledgementBody(CommandStatus.Completed, null, null));
            await pipe.WriteAsync(bytes.AsMemory(0, 3), token);
            await Task.Delay(Timeout.InfiniteTimeSpan, token);
        });
        await using var client = await TapClient.ConnectAsync(server.Name, Scope(), new byte[32], Deadline, default);
        await Assert.ThrowsAsync<TimeoutException>(() => client.SendAsync(TapMessageType.Capabilities,
            new CapabilitiesRequestBody(1), TimeSpan.FromMilliseconds(100), default));
        await Assert.ThrowsAsync<ObjectDisposedException>(() => client.SendAsync(TapMessageType.Capabilities,
            new CapabilitiesRequestBody(1), Deadline, default));
    }

    [Fact]
    public async Task Capture_acknowledgement_cannot_substitute_another_capture()
    {
        await using var server = new PipeFixture(async (pipe, token) =>
        {
            var handshake = await TapFraming.ReadAsync(pipe, token);
            await Ack(pipe, handshake, CommandStatus.Completed, null, token);
            var request = await TapFraming.ReadAsync(pipe, token);
            await Ack(pipe, request, CommandStatus.Accepted, Guid.NewGuid(), token);
        });
        await using var client = await TapClient.ConnectAsync(server.Name, Scope(), new byte[32], Deadline, default);
        await Assert.ThrowsAsync<InvalidDataException>(() => client.SendAsync(TapMessageType.Rundown,
            new RundownBody(Guid.NewGuid(), null, null), Deadline, default));
    }

    [Fact]
    public async Task Pipe_fixture_keeps_transport_open_until_cancelled_handler_finishes()
    {
        var entered = new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);
        var release = new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);
        var server = new PipeFixture(async (pipe, token) =>
        {
            entered.SetResult();
            try { await Task.Delay(Timeout.InfiniteTimeSpan, token); }
            finally
            {
                await release.Task;
                await pipe.FlushAsync(CancellationToken.None);
            }
        });
        Task? disposing = null;
        try
        {
            using var client = new NamedPipeClientStream(".", server.Name, PipeDirection.InOut, PipeOptions.Asynchronous);
            await client.ConnectAsync().WaitAsync(Deadline);
            await entered.Task.WaitAsync(Deadline);
            disposing = server.DisposeAsync().AsTask();
            release.SetResult();
            await disposing.WaitAsync(Deadline);
        }
        finally
        {
            release.TrySetResult();
            if (disposing is null) await server.DisposeAsync();
        }
    }

    private static ValueTask Ack(Stream pipe, TapMessage request, CommandStatus status, Guid? capture,
        CancellationToken token) => TapFraming.WriteAsync(pipe, TapMessageType.Acknowledgement,
            request.Header.RequestId, new AcknowledgementBody(status, capture, null), token);

    private sealed class PipeFixture : IAsyncDisposable
    {
        private readonly CancellationTokenSource stop = new();
        private readonly NamedPipeServerStream pipe;
        public string Name { get; } = TapClient.CreatePipeName();
        public Task Completed { get; }

        public PipeFixture(Func<Stream, CancellationToken, Task> handler)
        {
            pipe = new(Name, PipeDirection.InOut, 1, PipeTransmissionMode.Byte,
                PipeOptions.Asynchronous | PipeOptions.CurrentUserOnly);
            Completed = Serve(handler);
        }
        private async Task Serve(Func<Stream, CancellationToken, Task> handler)
        {
            await pipe.WaitForConnectionAsync(stop.Token);
            await handler(pipe, stop.Token);
        }
        public async ValueTask DisposeAsync()
        {
            stop.Cancel();
            try { await Completed; }
            catch (OperationCanceledException) when (stop.IsCancellationRequested) { }
            finally { pipe.Dispose(); stop.Dispose(); }
        }
    }
}
