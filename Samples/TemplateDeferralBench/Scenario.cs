using System;
using System.Collections.Generic;
using System.Linq;
using Microsoft.UI;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Media;

namespace TemplateDeferralBench
{
    // One benchmark scenario: N instances of a control whose template has deferred (x:Load="False") parts.
    public sealed class Scenario
    {
        // Unique name passed on the command line.
        public string Name { get; init; }

        // Builds the window content holding `count` instances of the control.
        public Func<int, FrameworkElement> Build { get; init; }

        // Template part names expected to be deferred. Realized instances of these names are counted
        // at startup and after the trigger.
        public string[] DeferredPartNames { get; init; } = Array.Empty<string>();

        // Changes state on all instances so that the deferred parts are needed (and realized). Optional.
        public Action<FrameworkElement> Trigger { get; init; }

        // Correctness checks after the trigger. Every bool value must be true in both variants.
        public Func<FrameworkElement, Dictionary<string, object>> Probe { get; init; }
    }

    public static class Bench
    {
        // Lays out `count` elements in rows of `perRow`.
        public static StackPanel Rows(int count, Func<int, FrameworkElement> create, int perRow = 10)
        {
            var root = new StackPanel { Background = new SolidColorBrush(Colors.White) };
            StackPanel row = null;
            for (int i = 0; i < count; i++)
            {
                if (i % perRow == 0)
                {
                    row = new StackPanel { Orientation = Orientation.Horizontal };
                    root.Children.Add(row);
                }
                row.Children.Add(create(i));
            }
            return root;
        }

        public static IEnumerable<DependencyObject> Descendants(DependencyObject start)
        {
            var pending = new Stack<DependencyObject>();
            pending.Push(start);
            while (pending.Count != 0)
            {
                var current = pending.Pop();
                yield return current;
                int children = VisualTreeHelper.GetChildrenCount(current);
                for (int i = children - 1; i >= 0; i--) pending.Push(VisualTreeHelper.GetChild(current, i));
            }
        }

        public static IEnumerable<T> All<T>(DependencyObject start) where T : DependencyObject =>
            Descendants(start).OfType<T>();

        public static FrameworkElement Named(DependencyObject start, string name) =>
            Descendants(start).OfType<FrameworkElement>().FirstOrDefault(f => f.Name == name);

        // True when the element exists, is Visible, and is visible all the way up to `stop` (exclusive).
        public static bool IsShown(FrameworkElement element, DependencyObject stop = null)
        {
            for (DependencyObject current = element; current != null && current != stop; current = VisualTreeHelper.GetParent(current))
            {
                if (current is UIElement u && (u.Visibility != Visibility.Visible || u.Opacity == 0)) return false;
            }
            return element != null;
        }
    }
}
