// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

using System;
using System.Collections.Generic;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using Windows.Foundation;
using Windows.Storage;
using Windows.UI;
using Windows.UI.Core;
using Inking = Windows.UI.Input.Inking;

namespace InkBugBash
{
    public sealed partial class MainWindow : Window
    {
        private readonly InkCanvas _canvas = new InkCanvas();
        private InkToolbar Toolbar;
        private readonly Random _random = new Random(1);
        private readonly List<Point> _lassoPoints = new List<Point>();
        private bool _lassoActive;
        private bool _fitToWindow;
        private int _strokeCount;
        private Microsoft.UI.Xaml.Controls.InkSynchronizer _synchronizer;

        // The control draws no selection adorner, so the app has to show what is selected.
        private readonly Microsoft.UI.Xaml.Shapes.Rectangle _selectionBox = new Microsoft.UI.Xaml.Shapes.Rectangle
        {
            HorizontalAlignment = HorizontalAlignment.Left,
            VerticalAlignment = VerticalAlignment.Top,
            Stroke = new SolidColorBrush(Microsoft.UI.Colors.DodgerBlue),
            StrokeThickness = 2,
            StrokeDashArray = new DoubleCollection { 4, 3 },
            Fill = new SolidColorBrush(Color.FromArgb(32, 30, 144, 255)),
            IsHitTestVisible = false,
            Visibility = Visibility.Collapsed,
        };

        // Transparent sheet over the canvas that collects the lasso drag.
        private readonly Grid _lassoLayer = new Grid
        {
            Background = new SolidColorBrush(Microsoft.UI.Colors.Transparent),
            Visibility = Visibility.Collapsed,
        };

        // Under custom drying the control stops rendering committed ink, so the app draws it here.
        private readonly Canvas _dryLayer = new Canvas { IsHitTestVisible = false };

        public MainWindow()
        {
            App.Log("ctor: begin");
            InitializeComponent();
            App.Log("ctor: InitializeComponent done");
            WireEvents();
            App.Log("ctor: WireEvents done");
            Title = "WinUI 3 Ink Bug Bash";

            _canvas.Width = 1600;
            _canvas.Height = 1200;
            Microsoft.UI.Xaml.Automation.AutomationProperties.SetName(_canvas, "Drawing surface");
            Microsoft.UI.Xaml.Automation.AutomationProperties.SetAutomationId(_canvas, "Canvas");

            // ActivateCustomDrying must run before the canvas loads, which is why the canvas is
            // created here and only added to the tree afterwards.
            if (App.CustomDrying)
            {
                try
                {
                    _synchronizer = _canvas.InkPresenter.ActivateCustomDrying();
                    DryingText.Text = "Custom drying is ON. Draw a stroke: it appears in your pen colour while the pen is down, then turns PINK the moment you lift it - that pink line is drawn by the app, not the control.";
                    DryingButton.Content = "Restart in default drying mode";
                }
                catch (Exception ex)
                {
                    DryingText.Text = $"ActivateCustomDrying failed: {ex.Message}";
                }
            }

            CanvasHost.Children.Insert(0, _canvas);
            CanvasHost.Children.Add(_dryLayer);
            CanvasHost.Children.Add(_selectionBox);
            CanvasHost.Children.Add(_lassoLayer);
            App.Log("ctor: canvas inserted");
            BuildToolbar();
            App.Log("ctor: BuildToolbar done");

            var presenter = _canvas.InkPresenter;
            presenter.StrokesCollected += OnStrokesCollected;
            presenter.StrokesErased += (_, args) => { _strokeCount = Math.Max(0, _strokeCount - args.Strokes.Count); Status($"erased {args.Strokes.Count}"); };

            // The lasso gets its input from this overlay, not from InkPresenter.UnprocessedInput:
            // the ink surface consumes pointer input below the XAML layer, so the overlay only
            // receives anything once ink input is switched off (see OnModeChanged).
            _lassoLayer.PointerPressed += OnLassoPointerPressed;
            _lassoLayer.PointerMoved += OnLassoPointerMoved;
            _lassoLayer.PointerReleased += OnLassoPointerReleased;
            _lassoLayer.PointerCaptureLost += OnLassoPointerReleased;
            _lassoLayer.PointerCanceled += OnLassoPointerReleased;

            OnInputDevicesChanged(null, null);
            OnInputConfigChanged(null, null);
            Status("Ready.");
            App.Log("ctor: end");
        }

