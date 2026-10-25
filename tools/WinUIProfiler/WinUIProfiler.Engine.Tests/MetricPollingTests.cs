using WinUIProfiler.Engine.Ingestion;
using Xunit;

namespace WinUIProfiler.Engine.Tests;

public sealed class MetricPollingTests
{
    [Fact]
    public async Task Polling_runs_off_caller_thread_and_stops_without_another_acquisition()
    {
        var caller = Environment.CurrentManagedThreadId;
        var captured = new TaskCompletionSource<int>(TaskCreationOptions.RunContinuationsAsynchronously);
        var count = 0;
        await using var worker = new MetricPollingWorker(TimeSpan.FromSeconds(30), () =>
        {
            Interlocked.Increment(ref count);
            captured.TrySetResult(Environment.CurrentManagedThreadId);
        });
        worker.Start();
        Assert.NotEqual(caller, await captured.Task.WaitAsync(TimeSpan.FromSeconds(5)));
        await worker.StopAsync();
        var stoppedCount = Volatile.Read(ref count);
        Assert.Equal(1, stoppedCount);
        Assert.True(worker.Completion.IsCompletedSuccessfully);
        Assert.Throws<InvalidOperationException>(worker.Start);
    }

    [Fact]
    public async Task Polling_failure_is_terminal_and_preserves_original_exception()
    {
        var failure = new IOException("Native metric acquisition failed.");
        var count = 0;
        var worker = new MetricPollingWorker(TimeSpan.FromMilliseconds(10), () =>
        {
            Interlocked.Increment(ref count);
            throw failure;
        });
        worker.Start();
        Assert.Same(failure, await Assert.ThrowsAsync<IOException>(
            () => worker.Completion.WaitAsync(TimeSpan.FromSeconds(5))));
        Assert.Equal(1, Volatile.Read(ref count));
        Assert.Same(failure, await Assert.ThrowsAsync<IOException>(() => worker.DisposeAsync().AsTask()));
    }

    [Fact]
    public async Task Stop_waits_for_the_inflight_capture_and_never_overlaps_captures()
    {
        using var release = new ManualResetEventSlim();
        var started = new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);
        var exited = false;
        var worker = new MetricPollingWorker(TimeSpan.FromMilliseconds(1), () =>
        {
            started.SetResult();
            if (!release.Wait(TimeSpan.FromSeconds(5))) throw new TimeoutException("Test capture was not released.");
            exited = true;
        });
        worker.Start();
        await started.Task.WaitAsync(TimeSpan.FromSeconds(5));
        var stopping = worker.StopAsync();
        try
        {
            Assert.False(stopping.IsCompleted);
            Assert.False(exited);
        }
        finally { release.Set(); }
        await stopping.WaitAsync(TimeSpan.FromSeconds(5));
        Assert.True(exited);
        await worker.DisposeAsync();
    }

    [Fact]
    public async Task Polling_deadline_is_independent_of_reader_or_ui_activity()
    {
        var captures = new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);
        var count = 0;
        await using var worker = new MetricPollingWorker(TimeSpan.FromMilliseconds(5), () =>
        {
            if (Interlocked.Increment(ref count) == 3) captures.SetResult();
        });
        worker.Start();
        await captures.Task.WaitAsync(TimeSpan.FromSeconds(5));
        await worker.StopAsync();
        Assert.True(Volatile.Read(ref count) >= 3);
    }

    [Fact]
    public async Task An_unstarted_worker_can_be_stopped_and_disposed_without_running_capture()
    {
        var worker = new MetricPollingWorker(TimeSpan.FromMilliseconds(10),
            () => throw new InvalidOperationException("Must not acquire."));
        await worker.StopAsync();
        await worker.DisposeAsync();
        await worker.DisposeAsync();
        Assert.True(worker.Completion.IsCompletedSuccessfully);
        Assert.Throws<ObjectDisposedException>(worker.Start);
    }

    [Fact]
    public void Invalid_polling_configuration_is_rejected()
    {
        Assert.Throws<ArgumentOutOfRangeException>(() => new MetricPollingWorker(TimeSpan.Zero, () => { }));
        Assert.Throws<ArgumentOutOfRangeException>(() => new MetricPollingWorker(TimeSpan.FromTicks(1), () => { }));
        Assert.Throws<ArgumentOutOfRangeException>(() => new MetricPollingWorker(TimeSpan.FromMilliseconds(-1), () => { }));
        Assert.Throws<ArgumentOutOfRangeException>(() => new MetricPollingWorker(TimeSpan.MaxValue, () => { }));
        Assert.Throws<ArgumentNullException>(() => new MetricPollingWorker(TimeSpan.FromSeconds(1), null!));
    }
}
