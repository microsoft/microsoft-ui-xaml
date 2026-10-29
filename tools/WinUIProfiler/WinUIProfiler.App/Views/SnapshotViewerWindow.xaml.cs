using System.ComponentModel;
using System.Diagnostics;
using System.Globalization;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Input;
using Windows.Graphics;
using WinUIProfiler.App.ViewModels;

namespace WinUIProfiler.App.Views;

public class SnapshotTreeLabel(string label, SnapshotTreeNode? node = null) : INotifyPropertyChanged
{
    public string Label { get; } = label;
    public SnapshotTreeNode? Node { get; } = node;
    public Visibility MatchVisibility { get; private set; } = Visibility.Collapsed;
    public event PropertyChangedEventHandler? PropertyChanged;
    public override string ToString() => Label;

    public void SetMatch(bool value)
    {
        var visibility = value ? Visibility.Visible : Visibility.Collapsed;
        if (MatchVisibility == visibility) return;
        MatchVisibility = visibility;
        PropertyChanged?.Invoke(this, new(nameof(MatchVisibility)));
    }
}

public sealed partial class SnapshotViewerWindow : Window
{
    private readonly CancellationTokenSource lifetime = new();
    private readonly Microsoft.Windows.ApplicationModel.Resources.ResourceLoader strings = new();
    private readonly Dictionary<ulong, TreeViewNode> xamlNodes = [], visualNodes = [];
    private readonly Dictionary<ulong, TreeViewNode> xamlRoots = [], visualRoots = [];
    private readonly Dictionary<TreeView, TreeViewNode?> synchronizedSelections = [];
    private HashSet<ulong> xamlMatches = [], visualMatches = [];
    private SnapshotTreeKind matchTarget;
    private bool synchronizing;
    private bool changingExpansion;
    private bool closed;
    public SnapshotHistoryEntry Entry { get; }
    public SnapshotViewerData? Data { get; private set; }
    public event EventHandler<SnapshotElement>? ElementRequested;

    public SnapshotViewerWindow(SnapshotHistoryEntry entry)
    {
        Entry = entry;
        Closed += (_, _) => { closed = true; lifetime.Cancel(); lifetime.Dispose(); };
        try
        {
            InitializeComponent();
            Title = Format("SnapshotViewerTitleFormat", entry.CaptureId);
            AppWindow.Resize(new SizeInt32(1440, 960));
            Root.Loaded += OnLoaded;
        }
        catch
        {
            Close();
            throw;
        }
    }

    private string Format(string key, params object?[] values) =>
        string.Format(CultureInfo.CurrentCulture, strings.GetString(key), values);

    private async void OnLoaded(object sender, RoutedEventArgs args)
    {
        Root.Loaded -= OnLoaded;
        if (closed) return;
        try
        {
            var token = lifetime.Token;
            Data = await Task.Run(() => SnapshotViewerData.LoadAsync(Entry, token));
            if (closed) return;
            AddRoots(XamlTree, SnapshotTreeKind.Xaml);
            AddRoots(CompositionTree, SnapshotTreeKind.Composition);
            AutomationProperties.SetItemStatus(Identity, Entry.CaptureId);
            AutomationProperties.SetItemStatus(XamlTree, Data.Xaml.Nodes.Count.ToString(CultureInfo.InvariantCulture));
            AutomationProperties.SetItemStatus(CompositionTree, Data.Composition.Nodes.Count.ToString(CultureInfo.InvariantCulture));
            ResourceSnapshot.Show(null, Data.Document.ResourceSnapshot, false);
            ResourceSnapshot.Visibility = Visibility.Visible;
            Viewer.Visibility = Visibility.Visible;
        }
        catch (OperationCanceledException) when (closed) { }
        catch (Exception error)
        {
            Trace.TraceError(error.ToString());
            if (!closed) { LoadError.Message = error.Message; LoadError.IsOpen = true; }
        }
        finally { if (!closed) Loading.IsActive = false; }
    }

    private void AddRoots(TreeView tree, SnapshotTreeKind kind)
    {
        var roots = kind == SnapshotTreeKind.Xaml ? xamlRoots : visualRoots;
        var data = kind == SnapshotTreeKind.Xaml ? Data!.Xaml : Data!.Composition;
        var rootNodes = data.Roots.ToLookup(node => node.RootId);
        foreach (var root in Data!.Document.Roots)
        {
            var group = new TreeViewNode
            {
                Content = new SnapshotTreeLabel(Format("SnapshotRootFormat", root.XamlRootAddress, root.ThreadId))
            };
            roots.Add(root.Id, group);
            foreach (var node in rootNodes[root.Id]) group.Children.Add(CreateNode(kind, node));
            tree.RootNodes.Add(group);
            group.IsExpanded = Data.Document.Roots.Count == 1;
        }
    }