        private void OnStrokesCollected(
            Microsoft.UI.Xaml.Controls.InkPresenter sender,
            Microsoft.UI.Xaml.Controls.InkStrokesCollectedEventArgs args)
        {
            if (_synchronizer is not null)
            {
                // BeginDry hands over the committed strokes and stops the control drawing them, so
                // the app must render them itself or they simply disappear.
                var dry = _synchronizer.BeginDry();
                DrawDryStrokes(dry);
                _synchronizer.EndDry();
                Status($"custom drying: the stroke turned pink - the app drew {dry.Count} of {_dryLayer.Children.Count} stroke(s)");
                return;
            }
            _strokeCount += args.Strokes.Count;
            Status($"collected {args.Strokes.Count}");
        }

        private void DrawDryStrokes(IReadOnlyList<Inking.InkStroke> strokes)
        {
            foreach (var stroke in strokes)
            {
                var attributes = stroke.DrawingAttributes;
                var line = new Microsoft.UI.Xaml.Shapes.Polyline
                {
                    // Deliberately not the pen colour: it has to be obvious that the app drew this, not the control.
                    Stroke = new SolidColorBrush(Microsoft.UI.Colors.MediumVioletRed),
                    StrokeThickness = attributes.Size.Width,
                    StrokeLineJoin = PenLineJoin.Round,
                    StrokeStartLineCap = PenLineCap.Round,
                    StrokeEndLineCap = PenLineCap.Round,
                };

                foreach (var point in stroke.GetInkPoints())
                {
                    line.Points.Add(point.Position);
                }

                _dryLayer.Children.Add(line);
            }
        }

        // StrokeContainer throws while custom drying is active, so every reader has to check first.
        private bool CustomDryingActive => _synchronizer != null;

        private bool NeedContainer()
        {
            if (!CustomDryingActive) { return true; }
            Status("not available while custom drying is active - use section 5 to restart in default drying mode");
            return false;
        }

        private void Status(string note = null)
        {
            // Never enumerate the stroke container here: GetStrokes()/BoundingRect marshal synchronously
            // off the ink thread, and Status runs on every draw, erase, tool switch and scroll frame.
            string ink = CustomDryingActive
                ? "strokes: owned by the app (custom drying)"
                : $"strokes: {_strokeCount}";

            StatusText.Text =
                $"compositor: {App.CompositorStatus}   drying: {(CustomDryingActive ? "custom" : "default")}   " +
                $"{ink}   zoom: {CanvasScroll.ZoomFactor:F2}x" + (note is null ? "" : $"   [{note}]");
        }

        // Authoritative recount for the infrequent operations that change the count by an unknown amount
        // (load, paste, delete). One synchronous marshal per user click is fine; the hot paths keep
        // _strokeCount up to date incrementally instead.
        private void SyncStrokeCount()
        {
            _strokeCount = CustomDryingActive ? 0 : _canvas.InkPresenter.StrokeContainer.GetStrokes().Count;
        }

        private void OnViewChanged(object sender, ScrollViewerViewChangedEventArgs e) => Status();

        // Attaching these in markup makes LoadComponent fail on this local build, so they are bound here.
        private void WireEvents()
        {
            CanvasScroll.ViewChanged += OnViewChanged;

            InitialControlsBox.SelectionChanged += OnInitialControlsChanged;
            FlyoutPlacementBox.SelectionChanged += OnFlyoutPlacementChanged;
            ModeBox.SelectionChanged += OnModeChanged;
            RightDragBox.SelectionChanged += OnRightDragChanged;
            HcBox.SelectionChanged += OnHighContrastChanged;
            SizeBox.SelectionChanged += OnCanvasSizeChanged;
            CanvasScroll.SizeChanged += OnCanvasScrollSizeChanged;

            VerticalToolbar.Toggled += OnOrientationToggled;

            LassoToggle.Checked += OnLassoToggle;
            LassoToggle.Unchecked += OnLassoToggle;

            SelectAllBtn.Click += OnSelectAll;
            SelectWithLineBtn.Click += OnSelectWithLine;
            MoveSelectedBtn.Click += OnMoveSelected;
            DeleteSelectedBtn.Click += OnDeleteSelected;
            CopyBtn.Click += OnCopy;
            PasteBtn.Click += OnPaste;
            SaveBtn.Click += OnSave;
            SaveGifBtn.Click += OnSaveGif;
            LoadBtn.Click += OnLoad;
            StressBtn.Click += OnStress;
            AddOneBtn.Click += OnAddOne;
            GetByIdBtn.Click += OnGetById;
            ClearBtn.Click += OnClear;
            DryingButton.Click += OnToggleDrying;

            // Defaults are applied first: setting IsChecked raises Checked, and the handlers
            // need the toolbar, which is not built until later in the constructor.
            foreach (var box in new[]
            {
                ClearAllVisible,
                PenInput, MouseInput, TouchInput, InputEnabled, EraserTip,
            })
            {
                box.IsChecked = true;
            }

            foreach (var (box, handler) in new (CheckBox, RoutedEventHandler)[]
            {
                (ClearAllVisible, OnClearAllVisibleChanged),
                (PenInput, OnInputDevicesChanged),
                (MouseInput, OnInputDevicesChanged),
                (TouchInput, OnInputDevicesChanged),
                (InputEnabled, OnInputEnabledChanged),
                (BarrelButton, OnInputConfigChanged),
                (EraserTip, OnInputConfigChanged),
            })
            {
                box.Checked += handler;
                box.Unchecked += handler;
            }
        }

