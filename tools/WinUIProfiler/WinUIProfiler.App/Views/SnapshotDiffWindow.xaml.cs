using System.Diagnostics;
using System.Globalization;
using System.Text.Json;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Controls;
using Windows.Graphics;
using WinUIProfiler.App.ViewModels;

namespace WinUIProfiler.App.Views;

public sealed record SnapshotDiffTableRow(string Change, SnapshotNodeDiff Difference)
{
    public string Label => (Difference.After ?? Difference.Before!).Label;
    public override string ToString() => $"{Change}: {Label}";
}

public sealed partial class SnapshotDiffWindow : Window
{
    private readonly CancellationTokenSource lifetime = new();
    private readonly Microsoft.Windows.ApplicationModel.Resources.ResourceLoader strings = new();
    private readonly SnapshotHistoryEntry before, after;
    private SnapshotDiffData? data;
    private SnapshotTreeDiffWindow? treeWindow;
    private bool closed;

    public event EventHandler<SnapshotObject>? ObjectRequested;

    public SnapshotDiffWindow(SnapshotHistoryEntry baseline, SnapshotHistoryEntry comparison)
    {
        before = baseline;
        after = comparison;
        Closed += (_, _) =>
        {
            closed = true;
            lifetime.Cancel();
            lifetime.Dispose();
            treeWindow?.Close();
        };
        try
        {
            InitializeComponent();
            Title = Text("SnapshotDiffTitle");
            AppWindow.Resize(new SizeInt32(1520, 1000));
            BeforeIdentity.Text = Format("SnapshotDiffBeforeFormat", before.Process.ProcessId, before.CaptureId);
            AfterIdentity.Text = Format("SnapshotDiffAfterFormat", after.Process.ProcessId, after.CaptureId);
            ResourceSnapshot.TableMaxHeight = double.PositiveInfinity;
            Root.Loaded += OnLoaded;
        }
        catch
        {
            Close();
            throw;
        }
    }

    private string Text(string key) => strings.GetString(key);
    private string Format(string key, params object?[] values) =>
        string.Format(CultureInfo.CurrentCulture, Text(key), values);

    private async void OnLoaded(object sender, RoutedEventArgs args)
    {
        Root.Loaded -= OnLoaded;
        if (closed) return;
        try
        {
            var token = lifetime.Token;
            data = await Task.Run(() => SnapshotDiffData.LoadAsync(before, after, token));
            if (closed) return;
            XamlSummary.Text = Summary(data.Xaml);
            CompositionSummary.Text = Summary(data.Composition);
            AutomationProperties.SetItemStatus(XamlSummary, Counts(data.Xaml));
            AutomationProperties.SetItemStatus(CompositionSummary, Counts(data.Composition));
            AutomationProperties.SetItemStatus(BeforeIdentity, before.CaptureId);
            AutomationProperties.SetItemStatus(AfterIdentity, after.CaptureId);
            XamlChanges.ItemsSource = TableRows(data.Xaml);
            CompositionChanges.ItemsSource = TableRows(data.Composition);
            XamlNoAddedRemoved.Visibility = data.Xaml.AddedOrRemovedNodes.Count == 0 ? Visibility.Visible : Visibility.Collapsed;
            CompositionNoAddedRemoved.Visibility = data.Composition.AddedOrRemovedNodes.Count == 0 ? Visibility.Visible : Visibility.Collapsed;
            ResourceSnapshot.Show(data.Before.Document.ResourceSnapshot, data.After.Document.ResourceSnapshot, true);
            ResourceSnapshot.Visibility = Visibility.Visible;
            ResourceSnapshot.Textures.Changed += (_, _) => UpdateNoDifferences();
            UpdateNoDifferences();
            Viewer.Visibility = Visibility.Visible;
            await ResourceSnapshot.Textures.ShowAsync(before, data.Before, after, data.After, token);
        }
        catch (OperationCanceledException) when (closed) { }
        catch (Exception error) { ReportError(error); }
        finally { if (!closed) Loading.IsActive = false; }
    }

    private void UpdateNoDifferences() =>
        NoDifferences.Visibility = data!.Resources.HasChanges || ResourceSnapshot.Textures.Data?.HasChanges == true ||
            data.Xaml.Nodes.Concat(data.Composition.Nodes).Any(node => node.Change != SnapshotChange.None)
                ? Visibility.Collapsed : Visibility.Visible;

    private string Summary(SnapshotTreeDiff tree) => Format("SnapshotDiffSummaryFormat",
        tree.Count(SnapshotChange.Added), tree.Count(SnapshotChange.Removed), tree.Count(SnapshotChange.Properties),
        tree.Count(SnapshotChange.Moved), tree.Count(SnapshotChange.Reordered));

    private SnapshotDiffTableRow[] TableRows(SnapshotTreeDiff tree) =>
        tree.AddedOrRemovedNodes.Select(node => new SnapshotDiffTableRow(
            Text(node.After is null ? "SnapshotDiffRemoved" : "SnapshotDiffAdded"), node)).ToArray();

    private static string Counts(SnapshotTreeDiff tree) => JsonSerializer.Serialize(new
    {
        added = tree.Count(SnapshotChange.Added),
        removed = tree.Count(SnapshotChange.Removed),
        properties = tree.Count(SnapshotChange.Properties),
        moved = tree.Count(SnapshotChange.Moved),
        reordered = tree.Count(SnapshotChange.Reordered)
    });

    private void OpenTrees_Click(object sender, RoutedEventArgs args) => ShowTrees();

    private void ChangesTable_ItemClick(object sender, ItemClickEventArgs args)
    {
        if (args.ClickedItem is not SnapshotDiffTableRow row) return;
        ShowTrees(ReferenceEquals(sender, XamlChanges) ? SnapshotTreeKind.Xaml : SnapshotTreeKind.Composition,
            row.Difference);
    }

    private void ShowTrees(SnapshotTreeKind kind = SnapshotTreeKind.Xaml, SnapshotNodeDiff? node = null)
    {
        if (data is null || closed) return;
        try
        {
            if (treeWindow is null)
            {
                var viewer = new SnapshotTreeDiffWindow(data);
                treeWindow = viewer;
                viewer.Closed += (_, _) => treeWindow = null;
                viewer.ObjectRequested += (_, requested) => ObjectRequested?.Invoke(this, requested);
            }
            if (node is not null) treeWindow.ShowNode(kind, node);
            treeWindow.Activate();
        }
        catch (Exception error) { ReportError(error); }
    }

    private void ReportError(Exception error)
    {
        Trace.TraceError(error.ToString());
        if (!closed) { LoadError.Message = error.Message; LoadError.IsOpen = true; }
    }
}
