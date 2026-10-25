using System.ComponentModel;
using System.Globalization;
using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Storage;

namespace WinUIProfiler.App.ViewModels;

public sealed class SavedSessionViewModel : INotifyPropertyChanged
{
    public const int PageSize = 256;
    private Document? document;
    public event PropertyChangedEventHandler? PropertyChanged;
    public bool IsBusy { get; private set; }
    public bool HasSession => document is not null;
    public bool CanOpen => !IsBusy;
    public bool CanRefresh => HasSession && !IsBusy;
    public bool CanPrevious => CanRefresh && document!.Metrics.Page.Offset > 0;
    public bool CanNext => CanRefresh && document!.Metrics.Page.HasMore;
    public SessionScope? Scope => document?.Info.Context.Scope;
    public CommittedWatermark? Watermark => document?.Info.Context.Watermark;
    public DataQuality? MetricQuality => document?.Metrics.Context.Quality;
    public IReadOnlyList<MetricRow> Rows => document?.Rows ?? [];
    public string SessionPath => document?.Path ?? "";
    public string SessionId => Scope?.SessionId.ToString("D") ?? "";
    public string ProcessId => Scope?.Process.ProcessId.ToString(CultureInfo.InvariantCulture) ?? "";
    public string ProcessInstanceId => Scope?.Process.InstanceId.ToString("D") ?? "";
    public string CreationFileTime => Scope?.Process.CreationTimeFileTime?.ToString(CultureInfo.InvariantCulture) ?? "Unavailable";
    public string Mode => document?.Info.Value.Mode.ToString() ?? "";
    public string Status => document?.Info.Value.Status.ToString() ?? "";
    public string StopReason => document?.Info.Value.StopReason ?? "";
    public string QpcFrequency => document?.Info.Value.QpcFrequency.ToString(CultureInfo.InvariantCulture) ?? "";
    public string CommittedOrdinal => Watermark?.IngestOrdinal.ToString(CultureInfo.InvariantCulture) ?? "";
    public string CommittedQpc => Watermark?.TimestampQpc.ToString(CultureInfo.InvariantCulture) ?? "";
    public string MetricCoverage => MetricQuality is { } quality ? $"{quality.Kind} / {quality.Coverage}" : "";
    public string CoverageReason => MetricQuality?.Reason ?? "";
    public string PageOffset => document?.Metrics.Page.Offset.ToString(CultureInfo.InvariantCulture) ?? "";
    public string ErrorMessage { get; private set; } = "";
    public bool HasError => ErrorMessage.Length > 0;

    public Task OpenAsync(string path, CancellationToken cancellationToken = default)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(path);
        var fullPath = Path.GetFullPath(path);
        return LoadAsync(() => ReadAsync(fullPath, null, 0, cancellationToken), cancellationToken);
    }

    public Task NextAsync(CancellationToken cancellationToken = default)
    {
        if (!CanNext) throw new InvalidOperationException("No next committed page is available.");
        var selected = document!;
        return LoadAsync(() => ReadAsync(selected.Path, selected, selected.Metrics.Page.NextOffset!.Value,
            cancellationToken), cancellationToken);
    }

    public Task PreviousAsync(CancellationToken cancellationToken = default)
    {
        if (!CanPrevious) throw new InvalidOperationException("No previous committed page is available.");
        var selected = document!;
        return LoadAsync(() => ReadAsync(selected.Path, selected, Math.Max(0, selected.Metrics.Page.Offset - PageSize),
            cancellationToken), cancellationToken);
    }

    public Task RefreshAsync(CancellationToken cancellationToken = default)
    {
        if (!CanRefresh) throw new InvalidOperationException("No saved session is open for refresh.");
        var selected = document!;
        return LoadAsync(async () =>
        {
            var current = await ReadAsync(selected.Path, null, 0, cancellationToken).ConfigureAwait(false);
            if (current.Info.Context.Scope != selected.Info.Context.Scope)
                throw new InvalidDataException("The session file was replaced. Open it explicitly to select the new process instance.");
            return current;
        }, cancellationToken);
    }

    public void ReportError(Exception error)
    {
        ArgumentNullException.ThrowIfNull(error);
        ErrorMessage = error.Message;
        System.Diagnostics.Trace.TraceError(error.ToString());
        Notify();
    }

    private async Task LoadAsync(Func<Task<Document>> read, CancellationToken cancellationToken)
    {
        if (IsBusy) throw new InvalidOperationException("A saved-session query is already running.");
        IsBusy = true;
        ErrorMessage = "";
        Notify();
        try
        {
            var loaded = await Task.Run(read, cancellationToken);
            cancellationToken.ThrowIfCancellationRequested();
            document = loaded;
        }
        finally
        {
            IsBusy = false;
            Notify();
        }
    }

    private static async Task<Document> ReadAsync(string path, Document? pinned, long offset, CancellationToken cancellationToken)
    {
        cancellationToken.ThrowIfCancellationRequested();
        using var store = SessionStore.OpenReadOnly(path);
        if (pinned is not null && store.Scope != pinned.Info.Context.Scope)
            throw new InvalidDataException("The selected session identity changed between pages.");
        var info = pinned?.Info ?? await store.Queries.GetSession(new(store.Scope), cancellationToken).ConfigureAwait(false);
        var watermark = info.Context.Watermark;
        if (watermark.TimestampQpc == long.MaxValue)
            throw new NotSupportedException("This session timestamp cannot be represented by a half-open metric query range.");
        var page = await store.Queries.GetMetrics(new(store.Scope, new(0, watermark.TimestampQpc + 1),
            new(PageSize, offset, watermark)), cancellationToken).ConfigureAwait(false);
        return new(path, info, page, Array.AsReadOnly(page.Items.Select(MetricRow.FromSample).ToArray()));
    }

    private void Notify() => PropertyChanged?.Invoke(this, new(null));
    private sealed record Document(string Path, QueryResult<SessionInfo> Info, QueryPage<MetricSample> Metrics,
        IReadOnlyList<MetricRow> Rows);
}

public sealed record MetricRow(string TimestampQpc, string Metric, string Value, string Unit, string Quality,
    string Reason, string Source, string GpuScope, string SourceObservation)
{
    public static MetricRow FromSample(MetricSample sample)
    {
        sample.Value.Validate();
        var value = sample.Value;
        var formatted = value.Kind switch
        {
            ValueKind.Integer => value.Integer!.Value.ToString(CultureInfo.InvariantCulture),
            ValueKind.Number => value.Number!.Value.ToString("R", CultureInfo.InvariantCulture),
            ValueKind.Boolean => value.Boolean!.Value.ToString(),
            ValueKind.Text => value.Text!,
            ValueKind.Missing => MeasurementKind.Unavailable.ToString(),
            _ => throw new InvalidDataException("Unknown metric value discriminator.")
        };
        return new(sample.TimestampQpc.ToString(CultureInfo.InvariantCulture), sample.Metric.ToString(), formatted,
            value.Unit.ToString(), $"{value.Quality.Kind} / {value.Quality.Coverage}", value.Quality.Reason ?? "",
            sample.Source + " / " + sample.SourceVersion, sample.Gpu?.ToString() ?? "", sample.SourceObservation?.ToString() ?? "");
    }
}
