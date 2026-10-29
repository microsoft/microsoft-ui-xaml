using System.Globalization;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Controls;
using WinUIProfiler.App.ViewModels;
using WinUIProfiler.Contracts;

namespace WinUIProfiler.App.Views;

public sealed class SnapshotResourceDisplayRow(string metric, string before, string after, string delta, bool comparison)
{
    public string Metric { get; } = metric;
    public string Before { get; } = before;
    public string After { get; } = after;
    public string Delta { get; } = delta;
    public GridLength ComparisonWidth { get; } = new(comparison ? 1 : 0, GridUnitType.Star);
    public Visibility ComparisonVisibility { get; } = comparison ? Visibility.Visible : Visibility.Collapsed;
    public override string ToString() => ComparisonVisibility == Visibility.Visible
        ? $"{Metric}: {Before} -> {After}; {Delta}" : $"{Metric}: {After}";
}

public sealed partial class SnapshotResourceView : UserControl
{
    private readonly Microsoft.Windows.ApplicationModel.Resources.ResourceLoader strings = new();

    public SnapshotResourceView() => InitializeComponent();

    public void Show(SnapshotResourceCapture? before, SnapshotResourceCapture? after, bool comparison)
    {
        var width = new GridLength(comparison ? 1 : 0, GridUnitType.Star);
        BeforeColumn.Width = DeltaColumn.Width = width;
        BeforeHeader.Visibility = DeltaHeader.Visibility = comparison ? Visibility.Visible : Visibility.Collapsed;
        AfterHeader.Text = Text(comparison ? "SnapshotResourceAfter" : "SnapshotResourceValue");
        Status.Visibility = Scope.Visibility = comparison ? Visibility.Collapsed : Visibility.Visible;
        Status.Text = comparison ? Format("SnapshotResourceStatusFormat", CaptureStatus(before), CaptureStatus(after))
            : CaptureStatus(after);
        AutomationProperties.SetItemStatus(Status, Status.Text);
        Values.ItemsSource = new SnapshotResourceComparison(before, after).Rows.Select(row =>
        {
            var label = Text("SnapshotResource" + row.Metric);
            if (row.ResourceIndex is { } index) label = Format("SnapshotResourceGraphicsFormat", index, label);
            if (row.IsBytes) label = Format("SnapshotResourceBytesFormat", label);
            return new SnapshotResourceDisplayRow(label, Value(row.Before), Value(row.After),
                row.Delta is { } delta ? delta.ToString("+#,0;-#,0;0", CultureInfo.CurrentCulture)
                    : Text("SnapshotResourceUnavailable"), comparison);
        }).ToArray();
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