    private TreeViewNode CreateNode(SnapshotTreeKind kind, SnapshotTreeNode model)
    {
        var label = new SnapshotTreeLabel(model.Label, model);
        label.SetMatch((kind == SnapshotTreeKind.Xaml ? xamlMatches : visualMatches).Contains(model.Id));
        var node = new TreeViewNode { Content = label, HasUnrealizedChildren = model.Children.Count > 0 };
        (kind == SnapshotTreeKind.Xaml ? xamlNodes : visualNodes).Add(model.Id, node);
        return node;
    }

    private void Populate(TreeViewNode node, SnapshotTreeKind kind)
    {
        if (!node.HasUnrealizedChildren || node.Content is not SnapshotTreeLabel { Node: { } model }) return;
        foreach (var child in model.Children) node.Children.Add(CreateNode(kind, child));
        node.HasUnrealizedChildren = false;
    }

    private void Tree_Expanding(TreeView sender, TreeViewExpandingEventArgs args) =>
        Populate(args.Node, sender == XamlTree ? SnapshotTreeKind.Xaml : SnapshotTreeKind.Composition);

    // Double-clicking a XAML node asks the main window to show its element's history in the Elements tab.
    private void XamlTree_DoubleTapped(object sender, DoubleTappedRoutedEventArgs args)
    {
        if (Data is null || SnapshotTreeExpansion.NodeAt(XamlTree, args.OriginalSource)?.Content is not SnapshotTreeLabel { Node: { } node }) return;
        args.Handled = true;
        ElementRequested?.Invoke(this, SnapshotElement.From(Data.Document, node));
    }

    private async void ExpandAll_Click(object sender, RoutedEventArgs args) =>
        await SetExpandedAsync(ReferenceEquals(sender, XamlExpandAll) ? XamlTree : CompositionTree, true);

    private async void CollapseAll_Click(object sender, RoutedEventArgs args) =>
        await SetExpandedAsync(ReferenceEquals(sender, XamlCollapseAll) ? XamlTree : CompositionTree, false);

    private async Task SetExpandedAsync(TreeView tree, bool expanded)
    {
        if (closed || changingExpansion) return;
        changingExpansion = true;
        Control[] controls = [XamlTree, CompositionTree, Matches, XamlExpandAll, XamlCollapseAll,
            CompositionExpandAll, CompositionCollapseAll];
        try
        {
            foreach (var control in controls) control.IsEnabled = false;
            var kind = tree == XamlTree ? SnapshotTreeKind.Xaml : SnapshotTreeKind.Composition;
            await SnapshotTreeExpansion.SetExpandedAsync(tree, expanded, node => Populate(node, kind), lifetime.Token);
        }
        catch (Exception error) { ReportInteractionError(error); }
        finally
        {
            changingExpansion = false;
            if (!closed)
            {
                foreach (var control in controls) control.IsEnabled = true;
                UpdateDetails();
            }
        }
    }

