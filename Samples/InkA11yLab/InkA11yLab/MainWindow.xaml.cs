// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

using System;
using System.Collections.Generic;
using System.Text;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Automation.Peers;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Shapes;
using Windows.UI.Core;
using Windows.UI.Input.Inking;

using InkPresenter = Microsoft.UI.Xaml.Controls.InkPresenter;
using InkHighContrastAdjustment = Microsoft.UI.Xaml.Controls.InkHighContrastAdjustment;
using InkStrokesCollectedEventArgs = Microsoft.UI.Xaml.Controls.InkStrokesCollectedEventArgs;
using InkSynchronizer = Microsoft.UI.Xaml.Controls.InkSynchronizer;

namespace InkA11yLab
{
    public sealed partial class MainWindow : Window
    {
        private InkSynchronizer _synchronizerA;

        public MainWindow()
        {
            this.InitializeComponent();
            Title = "Ink accessibility lab";
        }

        private void OnRootLoaded(object sender, RoutedEventArgs e)
        {
            // Sized here rather than in the constructor: the options row is wide, and a narrow window
            // pushes controls off screen, which the accessibility report then reports as real failures.
            AppWindow.Resize(new Windows.Graphics.SizeInt32(1900, 1150));

            // The Checked/SelectionChanged handlers no-op during parse, so apply the initial ink
            // configuration here. Without it the canvas keeps its pen-only default and a mouse draws nothing.
            OnInputDevicesChanged(sender, e);
            OnHighContrastModeChanged(sender, null);
            Announce($"Ready. Compositor: {App.CompositorStatus}. Draw with pen, mouse or touch.");
        }

        // ---- ink configuration -------------------------------------------------

        // Checked fires while InitializeComponent is still parsing, so the checkboxes declared after
        // PenInput are not assigned yet; OnRootLoaded applies the real value once the tree is up.
        private void OnInputDevicesChanged(object sender, RoutedEventArgs e)
        {
            if (PenInput is null || MouseInput is null || TouchInput is null)
            {
                return;
            }

            var types = CoreInputDeviceTypes.None;
            if (PenInput.IsChecked == true) { types |= CoreInputDeviceTypes.Pen; }
            if (MouseInput.IsChecked == true) { types |= CoreInputDeviceTypes.Mouse; }
            if (TouchInput.IsChecked == true) { types |= CoreInputDeviceTypes.Touch; }

            ForEachPresenter(p => p.InputDeviceTypes = types);
            Announce($"Input devices: {types}.");
        }

        private void OnHighContrastModeChanged(object sender, SelectionChangedEventArgs e)
        {
            var mode = HighContrastMode.SelectedIndex switch
            {
                1 => InkHighContrastAdjustment.UseSystemColors,
                2 => InkHighContrastAdjustment.UseOriginalColors,
                _ => InkHighContrastAdjustment.UseSystemColorsWhenNecessary,
            };

            ForEachPresenter(p => p.HighContrastAdjustment = mode);
            Announce($"High contrast adjustment: {mode}.");
        }

        private void OnOrientationToggled(object sender, RoutedEventArgs e)
        {
            var orientation = VerticalToolbars.IsOn ? Orientation.Vertical : Orientation.Horizontal;
            if (ToolbarA is null || ToolbarB is null)
            {
                return;
            }

            ToolbarA.Orientation = orientation;
            ToolbarB.Orientation = orientation;
            Announce($"Toolbars are now {orientation}.");
        }

        private void OnOverlayReproChanged(object sender, RoutedEventArgs e)
        {
            if (ToolbarB is null)
            {
                return;
            }

            bool on = OverlayRepro.IsChecked == true;
            ToolbarB.Visibility = on ? Visibility.Visible : Visibility.Collapsed;
            Announce(on
                ? "Overlaid toolbar shown. Known airspace bug: the ink surface composes above XAML, so these buttons are not clickable and drawing over them collects strokes instead."
                : "Overlaid toolbar hidden.");
        }

        private void OnClear(object sender, RoutedEventArgs e)
        {
            CanvasA?.InkPresenter?.StrokeContainer?.Clear();
            DryCanvasA?.Children.Clear();
            Announce("Surface cleared.");
        }

        // ---- custom drying -----------------------------------------------------

        private void OnCustomDryingChanged(object sender, RoutedEventArgs e)
        {
            var presenter = CanvasA.InkPresenter;
            if (presenter is null)
            {
                Announce("Custom drying: the ink presenter is not ready yet, try again.");
                CustomDrying.IsChecked = false;
                return;
            }

            if (CustomDrying.IsChecked == true)
            {
                try
                {
                    // Must be called before the first stroke; it cannot be undone for this presenter.
                    _synchronizerA = presenter.ActivateCustomDrying();
                    presenter.StrokesCollected += OnStrokesCollectedCustomDry;
                    Announce("Custom drying active on surface A. Dry strokes are rendered by the app.");
                }
                catch (Exception ex)
                {
                    CustomDrying.IsChecked = false;
                    Announce($"ActivateCustomDrying failed: 0x{ex.HResult:X8}. It must be called before the first stroke.");
                }
            }
            else
            {
                Announce("Custom drying stays active for the life of the presenter; restart to go back to system drying.");
                CustomDrying.IsChecked = true;
            }
        }

