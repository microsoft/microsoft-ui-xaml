using WinUIProfiler.App.ViewModels;
using WinUIProfiler.Engine.Target;
using Xunit;

namespace WinUIProfiler.Engine.Tests;

public sealed class TargetSelectionViewModelTests
{
    private static readonly TargetCandidate Packaged = new("Store sample", "Sample_family!App", TargetLaunchKind.Packaged);
    private static readonly TargetCandidate Process = new("SampleProcess", "12345", TargetLaunchKind.Attach, 123);

    [Theory]
    [InlineData("STORE", "Sample_family!App")]
    [InlineData("family!", "Sample_family!App")]
    [InlineData("12345", "12345")]
    [InlineData("sampleprocess", "12345")]
    public async Task Search_matches_name_and_target_case_insensitively(string search, string expected)
    {
        var model = new TargetSelectionViewModel(_ => Task.FromResult<IReadOnlyList<TargetCandidate>>([Packaged, Process]));
        await model.RefreshAsync();
        model.SearchText = search;
        Assert.Equal(expected, Assert.Single(model.Candidates).Value);
    }

    [Fact]
    public async Task Selection_notifies_only_selection_and_ignores_reentrant_same_value()
    {
        var model = new TargetSelectionViewModel(_ => Task.FromResult<IReadOnlyList<TargetCandidate>>([Packaged]));
        await model.RefreshAsync();
        var notifications = new List<string?>();
        model.PropertyChanged += (_, args) => notifications.Add(args.PropertyName);
        model.Selected = Packaged;
        model.Selected = Packaged;
        Assert.Equal(new[] { nameof(model.Selected), nameof(model.CanSelect) }, notifications);
    }

    [Fact]
    public async Task Refresh_and_filter_clear_stale_selection()
    {
        var model = new TargetSelectionViewModel(_ => Task.FromResult<IReadOnlyList<TargetCandidate>>([Packaged, Process]));
        await model.RefreshAsync();
        model.Selected = Packaged;
        Assert.True(model.CanSelect);
        model.SearchText = "12345";
        Assert.Null(model.Selected);
        model.Selected = Process;
        await model.RefreshAsync();
        Assert.Null(model.Selected);
        Assert.False(model.CanSelect);
    }

    [Fact]
    public async Task Discovery_failure_is_visible_and_refresh_recovers()
    {
        var fail = true;
        var model = new TargetSelectionViewModel(_ => fail
            ? Task.FromException<IReadOnlyList<TargetCandidate>>(new IOException("Discovery failed"))
            : Task.FromResult<IReadOnlyList<TargetCandidate>>([Packaged]));
        await model.RefreshAsync();
        Assert.True(model.HasError);
        Assert.Contains("Discovery failed", model.ErrorMessage);
        Assert.Empty(model.Candidates);
        Assert.False(model.IsBusy);
        fail = false;
        await model.RefreshAsync();
        Assert.False(model.HasError);
        Assert.Single(model.Candidates);
    }

    [Fact]
    public async Task Cancelled_discovery_does_not_publish_late_results_or_an_error()
    {
        using var cancellation = new CancellationTokenSource();
        var pending = new TaskCompletionSource<IReadOnlyList<TargetCandidate>>(TaskCreationOptions.RunContinuationsAsynchronously);
        var model = new TargetSelectionViewModel(_ => pending.Task);
        var refresh = model.RefreshAsync(cancellation.Token);
        Assert.True(model.IsBusy);
        cancellation.Cancel();
        pending.SetResult([Packaged]);
        await refresh;
        Assert.Empty(model.Candidates);
        Assert.False(model.HasError);
        Assert.False(model.IsBusy);
    }

    [Fact]
    public async Task Unverifiable_process_identity_cannot_be_selected()
    {
        TargetCandidate denied = new("Protected", "4", TargetLaunchKind.Attach, Error: "Access denied");
        TargetCandidate unknown = new("Unknown creation time", "42", TargetLaunchKind.Attach);
        var model = new TargetSelectionViewModel(_ => Task.FromResult<IReadOnlyList<TargetCandidate>>([denied, unknown]));
        await model.RefreshAsync();
        model.Selected = denied;
        Assert.False(model.CanSelect);
        model.Selected = unknown;
        Assert.False(model.CanSelect);
    }
}
