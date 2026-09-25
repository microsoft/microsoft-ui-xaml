// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;

namespace InkParityLab
{
    public sealed partial class MainWindow : Window
    {
        public MainWindow()
        {
            this.InitializeComponent();
        }

        private void OnRootLoaded(object sender, RoutedEventArgs e)
        {
            ToolbarA.TargetInkCanvas = CanvasA;
            ToolbarB.TargetInkCanvas = CanvasB;
            ApplyInputDevices();

            // Instrumentation: proves whether a click reached the toolbar and whether ink reached the canvas.
            ToolbarA.ActiveToolChanged += (s, _) => Announce($"A: tool changed -> {ToolName(ToolbarA)} (clicks={++_toolbarAClicks})");
            ToolbarB.ActiveToolChanged += (s, _) => Announce($"B(overlay): tool changed -> {ToolName(ToolbarB)} (clicks={++_toolbarBClicks})");

            CanvasA.InkPresenter.StrokesCollected += (s, args) =>
                Announce($"A: stroke collected (total={_strokesA += args.Strokes.Count})");
            CanvasB.InkPresenter.StrokesCollected += (s, args) =>
                Announce($"B(overlay): stroke collected (total={_strokesB += args.Strokes.Count})");

            Announce($"Compositor: {App.CompositorStatus}");

            // Diagnostic: is the CornerRadius the app set actually reaching the template root?
            var cr = ToolbarB.CornerRadius;
            var root = Microsoft.UI.Xaml.Media.VisualTreeHelper.GetChild(ToolbarB, 0) as FrameworkElement;
            string rootCr = root switch
            {
                Border b => $"Border.CornerRadius={b.CornerRadius.TopLeft}",
                StackPanel sp => $"StackPanel.CornerRadius={sp.CornerRadius.TopLeft} Padding={sp.Padding}",
                _ => "(root has no CornerRadius)"
            };
            StatusText.Text =
                $"ToolbarB.CornerRadius={cr.TopLeft} | root={root?.GetType().Name ?? "(null)"} | {rootCr}";
        }

        private int _toolbarAClicks;
        private int _toolbarBClicks;
        private int _strokesA;
        private int _strokesB;

        private static string ToolName(InkToolbar toolbar)
        {
            return toolbar.ActiveTool is null ? "(none)" : toolbar.ActiveTool.GetType().Name;
        }

        private void OnInputDevicesChanged(object sender, RoutedEventArgs e)
        {
            ApplyInputDevices();
        }

        private void ApplyInputDevices()
        {
            // Null-guarded: the CheckBox Checked events fire during XAML load, before OnRootLoaded.
            if (CanvasA is null || CanvasB is null)
            {
                return;
            }

            var types = Windows.UI.Core.CoreInputDeviceTypes.None;
            if (PenInput.IsChecked == true) { types |= Windows.UI.Core.CoreInputDeviceTypes.Pen; }
            if (MouseInput.IsChecked == true) { types |= Windows.UI.Core.CoreInputDeviceTypes.Mouse; }
            if (TouchInput.IsChecked == true) { types |= Windows.UI.Core.CoreInputDeviceTypes.Touch; }

            CanvasA.InkPresenter.InputDeviceTypes = types;
            CanvasB.InkPresenter.InputDeviceTypes = types;
            Announce($"Input devices set to {types}.");
        }

        private void OnHighContrastModeChanged(object sender, SelectionChangedEventArgs e)
        {
            if (CanvasA is null || CanvasB is null)
            {
                return;
            }

            var mode = HighContrastMode.SelectedIndex switch
            {
                1 => InkHighContrastAdjustment.UseSystemColors,
                2 => InkHighContrastAdjustment.UseOriginalColors,
                _ => InkHighContrastAdjustment.UseSystemColorsWhenNecessary,
            };

            CanvasA.InkPresenter.HighContrastAdjustment = mode;
            CanvasB.InkPresenter.HighContrastAdjustment = mode;
            Announce($"High contrast adjustment set to {mode}.");
        }

        private void OnToolbarOrientationToggled(object sender, RoutedEventArgs e)
        {
            if (ToolbarA is null || ToolbarB is null)
            {
                return;
            }

            var orientation = VerticalToolbars.IsOn ? Orientation.Vertical : Orientation.Horizontal;
            ToolbarA.Orientation = orientation;
            ToolbarB.Orientation = orientation;
            Announce($"Toolbars are now {orientation}.");
        }

        private void OnClearBoth(object sender, RoutedEventArgs e)
        {
            CanvasA.InkPresenter.StrokeContainer.Clear();
            CanvasB.InkPresenter.StrokeContainer.Clear();
            Announce("Both surfaces cleared.");
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