    private void Tree_SelectionChanged(TreeView sender, TreeViewSelectionChangedEventArgs args)
    {
        if (synchronizing || changingExpansion || Data is null) return;
        if (synchronizedSelections.TryGetValue(sender, out var expected))
        {
            if (sender.SelectedNode == expected) return;
            synchronizedSelections.Remove(sender);
        }
        var kind = sender == XamlTree ? SnapshotTreeKind.Xaml : SnapshotTreeKind.Composition;
        var selected = sender.SelectedNode?.Content as SnapshotTreeLabel;
        try
        {
            synchronizing = true;
            var selection = Data.SelectNodes(kind, selected?.Node is { } node ? [node.Id] : []);
            Highlight(SnapshotTreeKind.Xaml, selection.XamlIds);
            Highlight(SnapshotTreeKind.Composition, selection.VisualIds);
            var target = kind == SnapshotTreeKind.Xaml ? CompositionTree : XamlTree;
            matchTarget = kind == SnapshotTreeKind.Xaml ? SnapshotTreeKind.Composition : SnapshotTreeKind.Xaml;
            var targetIds = kind == SnapshotTreeKind.Xaml ? selection.VisualIds : selection.XamlIds;
            var targetData = matchTarget == SnapshotTreeKind.Xaml ? Data.Xaml : Data.Composition;
            Matches.ItemsSource = targetIds.Select(id => targetData.Nodes[id]).ToArray();
            Matches.SelectedIndex = targetIds.Count > 0 ? 0 : -1;
            SetSynchronizedSelection(target, targetIds.Count > 0 ? Reveal(matchTarget, targetIds[0]) : null);
            AutomationProperties.SetItemStatus(Matches, $"target={matchTarget};nodes={string.Join(",", targetIds)}");
            MappingStatus.Text = selected?.Node is null ? strings.GetString("SnapshotSelectNode") :
                targetIds.Count == 0 ? strings.GetString("SnapshotNoMapping") :
                Format("SnapshotMatchesFormat", targetIds.Count);
            AutomationProperties.SetItemStatus(MappingStatus,
                $"xaml={string.Join(",", selection.XamlIds)};visual={string.Join(",", selection.VisualIds)}");
            UpdateDetails();
        }
        catch (Exception error) { ReportInteractionError(error); }
        finally { synchronizing = false; }
    }

    private void SetSynchronizedSelection(TreeView tree, TreeViewNode? node)
    {
        // Realization can report selection after the immediate event guard has ended.
        synchronizedSelections[tree] = node;
        if (tree.SelectedNode != node) tree.SelectedNode = node;
    }

    private void Highlight(SnapshotTreeKind kind, IReadOnlyList<ulong> ids)
    {
        var nodes = kind == SnapshotTreeKind.Xaml ? xamlNodes : visualNodes;
        var previous = kind == SnapshotTreeKind.Xaml ? xamlMatches : visualMatches;
        foreach (var id in previous)
            if (nodes.TryGetValue(id, out var node)) ((SnapshotTreeLabel)node.Content).SetMatch(false);
        var matches = ids.ToHashSet();
        foreach (var id in matches)
            if (nodes.TryGetValue(id, out var node)) ((SnapshotTreeLabel)node.Content).SetMatch(true);
        if (kind == SnapshotTreeKind.Xaml) xamlMatches = matches; else visualMatches = matches;
    }

    private TreeViewNode Reveal(SnapshotTreeKind kind, ulong id)
    {
        var tree = kind == SnapshotTreeKind.Xaml ? XamlTree : CompositionTree;
        var data = kind == SnapshotTreeKind.Xaml ? Data!.Xaml : Data!.Composition;
        var nodes = kind == SnapshotTreeKind.Xaml ? xamlNodes : visualNodes;
        var roots = kind == SnapshotTreeKind.Xaml ? xamlRoots : visualRoots;
        var path = new Stack<SnapshotTreeNode>();
        var current = data.Nodes[id];
        path.Push(current);
        while (current.ParentId is { } parent) { current = data.Nodes[parent]; path.Push(current); }
        roots[current.RootId].IsExpanded = true;
        while (path.TryPop(out var next))
        {
            var uiNode = nodes[next.Id];
            if (path.Count > 0) { Populate(uiNode, kind); uiNode.IsExpanded = true; }
        }
        var result = nodes[id];
        SnapshotTreeExpansion.ScrollIntoView(tree, result, lifetime.Token);
        return result;
    }

    private void Matches_SelectionChanged(object sender, SelectionChangedEventArgs args)
    {
        if (synchronizing || Matches.SelectedItem is not SnapshotTreeNode model) return;
        try
        {
            synchronizing = true;
            SetSynchronizedSelection(matchTarget == SnapshotTreeKind.Xaml ? XamlTree : CompositionTree,
                Reveal(matchTarget, model.Id));
            UpdateDetails();
        }
        catch (Exception error) { ReportInteractionError(error); }
        finally { synchronizing = false; }
    }

    private void UpdateDetails()
    {
        XamlDetails.Text = (XamlTree.SelectedNode?.Content as SnapshotTreeLabel)?.Node?.Details ?? "";
        CompositionDetails.Text = (CompositionTree.SelectedNode?.Content as SnapshotTreeLabel)?.Node?.Details ?? "";
    }

    private void ReportInteractionError(Exception error)
    {
        Trace.TraceError(error.ToString());
        if (!closed) { LoadError.Message = error.Message; LoadError.IsOpen = true; }
    }
}
