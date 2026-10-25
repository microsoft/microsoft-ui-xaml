using System.Collections.ObjectModel;
using System.Globalization;
using WinUIProfiler.Contracts;

namespace WinUIProfiler.App.ViewModels;

public sealed record SnapshotHistoryEntry(SavedTreeSnapshot Saved, ProcessInstanceKey Process, DateTimeOffset SavedAtUtc)
{
    public string Title => $"{SavedAtUtc.ToLocalTime().ToString("G", CultureInfo.CurrentCulture)} | PID {Process.ProcessId}";
    public string Summary => $"{Saved.XamlNodes:N0} XAML | {Saved.CompositionNodes:N0} visuals | {Saved.Correlations:N0} links";
    public string CaptureId => Saved.CaptureId.ToString("D");
    public override string ToString() => $"{Title} | {CaptureId}";
}

public sealed class SnapshotHistory
{
    private readonly ObservableCollection<SnapshotHistoryEntry> entries = [];
    public ReadOnlyObservableCollection<SnapshotHistoryEntry> Entries { get; }
    public SnapshotHistory() => Entries = new(entries);

    public SnapshotHistoryEntry Add(SavedTreeSnapshot saved, ProcessInstanceKey process, DateTimeOffset savedAtUtc)
    {
        ArgumentNullException.ThrowIfNull(saved);
        process.Validate();
        Require.Id(saved.CaptureId);
        if (process.CreationTimeFileTime is null || !Path.IsPathFullyQualified(saved.Path) ||
            saved.Roots <= 0 || saved.XamlNodes <= 0 || saved.CompositionNodes <= 0 || saved.Correlations <= 0)
            throw new ArgumentException("Snapshot history requires an identified process and a successfully saved tree.");
        if (entries.Any(e => e.Saved.CaptureId == saved.CaptureId))
            throw new InvalidOperationException("This snapshot is already in the history.");
        var entry = new SnapshotHistoryEntry(saved, process, savedAtUtc.ToUniversalTime());
        entries.Insert(0, entry);
        return entry;
    }
}
