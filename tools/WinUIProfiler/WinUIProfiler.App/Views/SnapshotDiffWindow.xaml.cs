using System.Diagnostics;
using System.Globalization;
using System.Text.Json;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Input;
using Windows.Graphics;
using WinUIProfiler.App.ViewModels;

namespace WinUIProfiler.App.Views;

public sealed class SnapshotDiffTreeItem(string label, SnapshotNodeDiff? difference = null) : SnapshotTreeLabel(label)
{
    public SnapshotNodeDiff? Difference { get; } = difference;
    public Visibility AddedVisibility => VisibilityFor(SnapshotChange.Added);
    public Visibility RemovedVisibility => VisibilityFor(SnapshotChange.Removed);
    public Visibility EditedVisibility => VisibilityFor(SnapshotChange.Properties | SnapshotChange.Moved | SnapshotChange.Reordered);
    private Visibility VisibilityFor(SnapshotChange change) => Difference is { } node && (node.Change & change) != 0
        ? Visibility.Visible : Visibility.Collapsed;
}

public sealed record SnapshotDiffMatchItem(string Label, SnapshotDiffMatch Match)
{
    public override string ToString() => Label;
}

public sealed class SnapshotDiffPropertyRow(string name, string before, string after)
{
    public string Name { get; } = name;
    public string Before { get; } = before;
    public string After { get; } = after;
    public override string ToString() => $"{Name}: {Before} -> {After}";
}

public sealed partial class SnapshotDiffWindow : Window
{
    private readonly CancellationTokenSource lifetime = new();
    private readonly Microsoft.Windows.ApplicationModel.Resources.ResourceLoader strings = new();
    private readonly SnapshotHistoryEntry before, after;
    private readonly Dictionary<SnapshotNodeDiff, TreeViewNode> nodes = [];
    private readonly Dictionary<SnapshotRootDiff, TreeViewNode> roots = [];
    private readonly Dictionary<TreeView, TreeViewNode?> synchronizedSelections = [];
    private HashSet<SnapshotNodeDiff> correlated = [], correlationContext = [];
    private SnapshotTreeKind matchTarget;
    private SnapshotDiffData? data;
    private bool closed, changingExpansion, synchronizing;

    public event EventHandler<SnapshotElement>? ElementRequested;

    public SnapshotDiffWindow(SnapshotHistoryEntry baseline, SnapshotHistoryEntry comparison)
    {
        before = baseline;
        after = comparison;
        Closed += (_, _) => { closed = true; lifetime.Cancel(); lifetime.Dispose(); };
        try
        {
            InitializeComponent();
            Title = Text("SnapshotDiffTitle");
            AppWindow.Resize(new SizeInt32(1520, 1000));
            BeforeIdentity.Text = Format("SnapshotDiffBeforeFormat", before.Process.ProcessId, before.CaptureId);
            AfterIdentity.Text = Format("SnapshotDiffAfterFormat", after.Process.ProcessId, after.CaptureId);
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
            ResourceSnapshot.Show(data.Before.Document.ResourceSnapshot, data.After.Document.ResourceSnapshot, true);
            ResourceSnapshot.Visibility = Visibility.Visible;
            NoDifferences.Visibility = data.Resources.HasChanges || data.Xaml.Nodes.Concat(data.Composition.Nodes)
                .Any(node => node.Change != SnapshotChange.None) ? Visibility.Collapsed : Visibility.Visible;
            RebuildTrees();
            Viewer.Visibility = Visibility.Visible;
        }
        catch (OperationCanceledException) when (closed) { }
        catch (Exception error) { ReportError(error); }
        finally { if (!closed) Loading.IsActive = false; }
    }

    private string Summary(SnapshotTreeDiff tree) => Format("SnapshotDiffSummaryFormat",
        tree.Count(SnapshotChange.Added), tree.Count(SnapshotChange.Removed), tree.Count(SnapshotChange.Properties),
        tree.Count(SnapshotChange.Moved), tree.Count(SnapshotChange.Reordered));

