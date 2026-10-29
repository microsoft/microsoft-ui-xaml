using System.Diagnostics;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Media;

namespace WinUIProfiler.App.Views;

internal static class SnapshotTreeExpansion
{
    // The tree node whose item contains an element, such as the source of a double-tap; null outside the items.
    public static TreeViewNode? NodeAt(TreeView tree, object? source)
    {
        for (var element = source as DependencyObject; element is not null && element != tree; element = VisualTreeHelper.GetParent(element))
        {
            if (element is TreeViewItem item)
            {
                return tree.NodeFromContainer(item);
            }
        }

        return null;
    }

    public static void ScrollIntoView(TreeView tree, TreeViewNode node, CancellationToken token)
    {
        if (!tree.DispatcherQueue.TryEnqueue(() =>
        {
            if (token.IsCancellationRequested || tree.SelectedNode != node) return;
            var pending = new Stack<DependencyObject>();
            pending.Push(tree);
            while (pending.TryPop(out var element))
            {
                if (element is ListView list && list.Name == "ListControl")
                {
                    list.ScrollIntoView(node);
                    return;
                }
                for (var i = 0; i < VisualTreeHelper.GetChildrenCount(element); ++i)
                    pending.Push(VisualTreeHelper.GetChild(element, i));
            }
            if (tree.ContainerFromNode(node) is FrameworkElement container) container.StartBringIntoView();
            else Trace.TraceWarning("Snapshot mapping selected, but its TreeView scroll container was unavailable.");
        }) && !token.IsCancellationRequested)
            Trace.TraceWarning("The dispatcher rejected scrolling to the selected snapshot node.");
    }

    public static async Task SetExpandedAsync(TreeView tree, bool expanded, Action<TreeViewNode> populate,
        CancellationToken token)
    {
        var pending = new Stack<TreeViewNode>(tree.RootNodes);
        var processed = 0;
        while (pending.TryPop(out var node))
        {
            if (token.IsCancellationRequested) return;
            if (expanded) populate(node);
            node.IsExpanded = expanded;
            foreach (var child in node.Children) pending.Push(child);
            if (++processed % 128 == 0) await Task.Yield();
        }
    }
}
