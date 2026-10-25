using WinUIProfiler.Contracts;
using Xunit;
#if SAVED_SESSION_VIEWMODEL
using WinUIProfiler.App.ViewModels;
#endif

namespace WinUIProfiler.Engine.Tests;

public sealed class SavedSessionViewModelTests
{
#if !SAVED_SESSION_VIEWMODEL
    [Fact]
    public void Saved_session_view_model_is_implemented() =>
        Assert.Fail("The saved-session WinUI view model has not been implemented.");
#else
    [Fact]
    public async Task Pages_are_bounded_and_pinned_until_explicit_refresh()
    {
        using var fixture = new StoreFixture();
        using var store = fixture.Create();
        var writer = store.CreateWriter();
        writer.Commit(fixture.Batch(1, 300));
        var model = new SavedSessionViewModel();
        await model.OpenAsync(fixture.Path);
        Assert.Equal(fixture.Scope, model.Scope);
        Assert.Equal(256, model.Rows.Count);
        Assert.True(model.CanNext);
        Assert.False(model.CanPrevious);
        writer.Commit(fixture.Batch(301, 305));
        await model.NextAsync();
        Assert.Equal(44, model.Rows.Count);
        Assert.Equal("300", model.Rows[^1].Value);
        Assert.Equal(new(300, 3000), model.Watermark);
        Assert.False(model.CanNext);
        await model.PreviousAsync();
        Assert.Equal(256, model.Rows.Count);
        Assert.False(model.CanPrevious);
        await model.RefreshAsync();
        await model.NextAsync();
        Assert.Equal(49, model.Rows.Count);
        Assert.Equal(new(305, 3050), model.Watermark);
    }

    [Fact]
    public async Task Missing_sample_is_unavailable_not_a_measured_zero()
    {
        using var fixture = new StoreFixture();
        using var store = fixture.Create();
        var batch = fixture.Batch(1, 1);
        var sample = (MetricSample)batch.Changes.Single().Value;
        sample = sample with { Value = new(ValueKind.Missing, ValueUnit.Bytes, null, null, null, null,
            new(MeasurementKind.Unavailable, CoverageState.Partial, "GPU memory is unsupported.")) };
        store.CreateWriter().Commit(batch with { Changes = [new(1, sample)] });
        var model = new SavedSessionViewModel();
        await model.OpenAsync(fixture.Path);
        var row = Assert.Single(model.Rows);
        Assert.Equal("Unavailable", row.Value);
        Assert.Equal("Bytes", row.Unit);
        Assert.Equal("Unavailable / Partial", row.Quality);
        Assert.Equal("GPU memory is unsupported.", row.Reason);
        Assert.Equal("synthetic / 1", row.Source);
    }

    [Fact]
    public async Task Failed_or_cancelled_open_preserves_the_previous_document()
    {
        using var fixture = new StoreFixture();
        using var store = fixture.Create();
        store.CreateWriter().Commit(fixture.Batch(1, 2));
        var model = new SavedSessionViewModel();
        await model.OpenAsync(fixture.Path);
        await Assert.ThrowsAsync<FileNotFoundException>(() => model.OpenAsync(fixture.Path + ".missing"));
        Assert.Equal(fixture.Scope, model.Scope);
        Assert.Equal(2, model.Rows.Count);
        Assert.False(model.IsBusy);
        await Assert.ThrowsAnyAsync<OperationCanceledException>(() => model.OpenAsync(fixture.Path, new(true)));
        Assert.Equal(fixture.Scope, model.Scope);
        Assert.False(model.IsBusy);
        model.ReportError(new IOException("Visible local read failure."));
        Assert.True(model.HasError);
        Assert.Equal("Visible local read failure.", model.ErrorMessage);
    }

    [Fact]
    public async Task Empty_projection_has_explicit_unavailable_quality()
    {
        using var fixture = new StoreFixture();
        using var store = fixture.Create();
        var model = new SavedSessionViewModel();
        await model.OpenAsync(fixture.Path);
        Assert.True(model.HasSession);
        Assert.Empty(model.Rows);
        Assert.Equal(CoverageState.Unavailable, model.MetricQuality?.Coverage);
        Assert.False(model.CanPrevious);
        Assert.False(model.CanNext);
        Assert.True(model.CanRefresh);
    }
#endif
}
