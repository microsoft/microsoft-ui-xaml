using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Runtime.Versioning;
using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Target;
using Xunit;

namespace WinUIProfiler.Engine.Tests;

[SupportedOSPlatform("windows")]
public sealed class InjectorResolverTests
{
    [Theory]
    [InlineData(0, 0x8664, Architecture.X64)]
    [InlineData(0, 0xAA64, Architecture.Arm64)]
    [InlineData(0x8664, 0xAA64, Architecture.X64)]
    public void Actual_machine_uses_emulated_process_architecture_before_native_OS(
        ushort process, ushort native, Architecture expected) =>
        Assert.Equal(expected, InjectorResolver.ResolveMachine(process, native));

    [Theory]
    [InlineData(0x014C, 0x8664)]
    [InlineData(0, 0)]
    [InlineData(0xA641, 0xAA64)]
    public void Unsupported_machine_is_not_silently_treated_as_x64(ushort process, ushort native) =>
        Assert.Throws<PlatformNotSupportedException>(() => InjectorResolver.ResolveMachine(process, native));

    [Fact]
    public void Live_target_requires_exact_creation_identity()
    {
        using var current = Process.GetCurrentProcess();
        var target = new ProcessInstanceKey((uint)current.Id, current.StartTime.ToFileTimeUtc(), Guid.NewGuid());
        Assert.Equal(RuntimeInformation.ProcessArchitecture, InjectorResolver.GetArchitecture(target));
        Assert.Throws<UnauthorizedAccessException>(() => InjectorResolver.GetArchitecture(
            new(target.ProcessId, target.CreationTimeFileTime + 1, target.InstanceId)));
    }

    [Fact]
    public void Native_payload_resolution_requires_explicit_local_binary_trust()
    {
        using var current = Process.GetCurrentProcess();
        var target = new ProcessInstanceKey((uint)current.Id, current.StartTime.ToFileTimeUtc(), Guid.NewGuid());
        Assert.Throws<UnauthorizedAccessException>(() =>
            InjectorResolver.Resolve(target, new("missing", "missing", "missing", "missing"), false));
    }

    [Fact]
    public void Bootstrap_preserves_full_integer_identity_and_nonce_without_command_line_arguments()
    {
        var target = new ProcessInstanceKey(uint.MaxValue, long.MaxValue, Guid.NewGuid());
        var client = new ProcessInstanceKey(1, long.MaxValue - 1, Guid.NewGuid());
        var scope = new SessionScope(Guid.NewGuid(), target);
        var pipe = TapClient.CreatePipeName();
        var nonce = Enumerable.Range(0, 32).Select(value => (byte)value).ToArray();
        var bootstrap = InjectorResolver.CreateBootstrap(pipe, scope, client, nonce);
        Assert.Equal($"WUP1|{pipe}|{scope.SessionId:D}|4294967295|9223372036854775807|" +
            $"{target.InstanceId:D}|1|9223372036854775806|{client.InstanceId:D}|{Convert.ToHexString(nonce)}", bootstrap);
    }

    [Fact]
    public void Bootstrap_rejects_nonlocal_names_and_incomplete_process_identity()
    {
        var target = new ProcessInstanceKey(1, 2, Guid.NewGuid());
        var scope = new SessionScope(Guid.NewGuid(), target);
        Assert.Throws<ArgumentException>(() => InjectorResolver.CreateBootstrap(@"\\remote\pipe\profiler",
            scope, target, new byte[32]));
        Assert.Throws<ArgumentException>(() => InjectorResolver.CreateBootstrap(TapClient.CreatePipeName(),
            new(scope.SessionId, new(1, null, target.InstanceId)), target, new byte[32]));
        Assert.Throws<ArgumentException>(() => InjectorResolver.CreateBootstrap(TapClient.CreatePipeName(),
            scope, target, new byte[31]));
    }
}
