// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using Windows.Foundation;
using Windows.Storage.Streams;
using Windows.UI.Core;
using Windows.UI.Input.Inking;
using Windows.UI.Input.Inking.Analysis;

namespace InkGridTestApp
{
    public sealed partial class MainWindow : Window
    {
        private const int Rows = 4;
        private const int Columns = 4;

        private readonly List<InkCanvas> _canvases = new();
        private InkCanvas _focusedCanvas;
        private bool _initializing;

        public MainWindow()
        {
            this.InitializeComponent();

            _initializing = true;
            bool useSystem = App.ReadUseSystemVisualLink();
            CompositorToggle.IsOn = useSystem;
            _initializing = false;

            ModeStatusText.Text =
                $"Active: {(useSystem ? "System/CEOL" : "Lifted")} compositor. " +
                "Changing the switch persists it; click \"Restart to apply\".";

            BuildInkGrid();
        }

        private void BuildInkGrid()
        {
            for (int r = 0; r < Rows; r++)
            {
                InkGridHost.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto });
            }
            for (int c = 0; c < Columns; c++)
            {
                InkGridHost.ColumnDefinitions.Add(new ColumnDefinition { Width = GridLength.Auto });
            }

            for (int r = 0; r < Rows; r++)
            {
                for (int c = 0; c < Columns; c++)
                {
                    var canvas = new InkCanvas();
                    canvas.InkPresenter.InputDeviceTypes =
                        CoreInputDeviceTypes.Mouse | CoreInputDeviceTypes.Pen | CoreInputDeviceTypes.Touch;

                    // Track which cell was last interacted with (used by "Analyze focused" and the InkToolBar).
                    canvas.PointerPressed += (s, e) => SetFocusedCanvas((InkCanvas)s);
                    canvas.PointerEntered += (s, e) => _focusedCanvas = (InkCanvas)s;

                    _canvases.Add(canvas);

                    var header = new TextBlock
                    {
                        Text = $"Cell [{r},{c}]",
                        Margin = new Thickness(6, 4, 6, 4),
                        FontSize = 12,
                        Opacity = 0.7
                    };

                    var cellGrid = new Grid();
                    cellGrid.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto });
                    cellGrid.RowDefinitions.Add(new RowDefinition { Height = new GridLength(1, GridUnitType.Star) });
                    Grid.SetRow(header, 0);
                    Grid.SetRow(canvas, 1);
                    cellGrid.Children.Add(header);
                    cellGrid.Children.Add(canvas);

                    var border = new Border
                    {
                        BorderBrush = new SolidColorBrush(Microsoft.UI.Colors.Gray),
                        BorderThickness = new Thickness(1),
                        Background = new SolidColorBrush(Microsoft.UI.Colors.White),
                        Width = 300,
                        Height = 240,
                        Margin = new Thickness(4),
                        Child = cellGrid
                    };

