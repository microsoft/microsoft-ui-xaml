using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Media;

namespace XamlProfiler.Helpers;

/// <summary>
/// Generic visual-tree traversal helpers. These are UI helpers (they depend on
/// <see cref="VisualTreeHelper"/>), but they are control-agnostic and reused across the
/// window, so they live outside the MainWindow code-behind.
/// </summary>
internal static class VisualTreeUtils
{
    /// <summary>
    /// Depth-first search of the visual tree for the first descendant of type
    /// <typeparamref name="T"/>. Used to reach a control's internal ScrollViewer.
    /// </summary>
    public static T? FindDescendant<T>(DependencyObject root) where T : DependencyObject
    {
        int count = VisualTreeHelper.GetChildrenCount(root);
        for (int i = 0; i < count; i++)
        {
            var child = VisualTreeHelper.GetChild(root, i);
            if (child is T match) return match;
            var result = FindDescendant<T>(child);
            if (result != null) return result;
        }
        return null;
    }

    /// <summary>
    /// Searches the realized visual tree for the <see cref="TreeViewItem"/> whose DataContext
    /// is the given object. Only realized (on-screen / expanded-and-measured) containers are
    /// present, so this is bounded by what's currently materialized — virtualized rows simply
    /// aren't here yet.
    /// </summary>
    public static TreeViewItem? FindContainerByDataContext(DependencyObject root, object dataContext)
    {
        int count = VisualTreeHelper.GetChildrenCount(root);
        for (int i = 0; i < count; i++)
        {
            var child = VisualTreeHelper.GetChild(root, i);
            if (child is TreeViewItem tvi && ReferenceEquals(tvi.DataContext, dataContext))
                return tvi;
            var nested = FindContainerByDataContext(child, dataContext);
            if (nested != null) return nested;
        }
        return null;
    }
}
