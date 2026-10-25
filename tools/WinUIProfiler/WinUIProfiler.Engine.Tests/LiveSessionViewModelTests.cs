using System.Runtime.Versioning;
using WinUIProfiler.App.ViewModels;
using WinUIProfiler.Engine.Target;
using Xunit;

namespace WinUIProfiler.Engine.Tests;

[SupportedOSPlatform("windows")]
public sealed class LiveSessionViewModelTests
{
    [Fact]
    public async Task Concurrent_stop_callers_share_pending_connection_drain()
    {
        var model = new LiveSessionViewModel();
        var pending = new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);
        typeof(LiveSessionViewModel).GetField("connecting",
            System.Reflection.BindingFlags.NonPublic | System.Reflection.BindingFlags.Instance)!.SetValue(model, pending.Task);
        var first = model.StopAsync();
        var second = model.StopAsync();
        Assert.False(first.IsCompleted);
        Assert.Same(first, second);
        Assert.False(second.IsCompleted);
        pending.SetResult();
        await Task.WhenAll(first, second).WaitAsync(TimeSpan.FromSeconds(5));
        Assert.Equal("Disconnected", model.State);
        Assert.True(model.CanConnect);
    }

    [Fact]
    public async Task Initial_state_and_stop_without_connection_are_disconnected()
    {
        var model = new LiveSessionViewModel();
        Assert.True(model.CanConnect);
        Assert.False(model.CanDisconnect);
        Assert.False(model.HasError);
        await model.StopAsync();
        Assert.Equal("Disconnected", model.State);
        model.Refresh();
        Assert.Equal("", model.DatabasePath);
    }

    [Fact]
    public async Task Failed_connect_surfaces_error_and_restores_controls_without_success()
    {
        var model = new LiveSessionViewModel();
        var notifications = 0;
        model.PropertyChanged += (_, _) => ++notifications;
        await model.ConnectAsync(new(TargetLaunchKind.Unpackaged, @"C:\missing.exe", "", null, "", "", false));
        Assert.Equal("Failed", model.State);
        Assert.True(model.HasError);
        Assert.Contains("trust", model.ErrorMessage, StringComparison.OrdinalIgnoreCase);
        Assert.True(model.CanConnect);
        Assert.False(model.CanDisconnect);
        Assert.False(model.IsBusy);
        Assert.True(notifications > 0);
    }
}