        private void OnStrokesCollectedCustomDry(InkPresenter sender, InkStrokesCollectedEventArgs args)
        {
            if (_synchronizerA is null)
            {
                return;
            }

            var strokes = _synchronizerA.BeginDry();
            foreach (var stroke in strokes)
            {
                DryCanvasA.Children.Add(BuildStrokeShape(stroke));
            }

            _synchronizerA.EndDry();
        }

        private static Polyline BuildStrokeShape(InkStroke stroke)
        {
            var attributes = stroke.DrawingAttributes;
            var line = new Polyline
            {
                Stroke = new SolidColorBrush(attributes.Color),
                StrokeThickness = attributes.Size.Width,
                StrokeLineJoin = PenLineJoin.Round,
                StrokeStartLineCap = PenLineCap.Round,
                StrokeEndLineCap = PenLineCap.Round,
            };

            foreach (var point in stroke.GetInkPoints())
            {
                line.Points.Add(point.Position);
            }

            return line;
        }

        // ---- accessibility report ---------------------------------------------

        private void OnRunReport(object sender, RoutedEventArgs e)
        {
            var report = new StringBuilder();

            // Ink state first: if InputDeviceTypes excludes the device you are using, nothing draws.
            if (CanvasA?.InkPresenter is { } inkPresenter)
            {
                int strokes = inkPresenter.StrokeContainer?.GetStrokes()?.Count ?? -1;
                report.AppendLine($"InkPresenter: InputDeviceTypes={inkPresenter.InputDeviceTypes}, " +
                                  $"HighContrastAdjustment={inkPresenter.HighContrastAdjustment}, strokes={strokes}");
            }
            else
            {
                report.AppendLine("InkPresenter: not available yet");
            }

            report.AppendLine();
            report.AppendLine("element                    type        name                                 rect                      offscreen  focusable  patterns");
            report.AppendLine(new string('-', 150));

            int problems = 0;
            foreach (var element in NamedElements())
            {
                var peer = FrameworkElementAutomationPeer.FromElement(element)
                           ?? FrameworkElementAutomationPeer.CreatePeerForElement(element);

                var id = AutomationProperties.GetAutomationId(element);
                if (string.IsNullOrEmpty(id)) { id = element.Name; }

                if (peer is null)
                {
                    report.AppendLine($"{id,-26} {"(no peer)",-11}");
                    problems++;
                    continue;
                }

                var rect = peer.GetBoundingRectangle();
                bool emptyRect = rect.Width <= 0 || rect.Height <= 0
                                 || double.IsInfinity(rect.Width) || double.IsInfinity(rect.Height);
                bool offscreen = peer.IsOffscreen();
                bool visible = element.Visibility == Visibility.Visible;

                var rectText = emptyRect
                    ? "EMPTY"
                    : $"{(int)rect.X},{(int)rect.Y} {(int)rect.Width}x{(int)rect.Height}";

                report.AppendLine(
                    $"{id,-26} {peer.GetAutomationControlType(),-11} {Trim(peer.GetName(), 36),-36} " +
                    $"{rectText,-25} {offscreen,-10} {peer.IsKeyboardFocusable(),-10} {Patterns(peer)}");

                if (visible && (emptyRect || offscreen)) { problems++; }
            }

            report.AppendLine();
            report.AppendLine(problems == 0
                ? "No visible element reported an empty rectangle or offscreen state."
                : $"{problems} visible element(s) reported an empty rectangle or offscreen state.");

            ReportText.Text = report.ToString();

            // Also written to disk so the report can be attached to a bug without retyping it.
            var reportPath = System.IO.Path.Combine(AppContext.BaseDirectory, "a11y-report.txt");
            try
            {
                System.IO.File.WriteAllText(reportPath, report.ToString());
            }
            catch (System.IO.IOException)
            {
                reportPath = "(could not write file)";
            }

            Announce($"Accessibility report complete. {problems} problem(s) found. Saved to {reportPath}");
        }

        private IEnumerable<FrameworkElement> NamedElements()
        {
            yield return ToolbarA;
            yield return CanvasA;
            yield return ScrollA;
            yield return ToolbarB;
            yield return PenInput;
            yield return MouseInput;
            yield return TouchInput;
            yield return VerticalToolbars;
            yield return HighContrastMode;
            yield return CustomDrying;
            yield return OverlayRepro;
            yield return RunReport;
            yield return StatusText;
        }

        private static string Patterns(AutomationPeer peer)
        {
            var found = new List<string>();
            foreach (PatternInterface pattern in Enum.GetValues(typeof(PatternInterface)))
            {
                try
                {
                    if (peer.GetPattern(pattern) is not null) { found.Add(pattern.ToString()); }
                }
                catch
                {
                    // A peer may refuse a pattern it does not implement; that is not a failure here.
                }
            }

            return found.Count == 0 ? "(none)" : string.Join(",", found);
        }

        private static string Trim(string value, int max)
            => string.IsNullOrEmpty(value) ? "(unnamed)"
               : value.Length <= max ? value : value.Substring(0, max - 1) + "\u2026";

        // ---- helpers -----------------------------------------------------------

        private void ForEachPresenter(Action<InkPresenter> action)
        {
            if (CanvasA?.InkPresenter is { } presenter)
            {
                action(presenter);
            }
        }

        // StatusText is a polite live region, so updating it is what Narrator announces.
        private void Announce(string message)
        {
            if (StatusText is not null)
            {
                StatusText.Text = message;
            }
        }
    }
}