        // ---- toolbar -------------------------------------------------------

        private InkToolbarPenButton ActivePen => Toolbar?.ActiveTool as InkToolbarPenButton;

        private InkToolbarStencilButton Stencil =>
            Toolbar?.GetMenuButton(InkToolbarMenuKind.Stencil) as InkToolbarStencilButton;

        // InkToolbar reads InitialControls once, while it auto-populates, so changing it can only
        // take effect on a fresh toolbar. The other toolbar settings are re-applied to the new one.
        private void BuildToolbar()
        {
            if (Toolbar is not null)
            {
                if (Toolbar.Parent is StackPanel oldHost) { oldHost.Children.Remove(Toolbar); }
            }

            // InitialControls must be set before the toolbar auto-populates; the rest only after it
            // is in the tree, which is what the original ordering did.
            Toolbar = new InkToolbar
            {
                InitialControls = InitialControlsBox.SelectedIndex switch
                {
                    1 => InkToolbarInitialControls.None,
                    2 => InkToolbarInitialControls.PensOnly,
                    3 => InkToolbarInitialControls.AllExceptPens,
                    _ => InkToolbarInitialControls.All,
                },
            };

            Toolbar.ActiveToolChanged += (_, __) => Status($"ActiveTool = {Toolbar.ActiveTool?.ToolKind}");
            Toolbar.InkDrawingAttributesChanged += (_, __) =>
                Status($"attributes changed: size {Toolbar.InkDrawingAttributes?.Size.Width:F0}");
            Toolbar.EraseAllClicked += (_, __) => Status("EraseAllClicked");
            Toolbar.IsStencilButtonCheckedChanged += (_, args) =>
                Status($"stencil checked -> {args.StencilKind}");

            // The tool buttons only exist once the toolbar has auto-populated.
            Toolbar.Loaded += (_, __) =>
            {
                OnClearAllVisibleChanged(null, null);
            };

            ToolbarHost.Children.Add(Toolbar);

            Toolbar.TargetInkCanvas = _canvas;
            Toolbar.ButtonFlyoutPlacement = (InkToolbarButtonFlyoutPlacement)FlyoutPlacementBox.SelectedIndex;
            Toolbar.Orientation = VerticalToolbar.IsOn ? Orientation.Vertical : Orientation.Horizontal;
        }

        private void OnInitialControlsChanged(object sender, SelectionChangedEventArgs e)
        {
            BuildToolbar();
            Status($"InitialControls = {Toolbar.InitialControls} - toolbar rebuilt");
        }

        private void OnFlyoutPlacementChanged(object sender, SelectionChangedEventArgs e)
        {
            if (Toolbar is null) { return; }
            Toolbar.ButtonFlyoutPlacement = (InkToolbarButtonFlyoutPlacement)FlyoutPlacementBox.SelectedIndex;
            Status($"ButtonFlyoutPlacement = {Toolbar.ButtonFlyoutPlacement}");
        }

        private void OnOrientationToggled(object sender, RoutedEventArgs e) =>
            Toolbar.Orientation = VerticalToolbar.IsOn ? Orientation.Vertical : Orientation.Horizontal;

