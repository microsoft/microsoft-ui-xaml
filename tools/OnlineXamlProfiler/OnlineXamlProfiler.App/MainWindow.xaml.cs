// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Dispatching;
using System.ComponentModel;
using Microsoft.Windows.ApplicationModel.Resources;
using OnlineXamlProfiler.Core;
using Windows.Graphics;

namespace OnlineXamlProfiler.App;

public sealed partial class MainWindow : Window
{
    private CancellationTokenSource? _treeOperation;
    private Dictionary<SnapshotTreeNode, TreeViewNode> _viewNodes = [];
    private bool _closed;
    public MainViewModel ViewModel { get; } = new();

    public MainWindow()
    {
        InitializeComponent();
        Title = new ResourceLoader().GetString("WindowTitle");
        AppWindow.Resize(new SizeInt32(1500, 1000));
        ViewModel.PropertyChanged += ViewModel_PropertyChanged;
        Closed += (_, _) =>
        {
            _closed = true;
            _treeOperation?.Cancel();
            ViewModel.Dispose();
        };
    }

    public static string NodeLabel(object content) => ((SnapshotTreeNode)content).Label;

    private void Tree_SelectionChanged(TreeView sender, TreeViewSelectionChangedEventArgs args)
    {
        if (args.AddedItems.FirstOrDefault() is TreeViewNode { Content: SnapshotTreeNode node })
        {
            ViewModel.SelectNode(node);
        }
    }

    private void Link_Click(object sender, RoutedEventArgs args)
    {
        var kind = (TreeKind)int.Parse((string)((Button)sender).Tag);
        if (ViewModel.FindLinkedNode(kind) is not { } linked)
        {
            return;
        }
        var tree = kind switch { TreeKind.Xaml => XamlTree, TreeKind.Composition => CompTree, _ => VisualTree };
        if (!_viewNodes.TryGetValue(linked, out var viewNode))
        {
            return;
        }
        for (var parent = viewNode.Parent; parent is not null; parent = parent.Parent)
        {
            parent.IsExpanded = true;
        }
        tree.SelectedNode = viewNode;
        ViewModel.SelectNode(linked);
        tree.UpdateLayout();
        if (tree.ContainerFromNode(viewNode) is FrameworkElement container)
        {
            container.StartBringIntoView();
        }
        tree.Focus(FocusState.Programmatic);
    }

    private async void ExpandAll_Click(object sender, RoutedEventArgs args)
    {
        if (_treeOperation is not null || _viewNodes.Count == 0)
        {
            return;
        }
        using var expansion = new CancellationTokenSource();
        _treeOperation = expansion;
        ExpandAllButton.IsEnabled = false;
        try
        {
            var nodes = _viewNodes.Values.ToArray();
            for (int index = 0; index < nodes.Length; ++index)
            {
                expansion.Token.ThrowIfCancellationRequested();
                if (nodes[index].HasChildren) { nodes[index].IsExpanded = true; }
                if ((index + 1) % 256 == 0)
                {
                    await YieldToDispatcherAsync(expansion.Token);
                }
            }
        }
        catch (OperationCanceledException) when (expansion.IsCancellationRequested)
        {
            // A new snapshot selection or window close superseded this expansion.
        }
        catch (Exception exception)
        {
            if (!_closed) { ViewModel.ReportError(exception); }
        }
        finally
        {
            if (ReferenceEquals(_treeOperation, expansion))
            {
                _treeOperation = null;
                if (!_closed) { ExpandAllButton.IsEnabled = _viewNodes.Count != 0; }
            }
        }
    }

    private async void ViewModel_PropertyChanged(object? sender, PropertyChangedEventArgs args)
    {
        if (_closed) { return; }
        if (args.PropertyName is nameof(MainViewModel.StatisticsHistory) or nameof(MainViewModel.SelectedSnapshot))
        {
            ProcessStatistics.Update(ViewModel.StatisticsHistory, ViewModel.Snapshots,
                ViewModel.SelectedSnapshot, ViewModel.StatisticsStatus);
        }
        if (args.PropertyName != nameof(MainViewModel.SelectedSnapshot))
        {
            return;
        }
        _treeOperation?.Cancel();
        using var build = new CancellationTokenSource();
        _treeOperation = build;
        ExpandAllButton.IsEnabled = false;
        _viewNodes.Clear();
        foreach (var tree in new[] { XamlTree, CompTree, VisualTree })
        {
            tree.IsEnabled = false;
            tree.RootNodes.Clear();
        }
        try
        {
            var nodes = new Dictionary<SnapshotTreeNode, TreeViewNode>();
            var xaml = await BuildViewAsync(ViewModel.XamlRoots, nodes, build.Token);
            var comp = await BuildViewAsync(ViewModel.CompRoots, nodes, build.Token);
            var visuals = await BuildViewAsync(ViewModel.VisualRoots, nodes, build.Token);
            build.Token.ThrowIfCancellationRequested();
            _viewNodes = nodes;
            foreach (var node in xaml) { XamlTree.RootNodes.Add(node); }
            foreach (var node in comp) { CompTree.RootNodes.Add(node); }
            foreach (var node in visuals) { VisualTree.RootNodes.Add(node); }
        }
        catch (OperationCanceledException) when (build.IsCancellationRequested)
        {
            // A new selection or window close superseded this view publication.
        }
        catch (Exception exception)
        {
            if (!_closed) { ViewModel.ReportError(exception); }
        }
        finally
        {
            if (ReferenceEquals(_treeOperation, build))
            {
                _treeOperation = null;
                if (!_closed)
                {
                    XamlTree.IsEnabled = CompTree.IsEnabled = VisualTree.IsEnabled = true;
                    ExpandAllButton.IsEnabled = _viewNodes.Count != 0;
                }
            }
        }
    }

    private async Task<List<TreeViewNode>> BuildViewAsync(
        IReadOnlyList<SnapshotTreeNode> models,
        Dictionary<SnapshotTreeNode, TreeViewNode> nodes,
        CancellationToken token)
    {
        var roots = new List<TreeViewNode>();
        var pending = new Stack<(SnapshotTreeNode Model, TreeViewNode? Parent)>();
        for (int index = models.Count - 1; index >= 0; --index)
        {
            pending.Push((models[index], null));
        }
        while (pending.TryPop(out var entry))
        {
            token.ThrowIfCancellationRequested();
            var node = new TreeViewNode { Content = entry.Model };
            nodes.Add(entry.Model, node);
            if (entry.Parent is null) { roots.Add(node); }
            else { entry.Parent.Children.Add(node); }
            for (int index = entry.Model.Children.Count - 1; index >= 0; --index)
            {
                pending.Push((entry.Model.Children[index], node));
            }
            // Native view objects need the UI thread; yield between batches instead of blocking it.
            if (nodes.Count % 256 == 0)
            {
                await YieldToDispatcherAsync(token);
            }
        }
        return roots;
    }

    private async Task YieldToDispatcherAsync(CancellationToken token)
    {
        var resumed = new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);
        using var cancelled = token.Register(() => resumed.TrySetCanceled(token));
        if (!DispatcherQueue.TryEnqueue(DispatcherQueuePriority.Low, () => resumed.TrySetResult()))
        {
            throw new InvalidOperationException("The profiler UI dispatcher is shutting down.");
        }
        await resumed.Task;
    }
}
