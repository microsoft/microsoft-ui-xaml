using System.Buffers.Binary;
using System.Diagnostics;
using System.IO.Pipes;
using System.Runtime.InteropServices;
using System.Runtime.Versioning;
using System.Security.Cryptography;
using System.Security.Principal;
using WinUIProfiler.Contracts;
using WinUIProfiler.Engine;
using WinUIProfiler.Engine.Target;
using Xunit;

namespace WinUIProfiler.IntegrationTests;

[SupportedOSPlatform("windows")]
public sealed class NativeTapProtocolTests
{
    private static readonly TimeSpan Deadline = TimeSpan.FromSeconds(10);

    [Fact]
    public async Task Native_dll_authenticates_managed_client_and_unloads_after_disconnect()
    {
        await using var host = await Host.Start();
        await using var client = await TapClient.ConnectAsync(host.Pipe, host.Scope, host.Nonce, Deadline, default);
        Assert.Equal("hello", await client.HelloAsync(Deadline, default));
        Assert.Equal("hello", await client.HelloAsync(Deadline, default));
        var capabilities = await client.SendAsync(TapMessageType.Capabilities,
            new CapabilitiesRequestBody(1), Deadline, default);
        Assert.Empty(Assert.IsType<CapabilitiesBody>(capabilities.Body).Capabilities);
        var captureId = Guid.NewGuid();
        var capture = await client.SendAsync(TapMessageType.Rundown,
            new RundownBody(captureId, null, null), Deadline, default);
        var acknowledgement = Assert.IsType<AcknowledgementBody>(capture.Body);
        Assert.Equal(CommandStatus.Unsupported, acknowledgement.Status);
        Assert.Equal(captureId, acknowledgement.CaptureId);
        Assert.False(string.IsNullOrWhiteSpace(acknowledgement.Reason));
        var disconnect = await client.SendAsync(TapMessageType.Disconnect,
            new DisconnectBody("Native/managed protocol qualification completed."), Deadline, default);
        Assert.Equal(CommandStatus.Completed, Assert.IsType<AcknowledgementBody>(disconnect.Body).Status);
        await host.AssertCleanExit();
    }

    [Fact]
    public async Task Native_nonce_rejection_does_not_leave_site_or_worker_references()
    {
        await using var host = await Host.Start();
        var wrong = host.Nonce.ToArray();
        wrong[0] ^= 1;
        await Assert.ThrowsAsync<UnauthorizedAccessException>(() =>
            TapClient.ConnectAsync(host.Pipe, host.Scope, wrong, Deadline, default));
        await host.AssertCleanExit();
    }

    [Fact]
    public async Task Native_kernel_client_creation_check_precedes_body_authentication()
    {
        await using var host = await Host.Start(wrongClientCreation: true);
        using var deadline = new CancellationTokenSource(Deadline);
        await using var pipe = await host.ConnectRaw(deadline.Token);
        await Assert.ThrowsAnyAsync<IOException>(async () =>
            await TapFraming.ReadAsync(pipe, deadline.Token));
        await host.AssertCleanExit();
    }

    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    public async Task Native_duplicate_request_id_is_rejected_without_another_capture(bool hello)
    {
        await using var host = await Host.Start();
        using var deadline = new CancellationTokenSource(Deadline);
        await using var pipe = await host.ConnectRaw(deadline.Token);
        await TapFraming.WriteAsync(pipe, TapMessageType.Handshake, Guid.NewGuid(),
            new HandshakeBody(host.Scope.SessionId, host.Scope.Process, 1,
                Convert.ToBase64String(host.Nonce)), deadline.Token);
        Assert.Equal(CommandStatus.Completed,
            Assert.IsType<AcknowledgementBody>((await TapFraming.ReadAsync(pipe, deadline.Token)).Body).Status);
        var request = Guid.NewGuid();
        for (var i = 0; i < 2; ++i)
        {
            await TapFraming.WriteAsync(pipe, hello ? TapMessageType.Hello : TapMessageType.Capabilities, request,
                hello ? new HelloBody("hello") : new CapabilitiesRequestBody(1), deadline.Token);
            var response = await TapFraming.ReadAsync(pipe, deadline.Token);
            Assert.Equal(request, response.Header.RequestId);
            if (i == 0)
            {
                if (hello)
                {
                    Assert.Equal(TapMessageType.Hello, response.Header.Type);
                    Assert.Equal("hello", Assert.IsType<HelloBody>(response.Body).Message);
                }
                else Assert.IsType<CapabilitiesBody>(response.Body);
            }
            else Assert.Equal(CommandStatus.Rejected, Assert.IsType<AcknowledgementBody>(response.Body).Status);
        }
        await pipe.DisposeAsync();
        await host.AssertCleanExit();
    }