        private void OnClearAllVisibleChanged(object sender, RoutedEventArgs e)
        {
            if (Toolbar?.GetToolButton(InkToolbarTool.Eraser) is InkToolbarEraserButton eraser)
            {
                eraser.IsClearAllVisible = ClearAllVisible.IsChecked == true;
                Status($"IsClearAllVisible = {eraser.IsClearAllVisible}");
            }
        }

        private void OnLassoToggle(object sender, RoutedEventArgs e)
        {
            // Lasso is just the pen set to None; reuse the existing mode logic.
            // Index 0 = None (lasso), index 1 = Inking (draw).
            ModeBox.SelectedIndex = LassoToggle.IsChecked == true ? 0 : 1;
        }

        // ---- input ----------------------------------------------------------

        private void OnInputDevicesChanged(object sender, RoutedEventArgs e)
        {
            var types = CoreInputDeviceTypes.None;
            if (PenInput.IsChecked == true) { types |= CoreInputDeviceTypes.Pen; }
            if (MouseInput.IsChecked == true) { types |= CoreInputDeviceTypes.Mouse; }
            if (TouchInput.IsChecked == true) { types |= CoreInputDeviceTypes.Touch; }
            _canvas.InkPresenter.InputDeviceTypes = types;
            Status($"InputDeviceTypes = {types}");
        }

        private void OnInputEnabledChanged(object sender, RoutedEventArgs e)
        {
            if (_lassoLayer.Visibility == Visibility.Visible)
            {
                Status("ink input stays off while the pen is set to None (lasso)");
                return;
            }
            _canvas.InkPresenter.IsInputEnabled = InputEnabled.IsChecked == true;
            Status($"IsInputEnabled = {_canvas.InkPresenter.IsInputEnabled}");
        }

        private void OnModeChanged(object sender, SelectionChangedEventArgs e)
        {
            if (_canvas is null) { return; }
            var mode = ModeBox.SelectedIndex switch
            {
                1 => InkInputProcessingMode.Inking,
                2 => InkInputProcessingMode.Erasing,
                _ => InkInputProcessingMode.None,
            };
            _canvas.InkPresenter.InputProcessingConfiguration.Mode = mode;
            ClearSelection(null);

            // The ink surface takes pointer input below the XAML layer, so the lasso overlay only
            // sees a drag once ink input is switched off.
            var lasso = mode == InkInputProcessingMode.None;
            _canvas.InkPresenter.IsInputEnabled = lasso ? false : InputEnabled.IsChecked == true;
            _lassoLayer.Visibility = lasso ? Visibility.Visible : Visibility.Collapsed;
            CanvasScroll.HorizontalScrollMode = lasso ? ScrollMode.Disabled : ScrollMode.Auto;
            CanvasScroll.VerticalScrollMode = lasso ? ScrollMode.Disabled : ScrollMode.Auto;
            CanvasScroll.ZoomMode = lasso ? ZoomMode.Disabled : ZoomMode.Enabled;

            // Keep the simple lasso toggle in section 4 in step with the pen mode.
            if (LassoToggle != null) { LassoToggle.IsChecked = lasso; }

            EventText.Text = lasso
                ? "Lasso mode: drag a loop around some ink. Drawing is off until you pick Inking again."
                : $"Pen mode: {mode}";

            Status(lasso
                ? "Mode = None - drag a loop around some ink to select it"
                : $"Mode = {mode}");
        }

        private void OnRightDragChanged(object sender, SelectionChangedEventArgs e)
        {
            if (_canvas is null) { return; }
            _canvas.InkPresenter.InputProcessingConfiguration.RightDragAction =
                RightDragBox.SelectedIndex == 1
                    ? InkInputRightDragAction.AllowProcessing
                    : InkInputRightDragAction.LeaveUnprocessed;
            Status($"RightDragAction = {_canvas.InkPresenter.InputProcessingConfiguration.RightDragAction}");
        }

        private void OnInputConfigChanged(object sender, RoutedEventArgs e)
        {
            var config = _canvas.InkPresenter.InputConfiguration;
            config.IsPrimaryBarrelButtonInputEnabled = BarrelButton.IsChecked == true;
            config.IsEraserInputEnabled = EraserTip.IsChecked == true;
        }

        private void OnHighContrastChanged(object sender, SelectionChangedEventArgs e)
        {
            if (_canvas is null) { return; }
            _canvas.InkPresenter.HighContrastAdjustment = (InkHighContrastAdjustment)HcBox.SelectedIndex;
            Status($"HighContrastAdjustment = {_canvas.InkPresenter.HighContrastAdjustment}");
        }

