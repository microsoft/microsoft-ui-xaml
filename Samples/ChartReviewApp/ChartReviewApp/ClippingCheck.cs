// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

using System;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Media;
using Windows.Foundation;

namespace ChartsSample
{
    // Test hook (not for reviewers): --check-clipping=<path> opens every page and writes each single-line
    // text (button, combo box, check box, label) whose text needs more width than it was given, then exits.
    // Layout runs without a connected display, so this works where screenshots do not.
    public sealed partial class MainWindow
    {
        private async void CheckClippingIfRequested()
        {
            string path = Environment.GetCommandLineArgs()
                .FirstOrDefault(a => a.StartsWith("--check-clipping=", StringComparison.OrdinalIgnoreCase))?
                .Substring("--check-clipping=".Length);
            if (string.IsNullOrEmpty(path)) return;

            var report = new StringBuilder();
            report.AppendLine($"text scale {_textScale:0.##}, window {AppWindow.Size.Width}x{AppWindow.Size.Height}");
            int clipped = 0;
            foreach (var item in ScenarioNavigation.MenuItems.Concat(ScenarioNavigation.FooterMenuItems).OfType<NavigationViewItem>())
            {
                ScenarioNavigation.SelectedItem = item;
                await Task.Delay(1200);
                foreach (var text in Descendants(ScenarioScroll).OfType<TextBlock>())
                {
                    if (text.TextWrapping != TextWrapping.NoWrap || string.IsNullOrEmpty(text.Text) || !IsShown(text)) continue;
                    // Skip icon glyphs (private-use characters), such as the combo box chevron.
                    if (text.Text.Length == 1 && text.Text[0] >= '\uE000' && text.Text[0] <= '\uF8FF') continue;
                    // Measure the element itself: a detached TextBlock would not get the window's text scale.
                    double given = text.ActualWidth;
                    text.Measure(new Size(double.PositiveInfinity, double.PositiveInfinity));
                    double needed = text.DesiredSize.Width;
                    text.InvalidateMeasure();
                    // A button or combo box can clip its text even when the text keeps its full width, so also
                    // compare the text with the room inside its control (minus the combo box chevron).
                    double room = given;
                    var owner = OwnerControl(text);
                    if (owner != null)
                    {
                        double left = text.TransformToVisual(owner).TransformPoint(new Point(0, 0)).X;
                        double chevron = owner is ComboBox ? 30 : 0;
                        room = owner.ActualWidth - owner.Padding.Right - chevron - left;
                    }
                    // Placeholder text reports 2 to 3 px of rounding at every size, so only report real shortfalls.
                    if (needed > Math.Min(given, room) + 4.0)
                    {
                        clipped++;
                        report.AppendLine($"CLIPPED {item.Tag}: \"{text.Text}\" needs {needed:0} has {Math.Min(given, room):0}{(owner != null ? " in " + owner.GetType().Name : "")}");
                    }
                }
            }
            report.AppendLine($"clipped={clipped}");
            File.WriteAllText(path, report.ToString());
            Application.Current.Exit();
        }

        private static Control OwnerControl(DependencyObject element)
        {
            for (var e = VisualTreeHelper.GetParent(element); e != null; e = VisualTreeHelper.GetParent(e))
            {
                if (e is Button || e is ComboBox || e is CheckBox || e is Microsoft.UI.Xaml.Controls.Primitives.ToggleButton) return (Control)e;
                if (e is ScrollViewer) return null;
            }
            return null;
        }

        private static bool IsShown(FrameworkElement element)
        {
            for (DependencyObject e = element; e != null; e = VisualTreeHelper.GetParent(e))
            {
                if (e is UIElement u && u.Visibility != Visibility.Visible) return false;
            }
            return element.ActualWidth > 0;
        }

        private static System.Collections.Generic.IEnumerable<DependencyObject> Descendants(DependencyObject root)
        {
            int count = VisualTreeHelper.GetChildrenCount(root);
            for (int i = 0; i < count; i++)
            {
                var child = VisualTreeHelper.GetChild(root, i);
                yield return child;
                foreach (var d in Descendants(child)) yield return d;
            }
        }
    }
}