                    Grid.SetRow(border, r);
                    Grid.SetColumn(border, c);
                    InkGridHost.Children.Add(border);
                }
            }

            if (_canvases.Count > 0)
            {
                SetFocusedCanvas(_canvases[0]);
            }

            // Apply the initial Tool/Color/Size to every cell.
            ApplyDrawingAttributesToAll();
        }

        // Points the InkToolBar at the given cell so its buttons (pen/pencil/highlighter/eraser/
        // stencil) act on the cell the user last pressed.
        private void SetFocusedCanvas(InkCanvas canvas)
        {
            _focusedCanvas = canvas;
            if (Toolbar != null && canvas != null)
            {
                Toolbar.TargetInkCanvas = canvas;
            }
        }

        private void OnCompositorToggled(object sender, RoutedEventArgs e)
        {
            if (_initializing)
            {
                return;
            }

            App.WriteUseSystemVisualLink(CompositorToggle.IsOn);
            ModeStatusText.Text =
                $"Pending: {(CompositorToggle.IsOn ? "System (ContentExternalOutputLink)" : "Lifted (topmost overlay)")}. " +
                "Click \"Restart to apply\" to load InkCanvas with the new compositor path.";
        }

        private void OnDrawingAttributesChanged(object sender, object e)
        {
            ApplyDrawingAttributesToAll();
        }

        private void ApplyDrawingAttributesToAll()
        {
            if (_canvases.Count == 0 || ColorCombo == null || ToolCombo == null || SizeSlider == null)
            {
                return;
            }

            var color = ((ColorCombo.SelectedItem as ComboBoxItem)?.Content as string) switch
            {
                "Red" => Microsoft.UI.Colors.Red,
                "Blue" => Microsoft.UI.Colors.Blue,
                "Green" => Microsoft.UI.Colors.Green,
                "Orange" => Microsoft.UI.Colors.Orange,
                "Purple" => Microsoft.UI.Colors.Purple,
                _ => Microsoft.UI.Colors.Black,
            };

            var tool = (ToolCombo.SelectedItem as ComboBoxItem)?.Content as string ?? "Pen";
            bool highlighter = tool == "Highlighter";
            bool eraser = tool == "Eraser";
            double size = SizeSlider.Value;

            foreach (var canvas in _canvases)
            {
                var attributes = canvas.InkPresenter.CopyDefaultDrawingAttributes();
                attributes.Color = color;
                attributes.Size = new Size(size, size);
                attributes.DrawAsHighlighter = highlighter;
                attributes.PenTip = highlighter ? PenTipShape.Rectangle : PenTipShape.Circle;
                canvas.InkPresenter.UpdateDefaultDrawingAttributes(attributes);

                // Pen/Highlighter => inking; Eraser => erasing.
                canvas.InkPresenter.InputProcessingConfiguration.Mode =
                    eraser ? Microsoft.UI.Xaml.Controls.InkInputProcessingMode.Erasing : Microsoft.UI.Xaml.Controls.InkInputProcessingMode.Inking;
            }

            ScenarioStatusText.Text =
                $"Applied to ALL cells: tool={tool}, color={((ComboBoxItem)ColorCombo.SelectedItem).Content}, size={size:0}.";
        }

        private void OnRestartClick(object sender, RoutedEventArgs e)
        {
            try
            {
                var exePath = Environment.ProcessPath;
                if (!string.IsNullOrEmpty(exePath))
                {
                    Process.Start(new ProcessStartInfo { FileName = exePath, UseShellExecute = true });
                }
            }
            catch
            {
                // If relaunch fails, still exit so the user can start it manually.
            }

            Application.Current.Exit();
        }

        private void OnCountAllClick(object sender, RoutedEventArgs e)
        {
            int total = 0;
            foreach (var canvas in _canvases)
            {
                total += canvas.InkPresenter.StrokeContainer.GetStrokes().Count;
            }
            ScenarioStatusText.Text = $"Total strokes across all {_canvases.Count} canvases: {total}.";
        }

        private void OnClearAllClick(object sender, RoutedEventArgs e)
        {
            foreach (var canvas in _canvases)
            {
                canvas.InkPresenter.StrokeContainer.Clear();
            }
            ScenarioStatusText.Text = "Cleared strokes on all canvases.";
        }

        private async void OnSaveReloadAllClick(object sender, RoutedEventArgs e)
        {
            int roundTripped = 0;
            foreach (var canvas in _canvases)
            {
                var container = canvas.InkPresenter.StrokeContainer;
                if (container.GetStrokes().Count == 0)
                {
                    continue;
                }

                using var stream = new InMemoryRandomAccessStream();
                await container.SaveAsync(stream.GetOutputStreamAt(0));
                container.Clear();
                await container.LoadAsync(stream.GetInputStreamAt(0));
                roundTripped += container.GetStrokes().Count;
            }
            ScenarioStatusText.Text = $"Saved and reloaded {roundTripped} stroke(s) across the grid.";
        }

        private async void OnAnalyzeFocusedClick(object sender, RoutedEventArgs e)
        {
            if (_focusedCanvas == null)
            {
                ScenarioStatusText.Text = "No focused canvas. Touch a cell first.";
                return;
            }

            var strokes = _focusedCanvas.InkPresenter.StrokeContainer.GetStrokes();
            if (strokes.Count == 0)
            {
                ScenarioStatusText.Text = "Focused canvas has no strokes to analyze.";
                return;
            }

            var analyzer = new InkAnalyzer();
            analyzer.AddDataForStrokes(strokes);
            var result = await analyzer.AnalyzeAsync();

            var words = analyzer.AnalysisRoot.FindNodes(InkAnalysisNodeKind.InkWord);
            var drawings = analyzer.AnalysisRoot.FindNodes(InkAnalysisNodeKind.InkDrawing);
            ScenarioStatusText.Text =
                $"InkAnalyzer ({result.Status}): {words.Count} word(s), {drawings.Count} drawing(s) from {strokes.Count} stroke(s).";
        }
    }
}