        // ---- lasso ----------------------------------------------------------

        private void OnLassoPointerPressed(object sender, PointerRoutedEventArgs e)
        {
            var p = e.GetCurrentPoint(CanvasHost).Position;
            _lassoPoints.Clear();
            LassoPath.Points.Clear();
            _lassoPoints.Add(p);
            LassoPath.Points.Add(p);
            LassoPath.Visibility = Visibility.Visible;
            _lassoActive = true;
            _lassoLayer.CapturePointer(e.Pointer);
        }

        private void OnLassoPointerMoved(object sender, PointerRoutedEventArgs e)
        {
            if (!_lassoActive) { return; }
            var p = e.GetCurrentPoint(CanvasHost).Position;
            _lassoPoints.Add(p);
            LassoPath.Points.Add(p);
        }

        private void OnLassoPointerReleased(object sender, PointerRoutedEventArgs e)
        {
            if (!_lassoActive) { return; }
            _lassoLayer.ReleasePointerCaptures();
            LassoFinish();
        }

        private void LassoFinish()
        {
            if (!_lassoActive) { return; }
            _lassoActive = false;
            LassoPath.Visibility = Visibility.Collapsed;
            if (!NeedContainer()) { return; }
            if (_lassoPoints.Count < 3) { Status("lasso too short - drag a loop around some ink"); return; }

            // SelectWithPolyLine needs a closed loop; an open path selects nothing.
            var loop = new List<Point>(_lassoPoints);
            var first = loop[0];
            var last = loop[loop.Count - 1];
            if (first.X != last.X || first.Y != last.Y) { loop.Add(first); }

            var rect = _canvas.InkPresenter.StrokeContainer.SelectWithPolyLine(loop);
            if (SelectedCount() == 0)
            {
                _selectionBox.Visibility = Visibility.Collapsed;
                var total = _canvas.InkPresenter.StrokeContainer.GetStrokes().Count;
                Status(total == 0
                    ? "lasso: the canvas is empty - draw some ink first"
                    : $"lasso: nothing selected. A stroke is only picked up when the whole of it is inside the loop - {total} stroke(s) on the canvas.");
                return;
            }

            ShowSelection(rect, "lasso");
        }

        // ---- selection / clipboard -------------------------------------------

        private int SelectedCount()
        {
            if (CustomDryingActive) { return 0; }
            int n = 0;
            foreach (var s in _canvas.InkPresenter.StrokeContainer.GetStrokes())
            {
                if (s.Selected) { n++; }
            }
            return n;
        }

        private void ShowSelection(Rect rect, string what)
        {
            var count = SelectedCount();
            if (count == 0 || rect.Width <= 0 || rect.Height <= 0)
            {
                _selectionBox.Visibility = Visibility.Collapsed;
                Status($"{what}: nothing selected");
                return;
            }

            _selectionBox.Margin = new Thickness(rect.X, rect.Y, 0, 0);
            _selectionBox.Width = rect.Width;
            _selectionBox.Height = rect.Height;
            _selectionBox.Visibility = Visibility.Visible;
            Status($"{what}: {count} stroke(s) selected, box {rect.Width:F0}x{rect.Height:F0} at ({rect.X:F0},{rect.Y:F0})");
        }

        private void ClearSelection(string why)
        {
            if (!CustomDryingActive)
            {
                foreach (var s in _canvas.InkPresenter.StrokeContainer.GetStrokes()) { s.Selected = false; }
            }
            _selectionBox.Visibility = Visibility.Collapsed;
            if (why != null) { Status(why); }
        }

        private void OnSelectAll(object sender, RoutedEventArgs e)
        {
            if (!NeedContainer()) { return; }
            var container = _canvas.InkPresenter.StrokeContainer;
            var b = container.BoundingRect;
            if (b.Width <= 0 || b.Height <= 0) { Status("nothing to select - draw something first"); return; }

            const double pad = 8;
            var rect = container.SelectWithPolyLine(new List<Point>
            {
                new Point(b.Left - pad, b.Top - pad),
                new Point(b.Right + pad, b.Top - pad),
                new Point(b.Right + pad, b.Bottom + pad),
                new Point(b.Left - pad, b.Bottom + pad),
                new Point(b.Left - pad, b.Top - pad),
            });
            ShowSelection(rect, "select every stroke");
        }