    private static string Counts(SnapshotTreeDiff tree) => JsonSerializer.Serialize(new
    {
        added = tree.Count(SnapshotChange.Added),
        removed = tree.Count(SnapshotChange.Removed),
        properties = tree.Count(SnapshotChange.Properties),
        moved = tree.Count(SnapshotChange.Moved),
        reordered = tree.Count(SnapshotChange.Reordered)
    });

    private string ChangeLabel(SnapshotNodeDiff node)
    {
        if (node.Change == SnapshotChange.None)
            return Text(node.ContainsChanges ? "SnapshotDiffSubtree" : "SnapshotDiffUnchanged");
        return string.Join(", ", Enum.GetValues<SnapshotChange>().Where(change =>
            change != SnapshotChange.None && node.Change.HasFlag(change)).Select(change => Text("SnapshotDiff" + change)));
    }

    private bool Include(SnapshotNodeDiff node) =>
        ChangesOnly.IsChecked != true || node.ContainsChanges || correlationContext.Contains(node);

    private void RebuildTrees()
    {
        nodes.Clear();
        roots.Clear();
        synchronizedSelections.Clear();
        foreach (var (tree, diff) in new[] { (XamlTree, data!.Xaml), (CompositionTree, data.Composition) })
        {
            tree.RootNodes.Clear();
            foreach (var root in diff.Roots.Where(root => root.Children.Any(Include)))
            {
                var label = Format("SnapshotDiffRootFormat", root.Root.XamlRootAddress, root.Root.ThreadId);
                var group = new TreeViewNode { Content = new SnapshotDiffTreeItem(label) };
                foreach (var node in root.Children.Where(Include)) group.Children.Add(CreateNode(node));
                tree.RootNodes.Add(group);
                roots.Add(root, group);
                group.IsExpanded = true;
            }
        }
        ClearDetails();
    }

    private void RefreshCorrelationContext()
    {
        foreach (var (tree, diff) in new[] { (XamlTree, data!.Xaml), (CompositionTree, data.Composition) })
        {
            foreach (var root in diff.Roots.Where(root => !root.Children.Any(Include)))
            {
                if (!roots.Remove(root, out var group)) continue;
                ForgetNodes(group);
                tree.RootNodes.Remove(group);
            }
            var pending = new Stack<(IList<TreeViewNode> Children, IReadOnlyList<SnapshotNodeDiff> Models)>();
            var index = 0;
            foreach (var root in diff.Roots.Where(root => root.Children.Any(Include)))
            {
                if (!roots.TryGetValue(root, out var group))
                {
                    group = new TreeViewNode
                    {
                        Content = new SnapshotDiffTreeItem(Format("SnapshotDiffRootFormat", root.Root.XamlRootAddress, root.Root.ThreadId)),
                        IsExpanded = true
                    };
                    roots.Add(root, group);
                    tree.RootNodes.Insert(index, group);
                }
                pending.Push((group.Children, root.Children));
                index++;
            }
            while (pending.TryPop(out var entry))
            {
                for (var i = entry.Children.Count - 1; i >= 0; i--)
                {
                    var child = entry.Children[i];
                    if (Include(((SnapshotDiffTreeItem)child.Content).Difference!)) continue;
                    ForgetNodes(child);
                    entry.Children.RemoveAt(i);
                }
                index = 0;
                foreach (var model in entry.Models.Where(Include))
                {
                    TreeViewNode child;
                    if (index < entry.Children.Count &&
                        ((SnapshotDiffTreeItem)entry.Children[index].Content).Difference == model)
                        child = entry.Children[index];
                    else
                    {
                        child = CreateNode(model);
                        entry.Children.Insert(index, child);
                    }
                    if (child.HasUnrealizedChildren) child.HasUnrealizedChildren = model.Children.Any(Include);
                    else pending.Push((child.Children, model.Children));
                    index++;
                }
            }
        }
    }

