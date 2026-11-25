using WinUIProfiler.Contracts;

namespace WinUIProfiler.App.ViewModels;

public sealed class SnapshotHistoryEntry(SavedTreeSnapshot saved, ProcessInstanceKey process,
    DateTimeOffset savedAtUtc, string title, string summary)
{
    public SavedTreeSnapshot Saved { get; } = saved;
    public ProcessInstanceKey Process { get; } = process;
    public DateTimeOffset SavedAtUtc { get; } = savedAtUtc;
    public string Title { get; } = title;
    public string Summary { get; } = summary;
    public string CaptureId => Saved.CaptureId.ToString("D");
    public override string ToString() => $"{Title} | {CaptureId}";
}