        private void OnSelectWithLine(object sender, RoutedEventArgs e)
        {
            if (!NeedContainer()) { return; }
            var b = _canvas.InkPresenter.StrokeContainer.BoundingRect;
            if (b.Width <= 0) { Status("nothing to select - draw something first"); return; }
            var rect = _canvas.InkPresenter.StrokeContainer.SelectWithLine(
                new Point(b.Left - 10, b.Top + b.Height / 2),
                new Point(b.Right + 10, b.Top + b.Height / 2));
            ShowSelection(rect, "select with a line across the middle");
        }

        private void OnMoveSelected(object sender, RoutedEventArgs e)
        {
            if (!NeedContainer()) { return; }
            if (SelectedCount() == 0) { Status("nothing selected - lasso some ink or press Select every stroke"); return; }
            var rect = _canvas.InkPresenter.StrokeContainer.MoveSelected(new Point(20, 20));
            ShowSelection(rect, "moved by 20,20");
        }

        private void OnDeleteSelected(object sender, RoutedEventArgs e)
        {
            if (!NeedContainer()) { return; }
            if (SelectedCount() == 0) { Status("nothing selected - lasso some ink or press Select every stroke"); return; }
            _canvas.InkPresenter.StrokeContainer.DeleteSelected();
            SyncStrokeCount();
            _selectionBox.Visibility = Visibility.Collapsed;
            Status($"deleted the selection, {_strokeCount} stroke(s) left");
        }

        private void OnCopy(object sender, RoutedEventArgs e)
        {
            if (!NeedContainer()) { return; }
            _canvas.InkPresenter.StrokeContainer.CopySelectedToClipboard();
            Status("copied selection to clipboard");
        }

        private void OnPaste(object sender, RoutedEventArgs e)
        {
            if (!NeedContainer()) { return; }
            var container = _canvas.InkPresenter.StrokeContainer;
            if (!container.CanPasteFromClipboard()) { Status("clipboard has no ink"); return; }
            var rect = container.PasteFromClipboard(new Point(100, 100));
            SyncStrokeCount();
            Status($"pasted -> {rect.Width:F0}x{rect.Height:F0}");
        }

        // ---- strokes / file / stress -----------------------------------------

        // Unpackaged app: the pickers need the window handle before they can be shown.
        private IntPtr Hwnd => WinRT.Interop.WindowNative.GetWindowHandle(this);

        private void ShowFile(string verb, StorageFile file)
        {
            LastFileText.Text = $"Last {verb}: {file.Path}";
            Status($"{verb} {file.Path}");
        }

        private async void OnSave(object sender, RoutedEventArgs e) => await SaveAsync(false);

        private async void OnSaveGif(object sender, RoutedEventArgs e) => await SaveAsync(true);

        private async System.Threading.Tasks.Task SaveAsync(bool gif)
        {
            try
            {
                if (!NeedContainer()) { return; }
                if (_canvas.InkPresenter.StrokeContainer.GetStrokes().Count == 0)
                {
                    Status("nothing to save - draw something first");
                    return;
                }

                var picker = new Windows.Storage.Pickers.FileSavePicker
                {
                    SuggestedStartLocation = Windows.Storage.Pickers.PickerLocationId.Desktop,
                    SuggestedFileName = gif ? "ink" : "ink",
                };
                WinRT.Interop.InitializeWithWindow.Initialize(picker, Hwnd);
                picker.FileTypeChoices.Add(
                    gif ? "GIF with embedded ink" : "Ink Serialized Format",
                    new List<string> { gif ? ".gif" : ".isf" });

                var file = await picker.PickSaveFileAsync();
                if (file is null) { Status("save cancelled"); return; }

                using (var stream = await file.OpenAsync(FileAccessMode.ReadWrite))
                {
                    stream.Size = 0;
                    var container = _canvas.InkPresenter.StrokeContainer;
                    if (gif)
                    {
                        // Projected as a SaveAsync overload via [overload("SaveAsync")] in the IDL.
                        await container.SaveAsync(stream.GetOutputStreamAt(0), Inking.InkPersistenceFormat.GifWithEmbeddedIsf);
                    }
                    else
                    {
                        await container.SaveAsync(stream.GetOutputStreamAt(0));
                    }
                }

                ShowFile("saved to", file);
            }
            catch (Exception ex)
            {
                Status($"save failed: {ex.Message}");
            }
        }