    [Fact]
    public async Task Native_hello_before_handshake_is_rejected_and_worker_unloads()
    {
        await using var host = await Host.Start();
        using var deadline = new CancellationTokenSource(Deadline);
        await using var pipe = await host.ConnectRaw(deadline.Token);
        var request = Guid.NewGuid();
        await TapFraming.WriteAsync(pipe, TapMessageType.Hello, request, new HelloBody("hello"), deadline.Token);
        var response = await TapFraming.ReadAsync(pipe, deadline.Token);
        Assert.Equal(request, response.Header.RequestId);
        Assert.Equal(CommandStatus.Rejected, Assert.IsType<AcknowledgementBody>(response.Body).Status);
        await pipe.DisposeAsync();
        await host.AssertCleanExit();
    }

    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    public async Task Native_invalid_or_truncated_header_releases_worker(bool oversized)
    {
        await using var host = await Host.Start();
        using var deadline = new CancellationTokenSource(Deadline);
        await using var pipe = await host.ConnectRaw(deadline.Token);
        var frame = TapCodec.Encode(TapMessageType.Capabilities, Guid.NewGuid(), new CapabilitiesRequestBody(1));
        if (oversized)
        {
            BinaryPrimitives.WriteUInt32LittleEndian(frame, TapProtocol.MaximumBodyBytes + 1);
            await pipe.WriteAsync(frame.AsMemory(0, TapProtocol.HeaderBytes), deadline.Token);
            await Assert.ThrowsAnyAsync<IOException>(async () =>
            {
                await TapFraming.ReadAsync(pipe, deadline.Token);
            });
        }
        else
        {
            await pipe.WriteAsync(frame.AsMemory(0, 3), deadline.Token);
        }
        await pipe.DisposeAsync();
        await host.AssertCleanExit();
    }

    [Fact]
    public async Task Native_session_identity_rejects_another_authenticated_scope()
    {
        await using var host = await Host.Start();
        var wrongScope = new SessionScope(Guid.NewGuid(), host.Scope.Process);
        await Assert.ThrowsAsync<UnauthorizedAccessException>(() =>
            TapClient.ConnectAsync(host.Pipe, wrongScope, host.Nonce, Deadline, default));
        await host.AssertCleanExit();
    }

    private sealed class Host : IAsyncDisposable
    {
        private readonly Process process;
        private readonly Task<string> errors;
        public string Pipe { get; } = TapClient.CreatePipeName();
        public byte[] Nonce { get; } = RandomNumberGenerator.GetBytes(32);
        public SessionScope Scope { get; }

        private Host(Process process)
        {
            this.process = process;
            errors = process.StandardError.ReadToEndAsync();
            Scope = new(Guid.NewGuid(), new((uint)process.Id,
                process.StartTime.ToFileTimeUtc(), Guid.NewGuid()));
        }

        public static async Task<Host> Start(bool wrongClientCreation = false)
        {
            var root = Path.GetFullPath(Path.Combine(AppContext.BaseDirectory, @"..\..\..\..\..\.."));
            var architecture = RuntimeInformation.ProcessArchitecture == Architecture.Arm64 ? "arm64" : "x64";
            var executable = Path.Combine(root,
                @"tools\WinUIProfiler\WinUIProfiler.Native.Tests\bin\TapHost", architecture, "TapHost.exe");
            Assert.True(File.Exists(executable), $"Build the architecture-matched native TAP test host: {executable}");
            var process = Process.Start(new ProcessStartInfo(executable)
            {
                RedirectStandardInput = true,
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                UseShellExecute = false
            }) ?? throw new InvalidOperationException("Native TAP test host did not start.");
            var host = new Host(process);
            try
            {
                using var current = Process.GetCurrentProcess();
                var creation = current.StartTime.ToFileTimeUtc() + (wrongClientCreation ? 1 : 0);
                var bootstrap = $"WUP1|{host.Pipe}|{host.Scope.SessionId:D}|{host.Scope.Process.ProcessId}|" +
                    $"{host.Scope.Process.CreationTimeFileTime}|{host.Scope.Process.InstanceId:D}|" +
                    $"{current.Id}|{creation}|{Guid.NewGuid():D}|{Convert.ToHexString(host.Nonce)}";
                await process.StandardInput.WriteLineAsync(bootstrap);
                await process.StandardInput.FlushAsync();
                Assert.Equal("READY", await process.StandardOutput.ReadLineAsync().WaitAsync(Deadline));
                return host;
            }
            catch
            {
                await host.DisposeAsync();
                throw;
            }
        }

        public async Task AssertCleanExit()
        {
            await process.WaitForExitAsync().WaitAsync(Deadline);
            Assert.True(process.ExitCode == 0, $"Native TAP host exit {process.ExitCode}: {await errors}");
            Assert.Equal("UNLOADED", await process.StandardOutput.ReadLineAsync().WaitAsync(Deadline));
        }

        public async Task<NamedPipeClientStream> ConnectRaw(CancellationToken cancellationToken)
        {
            var pipe = new NamedPipeClientStream(".", Pipe, PipeDirection.InOut,
                PipeOptions.Asynchronous | PipeOptions.CurrentUserOnly, TokenImpersonationLevel.Identification);
            try
            {
                await pipe.ConnectAsync(cancellationToken);
                return pipe;
            }
            catch
            {
                await pipe.DisposeAsync();
                throw;
            }
        }

        public async ValueTask DisposeAsync()
        {
            if (!process.HasExited) process.Kill(entireProcessTree: true);
            await process.WaitForExitAsync();
            process.Dispose();
        }
    }
}