    private void ForgetNodes(TreeViewNode node)
    {
        var pending = new Stack<TreeViewNode>();
        pending.Push(node);
        while (pending.TryPop(out var current))
        {
            if (current.Content is SnapshotDiffTreeItem { Difference: { } model }) nodes.Remove(model);
            foreach (var child in current.Children) pending.Push(child);
        }
    }

    private TreeViewNode CreateNode(SnapshotNodeDiff model)
    {
        var label = new SnapshotDiffTreeItem($"[{ChangeLabel(model)}] {(model.After ?? model.Before!).Label}", model);
        label.SetMatch(correlated.Contains(model));
        var node = new TreeViewNode { Content = label, HasUnrealizedChildren = model.Children.Any(Include) };
        nodes.Add(model, node);
        return node;
    }

    private void Populate(TreeViewNode node)
    {
        if (!node.HasUnrealizedChildren || node.Content is not SnapshotDiffTreeItem { Difference: { } model }) return;
        foreach (var child in model.Children.Where(Include)) node.Children.Add(CreateNode(child));
        node.HasUnrealizedChildren = false;
    }

    private void Tree_Expanding(TreeView sender, TreeViewExpandingEventArgs args) => Populate(args.Node);

    // Double-clicking a XAML node asks the main window to show its element's history in the Elements tab: as it was in
    // the later snapshot, or in the earlier one for a removed node.
    private void XamlTree_DoubleTapped(object sender, DoubleTappedRoutedEventArgs args)
    {
        if (data is null || SnapshotTreeExpansion.NodeAt(XamlTree, args.OriginalSource)?.Content is not SnapshotDiffTreeItem { Difference: { } node }) return;
        args.Handled = true;
        ElementRequested?.Invoke(this, node.After is { } later
            ? SnapshotElement.From(data.After.Document, later) : SnapshotElement.From(data.Before.Document, node.Before!));
    }

    private void ChangesOnly_Click(object sender, RoutedEventArgs args)
    {
        if (data is null || changingExpansion || closed) return;
        try
        {
            synchronizing = true;
            ClearCorrelation();
            RebuildTrees();
        }
        catch (Exception error) { ReportError(error); }
        finally { synchronizing = false; }
    }

    private async void ExpandAll_Click(object sender, RoutedEventArgs args) =>
        await SetExpandedAsync(ReferenceEquals(sender, XamlExpandAll) ? XamlTree : CompositionTree, true);

    private async void CollapseAll_Click(object sender, RoutedEventArgs args) =>
        await SetExpandedAsync(ReferenceEquals(sender, XamlCollapseAll) ? XamlTree : CompositionTree, false);

    private async Task SetExpandedAsync(TreeView tree, bool expanded)
    {
        if (closed || changingExpansion) return;
        changingExpansion = true;
        Control[] controls = [XamlTree, CompositionTree, ChangesOnly, Matches, XamlExpandAll, XamlCollapseAll,
            CompositionExpandAll, CompositionCollapseAll];
        try
        {
            foreach (var control in controls) control.IsEnabled = false;
            await SnapshotTreeExpansion.SetExpandedAsync(tree, expanded, Populate, lifetime.Token);
        }
        catch (Exception error) { ReportError(error); }
        finally
        {
            changingExpansion = false;
            if (!closed)
                foreach (var control in controls) control.IsEnabled = true;
        }
    }

    private void Tree_SelectionChanged(TreeView sender, TreeViewSelectionChangedEventArgs args)
    {
        if (data is null || closed || synchronizing || changingExpansion) return;
        if (synchronizedSelections.TryGetValue(sender, out var expected))
        {
            if (sender.SelectedNode == expected) return;
            synchronizedSelections.Remove(sender);
        }
        SelectNode(sender);
    }

