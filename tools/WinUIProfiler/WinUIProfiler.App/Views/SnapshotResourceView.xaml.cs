using System.Globalization;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Controls;
using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Processing;

namespace WinUIProfiler.App.Views;

public sealed class SnapshotResourceDisplayValue(string before, string after, string delta, bool comparison,
    string heading = "")
{
    public string Before { get; } = before;
    public string After { get; } = after;
    public string Delta { get; } = delta;
    public string Heading { get; } = heading;
    public Visibility HeadingVisibility { get; } = heading.Length == 0 ? Visibility.Collapsed : Visibility.Visible;
    public GridLength ComparisonWidth { get; } = new(comparison ? 1 : 0, GridUnitType.Star);
    public Visibility ComparisonVisibility { get; } = comparison ? Visibility.Visible : Visibility.Collapsed;
    public override string ToString() => ComparisonVisibility == Visibility.Visible ? $"{Before} -> {After}; {Delta}" : After;
}

public sealed class SnapshotResourceDisplayRow(string metric, SnapshotResourceDisplayValue created,
    SnapshotResourceDisplayValue? destroyed, SnapshotResourceDisplayValue? live, bool comparison)
{
    public string Metric { get; } = metric;
    public SnapshotResourceDisplayValue Created { get; } = created;
    public SnapshotResourceDisplayValue? Destroyed { get; } = destroyed;
    public SnapshotResourceDisplayValue? Live { get; } = live;
    public GridLength ValueWidth { get; } = new(comparison ? 3 : 1, GridUnitType.Star);
    public GridLength LifetimeWidth { get; } = new(destroyed is null ? 0 : comparison ? 3 : 1, GridUnitType.Star);
    public Visibility LifetimeVisibility { get; } = destroyed is null ? Visibility.Collapsed : Visibility.Visible;
    public override string ToString() => Destroyed is null
        ? $"{Metric}: {Created}" : $"{Metric}: {Created}; {Destroyed}; {Live}";
}

public sealed record SnapshotResourceDisplayGroup(string Header, string Note, string AutomationId,
    SnapshotResourceDisplayRow Columns, IReadOnlyList<SnapshotResourceDisplayRow> Rows, string EmptyText)
{
    public Visibility EmptyVisibility => Rows.Count == 0 ? Visibility.Visible : Visibility.Collapsed;
    public Visibility ValuesVisibility => Rows.Count == 0 ? Visibility.Collapsed : Visibility.Visible;
}

public sealed partial class SnapshotResourceView : UserControl
{
    private readonly Microsoft.Windows.ApplicationModel.Resources.ResourceLoader strings = new();

    public SnapshotResourceView() => InitializeComponent();

    public void Show(SnapshotResourceCapture? before, SnapshotResourceCapture? after, bool comparison)
    {
        Status.Visibility = Visibility.Visible;
        Status.Text = comparison ? Format("SnapshotResourceStatusFormat", CaptureStatus(before), CaptureStatus(after))
            : CaptureStatus(after);
        AutomationProperties.SetItemStatus(Status, Status.Text);
        var resources = new SnapshotResourceComparison(before, after);
        Groups.ItemsSource = resources.Groups.Select(group =>
        {
            var key = "SnapshotResource" + group.Kind;
            return new SnapshotResourceDisplayGroup(Text(key + "Heading"), Text(key + "Scope"),
                group.Kind + "Group", Columns(group.Kind == SnapshotResourceGroupKind.Allocations,
                    group.Kind == SnapshotResourceGroupKind.Wrappers, comparison),
                group.Entries.Select(entry => DisplayRow(entry, comparison)).ToArray(), Text("SnapshotResourceGraphicsEmpty"));
        }).ToArray();
        UnknownGraphics.Visibility = resources.UnknownResources.Count == 0 ? Visibility.Collapsed : Visibility.Visible;
        UnknownColumns.Content = Columns(false, true, comparison);
        UnknownValues.ItemsSource = resources.UnknownResources.Select(entry => DisplayRow(entry, comparison)).ToArray();
    }

    private SnapshotResourceDisplayRow Columns(bool allocations, bool lifetime, bool comparison)
    {
        SnapshotResourceDisplayValue Header(string key) => new(Text("SnapshotResourceBefore"), Text(
            comparison ? "SnapshotResourceAfter" : key), Text("SnapshotResourceDelta"), comparison,
            comparison ? Text(key) : "");
        return new(Text(allocations ? "SnapshotResourceMetric" : "SnapshotResourceType"),
            Header(allocations ? "SnapshotResourceValue" : "SnapshotResourceTotalCreated"),
            lifetime ? Header("SnapshotResourceTotalDestroyed") : null,
            lifetime ? Header("SnapshotResourceLiveCount") : null, comparison);
    }

    private SnapshotResourceDisplayRow DisplayRow(SnapshotResourceEntry entry, bool comparison)
    {
        var metric = entry.Metrics[0];
        var label = entry.ResourceName is { } name ? name[(name.IndexOf('.') + 1)..]
            : entry.ResourceIndex is { } index ? Format("SnapshotResourceUnknownFormat", index)
            : metric.IsBytes ? Format("SnapshotResourceBytesFormat", Text("SnapshotResource" + metric.Metric))
            : Text("SnapshotResource" + metric.Metric);
        SnapshotResourceDisplayValue Display(SnapshotResourceRow row) => new(Value(row.Before), Value(row.After),
            row.Delta is { } delta ? delta.ToString("+#,0;-#,0;0", CultureInfo.CurrentCulture)
                : Text("SnapshotResourceUnavailable"), comparison);
        return new(label, Display(metric), entry.Metrics.Count > 1 ? Display(entry.Metrics[1]) : null,
            entry.Metrics.Count > 2 ? Display(entry.Metrics[2]) : null, comparison);
    }

    private string CaptureStatus(SnapshotResourceCapture? capture) => capture is null
        ? Text("SnapshotResourceNotRecorded")
        : capture.Status == "available" ? Text("SnapshotResourceAvailable")
        : Format(capture.Status == "unavailable" ? "SnapshotResourceUnsupported" : "SnapshotResourceFailed",
            $"0x{unchecked((uint)capture.HResult):X8}");

    private string Value(decimal? value) => value?.ToString("N0", CultureInfo.CurrentCulture)
        ?? Text("SnapshotResourceUnavailable");
    private string Text(string key) => strings.GetString(key);
    private string Format(string key, params object?[] values) =>
        string.Format(CultureInfo.CurrentCulture, Text(key), values);
}