        private async void OnLoad(object sender, RoutedEventArgs e)
        {
            try
            {
                if (!NeedContainer()) { return; }
                var picker = new Windows.Storage.Pickers.FileOpenPicker
                {
                    SuggestedStartLocation = Windows.Storage.Pickers.PickerLocationId.Desktop,
                };
                WinRT.Interop.InitializeWithWindow.Initialize(picker, Hwnd);
                picker.FileTypeFilter.Add(".isf");
                picker.FileTypeFilter.Add(".gif");

                var file = await picker.PickSingleFileAsync();
                if (file is null) { Status("open cancelled"); return; }

                using (var stream = await file.OpenAsync(FileAccessMode.Read))
                {
                    await _canvas.InkPresenter.StrokeContainer.LoadAsync(stream);
                }

                SyncStrokeCount();
                ShowFile("opened", file);
            }
            catch (Exception ex)
            {
                Status($"open failed: {ex.Message}");
            }
        }

        private Inking.InkStroke BuildStroke(double x, double y, int points)
        {
            var builder = new Inking.InkStrokeBuilder();
            var list = new List<Point>();
            for (int p = 0; p < points; p++) { list.Add(new Point(x + p * 4, y + Math.Sin(p * 0.5) * 20)); }
            return builder.CreateStroke(list);
        }

        private void OnStress(object sender, RoutedEventArgs e)
        {
            if (!NeedContainer()) { return; }
            var strokes = new List<Inking.InkStroke>();
            for (int i = 0; i < 500; i++)
            {
                strokes.Add(BuildStroke(_random.NextDouble() * 1400 + 50, _random.NextDouble() * 1000 + 50, 24));
            }
            _canvas.InkPresenter.StrokeContainer.AddStrokes(strokes);
            _strokeCount += strokes.Count;
            Status("added 500 strokes");
        }

        private void OnAddOne(object sender, RoutedEventArgs e)
        {
            if (!NeedContainer()) { return; }
            _canvas.InkPresenter.StrokeContainer.AddStroke(
                BuildStroke(_random.NextDouble() * 1400 + 50, _random.NextDouble() * 1000 + 50, 40));
            _strokeCount += 1;
            Status("added 1 stroke");
        }

        private void OnGetById(object sender, RoutedEventArgs e)
        {
            if (!NeedContainer()) { return; }
            var strokes = _canvas.InkPresenter.StrokeContainer.GetStrokes();
            if (strokes.Count == 0) { Status("no strokes"); return; }
            var id = strokes[0].Id;
            var found = _canvas.InkPresenter.StrokeContainer.GetStrokeById(id);
            Status($"GetStrokeById({id}) -> {(found is null ? "null" : "found")}");
        }

        private void OnClear(object sender, RoutedEventArgs e)
        {
            if (CustomDryingActive)
            {
                _dryLayer.Children.Clear();
                Status("cleared the ink the app drew under custom drying");
                return;
            }
            if (!NeedContainer()) { return; }
            _canvas.InkPresenter.StrokeContainer.Clear();
            _strokeCount = 0;
            _selectionBox.Visibility = Visibility.Collapsed;
            Status("cleared");
        }

        private void OnCanvasSizeChanged(object sender, SelectionChangedEventArgs e)
        {
            if (_canvas is null) { return; }
            switch (SizeBox.SelectedIndex)
            {
                // InkCanvas has no intrinsic size, so NaN would collapse it to 0x0; size it to the viewport instead.
                case 0: _fitToWindow = true; FitCanvasToWindow(); break;
                case 2: _fitToWindow = false; _canvas.Width = 4000; _canvas.Height = 3000; break;
                default: _fitToWindow = false; _canvas.Width = 1600; _canvas.Height = 1200; break;
            }
            Status($"canvas = {_canvas.Width:F0}x{_canvas.Height:F0}");
        }

        private void FitCanvasToWindow()
        {
            var w = CanvasScroll.ViewportWidth;
            var h = CanvasScroll.ViewportHeight;
            if (w > 0) { _canvas.Width = w; }
            if (h > 0) { _canvas.Height = h; }
        }

        private void OnCanvasScrollSizeChanged(object sender, SizeChangedEventArgs e)
        {
            if (_fitToWindow) { FitCanvasToWindow(); }
        }

        private void OnToggleDrying(object sender, RoutedEventArgs e)
        {
            App.SetCustomDrying(!App.CustomDrying);
            Status("restarting...");
            Microsoft.Windows.AppLifecycle.AppInstance.Restart("");
        }
    }
}