    private void Tree_ItemInvoked(TreeView sender, TreeViewItemInvokedEventArgs args)
    {
        if (data is null || closed || synchronizing || changingExpansion) return;
        if (sender == Tree(matchTarget))
        {
            if (synchronizedSelections.Remove(sender)) SelectNode(sender);
        }
        else if (sender.SelectedNode?.Content is SnapshotDiffTreeItem { Difference: { } node })
        {
            try
            {
                UpdateDetails(sender == XamlTree ? SnapshotTreeKind.Xaml : SnapshotTreeKind.Composition, node);
            }
            catch (Exception error) { ReportError(error); }
        }
    }

    private TreeView Tree(SnapshotTreeKind kind) => kind == SnapshotTreeKind.Xaml ? XamlTree : CompositionTree;

    private void SelectNode(TreeView sender)
    {
        try
        {
            synchronizing = true;
            if (sender.SelectedNode?.Content is not SnapshotDiffTreeItem { Difference: { } node })
            {
                var rebuild = ChangesOnly.IsChecked == true && correlationContext.Count > 0;
                ClearCorrelation();
                if (rebuild) RefreshCorrelationContext();
                SetSynchronizedSelection(sender == XamlTree ? CompositionTree : XamlTree, null);
                ClearDetails();
                return;
            }
            var kind = sender == XamlTree ? SnapshotTreeKind.Xaml : SnapshotTreeKind.Composition;
            var matches = data!.SelectMatches(kind, node);
            var context = new HashSet<SnapshotNodeDiff>();
            foreach (var match in matches.Select(match => match.Node).Prepend(node))
            {
                for (SnapshotNodeDiff? current = match; current is not null && context.Add(current); current = current.Parent) { }
            }
            context.RemoveWhere(model => model.ContainsChanges);
            var refreshContext = ChangesOnly.IsChecked == true && !correlationContext.SetEquals(context);
            SetSynchronizedSelection(sender == XamlTree ? CompositionTree : XamlTree, null);
            ClearCorrelation();
            correlationContext = context;
            correlated = matches.Select(match => match.Node).Append(node).ToHashSet();
            if (refreshContext) RefreshCorrelationContext();
            foreach (var match in correlated)
                if (nodes.TryGetValue(match, out var uiNode)) ((SnapshotDiffTreeItem)uiNode.Content).SetMatch(true);

            SetSynchronizedSelection(sender, Reveal(kind, node));
            matchTarget = kind == SnapshotTreeKind.Xaml ? SnapshotTreeKind.Composition : SnapshotTreeKind.Xaml;
            Matches.ItemsSource = matches.Select(match => new SnapshotDiffMatchItem(
                Format("SnapshotDiffMatchFormat", Text(match.Before && match.After ? "SnapshotDiffMatchBoth" :
                    match.After ? "SnapshotDiffMatchAfter" : "SnapshotDiffMatchBefore"),
                    $"[{ChangeLabel(match.Node)}] {(match.Node.After ?? match.Node.Before!).Label}"), match)).ToArray();
            Matches.SelectedIndex = matches.Count > 0 ? 0 : -1;
            SetSynchronizedSelection(Tree(matchTarget), matches.Count > 0 ? Reveal(matchTarget, matches[0].Node) : null);
            MappingStatus.Text = matches.Count > 0 ? Format("SnapshotDiffMappingsFormat", matches.Count) : Text("SnapshotNoMapping");
            AutomationProperties.SetItemStatus(MappingStatus, JsonSerializer.Serialize(new
            {
                tree = kind.ToString(),
                beforeId = node.Before?.Id,
                afterId = node.After?.Id,
                matches = matches.Select(match => new
                {
                    beforeId = match.Node.Before?.Id,
                    afterId = match.Node.After?.Id,
                    before = match.Before,
                    after = match.After
                })
            }));
            UpdateDetails(kind, node);
        }
        catch (Exception error) { ReportError(error); }
        finally { synchronizing = false; }
    }

    private void SetSynchronizedSelection(TreeView tree, TreeViewNode? node)
    {
        // Lazy realization can deliver selection events after the immediate guard ends.
        synchronizedSelections[tree] = node;
        if (tree.SelectedNode != node) tree.SelectedNode = node;
    }

    private TreeViewNode Reveal(SnapshotTreeKind kind, SnapshotNodeDiff model)
    {
        var path = new Stack<SnapshotNodeDiff>();
        for (SnapshotNodeDiff? current = model; current is not null; current = current.Parent) path.Push(current);
        nodes[path.Peek()].Parent.IsExpanded = true;
        while (path.TryPop(out var next))
        {
            var node = nodes[next];
            if (path.Count > 0) { Populate(node); node.IsExpanded = true; }
        }
        var result = nodes[model];
        SnapshotTreeExpansion.ScrollIntoView(Tree(kind), result, lifetime.Token);
        return result;
    }

    private void ClearCorrelation()
    {
        foreach (var match in correlated)
            if (nodes.TryGetValue(match, out var node)) ((SnapshotDiffTreeItem)node.Content).SetMatch(false);
        correlated.Clear();
        correlationContext.Clear();
    }

    private void Matches_SelectionChanged(object sender, SelectionChangedEventArgs args)
    {
        if (closed || synchronizing || changingExpansion || Matches.SelectedItem is not SnapshotDiffMatchItem item) return;
        try
        {
            synchronizing = true;
            SetSynchronizedSelection(Tree(matchTarget), Reveal(matchTarget, item.Match.Node));
            UpdateDetails(matchTarget, item.Match.Node);
        }
        catch (Exception error) { ReportError(error); }
        finally { synchronizing = false; }
    }

    private void UpdateDetails(SnapshotTreeKind kind, SnapshotNodeDiff node)
    {
        SelectionSummary.Text = Format("SnapshotDiffSelectedFormat", Text("SnapshotDiffKind" + kind),
            ChangeLabel(node), (node.After ?? node.Before!).Label) +
            (node.Properties.Count == 0 ? " " + Text("SnapshotDiffNoPropertyChanges") : "");
        BeforeLocation.Text = Format("SnapshotDiffBeforeLocationFormat",
            Location(node.Before, kind == SnapshotTreeKind.Xaml ? data!.Before.Xaml : data!.Before.Composition));
        AfterLocation.Text = Format("SnapshotDiffAfterLocationFormat",
            Location(node.After, kind == SnapshotTreeKind.Xaml ? data!.After.Xaml : data!.After.Composition));
        PropertyChanges.ItemsSource = node.Properties.Select(property => new SnapshotDiffPropertyRow(
            property.Name, Value(property.Before), Value(property.After))).ToArray();
        AutomationProperties.SetItemStatus(SelectionSummary, JsonSerializer.Serialize(new
        {
            tree = kind.ToString(),
            beforeId = node.Before?.Id,
            afterId = node.After?.Id,
            change = node.Change.ToString(),
            properties = node.Properties.Select(property => property.Name)
        }));
    }

    private void ClearDetails()
    {
        Matches.ItemsSource = null;
        MappingStatus.Text = Text("SnapshotSelectNode");
        AutomationProperties.SetItemStatus(MappingStatus, "");
        SelectionSummary.Text = Text("SnapshotDiffSelectNode");
        AutomationProperties.SetItemStatus(SelectionSummary, "");
        BeforeLocation.Text = AfterLocation.Text = "";
        PropertyChanges.ItemsSource = null;
    }

    private string Value(JsonElement? value) => value.HasValue ? JsonSerializer.Serialize(value.Value) : Text("SnapshotDiffAbsent");

    private string Location(SnapshotTreeNode? node, SnapshotTreeData tree) => node is null ? Text("SnapshotDiffAbsent") :
        Format("SnapshotDiffLocationFormat", node.ParentId is { } id ? tree.Nodes[id].Label : Text("SnapshotDiffTreeRoot"), node.ChildIndex);

    private void ReportError(Exception error)
    {
        Trace.TraceError(error.ToString());
        if (!closed) { LoadError.Message = error.Message; LoadError.IsOpen = true; }
    }
}
