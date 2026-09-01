// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Text;
using Windows.UI.Core;
using Windows.UI.Input.Inking;

namespace ScratchPadApp
{
    public sealed partial class MainWindow : Window
    {
        // One measured row per collected stroke. Dumped to CSV on demand.
        private sealed class StrokeSample
        {
            public int Index;
            public int PointCount;
            public double DurationMs;        // first->last InkPoint timestamp
            public double MeanIntervalMs;    // mean gap between consecutive InkPoints
            public double MinIntervalMs;
            public double MaxIntervalMs;
            public double JitterMs;          // stdev of intervals
            public double SampleRateHz;      // 1000 / mean interval
        }

        private readonly List<StrokeSample> _samples = new List<StrokeSample>();
        private readonly Stopwatch _appClock = Stopwatch.StartNew();

        // Frame-cadence capture (CompositionTarget.Rendering).
        private bool _frameCapture;
        private long _lastFrameQpc;
        private readonly List<double> _frameIntervalsMs = new List<double>();

        private double _ctorToLoadedMs;

        public MainWindow()
        {
            var ctorStart = _appClock.Elapsed;
            this.InitializeComponent();

            // One batch per collection; iterate its strokes.
            InkSurface.InkPresenter.StrokesCollected += InkPresenter_StrokesCollected;

            // Events added in this PR: StrokesErased plus the InkStrokeInput / InkUnprocessedInput
            // surfaces. Wire them so each can be seen firing (all are marshaled off the ink thread
            // by the product's RunOnInkHostThread path).
            WireInputEvents();

            // DIAGNOSTIC: start with the toolbar DETACHED so the first draw exercises the raw InkCanvas
            // (exactly like the app that inked on this hardware). The "Connect InkToolbar" switch
            // re-attaches it at runtime so we can compare without reinstalling.
            // (Toolbar.TargetInkCanvas is left null here; the toggle sets it.)

            // Enable ALL input devices once. The InkPresenter is a stable instance that caches this, so it
            // does not need re-applying as the toolbar / canvas load.
            ApplyInputDeviceTypes();
            Toolbar.Loaded += (s, e) =>
            {
                App.Log("Toolbar.Loaded ActiveTool=" + (Toolbar.ActiveTool?.GetType().Name ?? "null"));
            };
            InkSurface.Loaded += (s, e) =>
            {
                App.Log($"InkSurface.Loaded size = {InkSurface.ActualWidth} x {InkSurface.ActualHeight}");
            };
            // Diagnostic: log raw pointer input reaching the canvas (handledEventsToo=true so we see it
            // even when the InkCanvas handles/captures the pointer for wet-ink processing).
            InkSurface.AddHandler(UIElement.PointerPressedEvent,
                new PointerEventHandler((s, e) => App.Log("InkSurface PointerPressed dev=" + e.Pointer.PointerDeviceType)), true);

            // Reflect the persisted compositor mode in the combo.
            SelectCompositorMode(App.ReadCompositorMode());

            this.Activated += OnFirstActivated;

            UpdateStartup();
            UpdateStroke(null);
            UpdateAggregate();
            UpdateFrame();
            UpdateMemory();
            RefreshCompositorPath();
        }

        private bool _firstActivation = true;
        private void OnFirstActivated(object sender, Microsoft.UI.Xaml.WindowActivatedEventArgs e)
        {
            if (!_firstActivation) return;
            _firstActivation = false;
            _ctorToLoadedMs = (_appClock.Elapsed).TotalMilliseconds;
            UpdateStartup();
            UpdateMemory();
            // The first InkCanvas has loaded by now; the compositor decision is in the log.
            RefreshCompositorPath();
        }

        // InkPoint.Timestamp is microseconds; convert deltas to milliseconds.
        private static double UsToMs(double micros) => micros / 1000.0;

        private void ApplyInputDeviceTypes()
        {
            try
            {
                InkSurface.InkPresenter.InputDeviceTypes =
                    CoreInputDeviceTypes.Pen | CoreInputDeviceTypes.Mouse | CoreInputDeviceTypes.Touch;
                App.Log("InputDeviceTypes set = " + InkSurface.InkPresenter.InputDeviceTypes);
            }
            catch (Exception ex) { App.Log("ApplyInputDeviceTypes failed: " + ex.Message); }
        }

        private void InkPresenter_StrokesCollected(Microsoft.UI.Xaml.Controls.InkPresenter sender, Microsoft.UI.Xaml.Controls.InkStrokesCollectedEventArgs args)
        {
            App.Log("StrokesCollected fired: " + (args?.Strokes?.Count ?? 0) + " strokes");

            // Custom drying: bracket the app's own rendering of the just-committed strokes. BeginDry hands
            // back the strokes the presenter committed (a real app renders them itself here); EndDry
            // releases the wet-ink layer. A returned count of 0 means the OS BeginDry capture (only valid
            // inside this notification) did not land - the known gap this PR is about.
            if (_customDryActive && _inkSynchronizer != null)
            {
                CustomDryStep();
            }

            if (args?.Strokes == null) return;

            StrokeSample last = null;
            foreach (var stroke in args.Strokes)
            {
                var sample = ProcessStroke(stroke);
                if (sample != null) last = sample;
            }

            UpdateStroke(last);
            UpdateAggregate();
            UpdateMemory();
        }

        // ---- Events added in this PR (StrokesErased + InkStrokeInput + InkUnprocessedInput) ----

        private int _strokesErasedTotal;
        private int _strokeStartedCount, _strokeContinuedCount, _strokeEndedCount, _strokeCanceledCount;
        private int _pointerPressedCount, _pointerMovedCount, _pointerReleasedCount, _pointerHoveredCount,
                    _pointerEnteredCount, _pointerExitedCount, _pointerLostCount;
        private readonly List<string> _eventLog = new List<string>();

        // Custom drying (WIP): the synchronizer handed back by ActivateCustomDrying + running counters.
        private Microsoft.UI.Xaml.Controls.InkSynchronizer _inkSynchronizer;
        private bool _customDryActive;
        private int _customDryCollections;
        private int _customDryStrokesTotal;

        // Constant tint the app paints dried strokes with; created once, not per collection.
        private readonly SolidColorBrush _customDryTint = new SolidColorBrush(Windows.UI.Color.FromArgb(0xFF, 0xC0, 0x30, 0x30));

        // Subscribes to every event this PR adds. The high-frequency events (StrokeContinued,
        // PointerMoved, PointerHovered) are counted only; the rest are also written to a short log.
        private void WireInputEvents()
        {
            var presenter = InkSurface.InkPresenter;

            presenter.StrokesErased += (s, a) =>
            {
                _strokesErasedTotal += a?.Strokes?.Count ?? 0;
                LogInputEvent($"StrokesErased -{a?.Strokes?.Count ?? 0}");
            };

            var strokeInput = presenter.StrokeInput;
            strokeInput.StrokeStarted += (s, a) => { _strokeStartedCount++; LogInputEvent("StrokeStarted"); };
            strokeInput.StrokeContinued += (s, a) => { _strokeContinuedCount++; UpdateEventCounts(); };
            strokeInput.StrokeEnded += (s, a) => { _strokeEndedCount++; LogInputEvent("StrokeEnded"); };
            strokeInput.StrokeCanceled += (s, a) => { _strokeCanceledCount++; LogInputEvent("StrokeCanceled"); };

            var unprocessed = presenter.UnprocessedInput;
            unprocessed.PointerEntered += (s, a) => { _pointerEnteredCount++; LogInputEvent("PointerEntered"); };
            unprocessed.PointerHovered += (s, a) => { _pointerHoveredCount++; UpdateEventCounts(); };
            unprocessed.PointerExited += (s, a) => { _pointerExitedCount++; LogInputEvent("PointerExited"); };
            unprocessed.PointerPressed += (s, a) => { _pointerPressedCount++; LogInputEvent("PointerPressed"); };
            unprocessed.PointerMoved += (s, a) => { _pointerMovedCount++; UpdateEventCounts(); };
            unprocessed.PointerReleased += (s, a) => { _pointerReleasedCount++; LogInputEvent("PointerReleased"); };
            unprocessed.PointerLost += (s, a) => { _pointerLostCount++; LogInputEvent("PointerLost"); };
        }

        private void LogInputEvent(string msg)
        {
            _eventLog.Insert(0, msg);
            if (_eventLog.Count > 12) _eventLog.RemoveAt(_eventLog.Count - 1);
            if (EventLogText != null) EventLogText.Text = string.Join("\n", _eventLog);
            UpdateEventCounts();
        }

        private void UpdateEventCounts()
        {
            if (EventCountText == null) return;
            EventCountText.Text =
                $"StrokeInput: started={_strokeStartedCount} continued={_strokeContinuedCount} " +
                $"ended={_strokeEndedCount} canceled={_strokeCanceledCount}\n" +
                $"StrokesErased total = {_strokesErasedTotal}\n" +
                $"Unprocessed: pressed={_pointerPressedCount} moved={_pointerMovedCount} " +
                $"released={_pointerReleasedCount} entered={_pointerEnteredCount} " +
                $"exited={_pointerExitedCount} hovered={_pointerHoveredCount} lost={_pointerLostCount}";
        }

        // Mode=None routes pointer input to InkUnprocessedInput instead of wet-ink collection, so the
        // Pointer* events fire; Inking restores normal stroke collection (StrokesCollected/StrokeInput).
        private void OnUnprocessedModeToggled(object sender, RoutedEventArgs e)
        {
            InkSurface.InkPresenter.InputProcessingConfiguration.Mode =
                UnprocessedModeToggle.IsOn
                    ? Microsoft.UI.Xaml.Controls.InkInputProcessingMode.None
                    : Microsoft.UI.Xaml.Controls.InkInputProcessingMode.Inking;
            App.Log("InputProcessingMode = " + InkSurface.InkPresenter.InputProcessingConfiguration.Mode);
        }

        // Activate custom drying: the app takes over rendering of committed ("dry") ink. Once active,
        // each StrokesCollected brackets the app's rendering with InkSynchronizer.BeginDry()/EndDry().
        private void OnCustomDryToggled(object sender, RoutedEventArgs e)
        {
            try
            {
                if (CustomDryToggle.IsOn)
                {
                    _inkSynchronizer = InkSurface.InkPresenter.ActivateCustomDrying();
                    _customDryActive = _inkSynchronizer != null;
                    _customDryCollections = 0;
                    _customDryStrokesTotal = 0;
                    CustomDryText.Text = _customDryActive
                        ? "activated - draw a stroke; BeginDry/EndDry run on each collection"
                        : "ActivateCustomDrying returned null";
                    App.Log("Custom drying activated: " + _customDryActive);
                }
                else
                {
                    _customDryActive = false;
                    _inkSynchronizer = null;
                    CustomDryText.Text = "off";
                    App.Log("Custom drying deactivated");
                }
            }
            catch (Exception ex)
            {
                _customDryActive = false;
                CustomDryText.Text = "ActivateCustomDrying failed: " + ex.Message;
                App.Log("Custom drying activate failed: " + ex.Message);
            }
        }

        // Clip the dry-ink overlay to its own bounds so app-rendered strokes that leave the canvas (a
        // stroke keeps drawing once the pointer is captured) do not paint across the neighboring panel.
        private void OnDryOverlaySizeChanged(object sender, SizeChangedEventArgs e)
        {
            DryOverlay.Clip = new RectangleGeometry
            {
                Rect = new Windows.Foundation.Rect(0, 0, e.NewSize.Width, e.NewSize.Height)
            };
        }

        // Runs from StrokesCollected while custom drying is active: BeginDry returns the strokes the
        // presenter just committed (a real app renders them itself), then EndDry releases the wet layer.
        private void CustomDryStep()
        {
            try
            {
                var dryStrokes = _inkSynchronizer.BeginDry();
                int n = dryStrokes?.Count ?? 0;
                _customDryCollections++;
                _customDryStrokesTotal += n;

                // The app renders the dried strokes itself - the whole point of custom drying. Draw each as
                // a red polyline on our own overlay Canvas, so it is visibly the app (not InkCanvas) drawing
                // the committed ink.
                if (n > 0)
                {
                    foreach (var stroke in dryStrokes)
                    {
                        var pts = stroke.GetInkPoints();
                        if (pts == null || pts.Count < 2) { continue; }
                        var poly = new Microsoft.UI.Xaml.Shapes.Polyline { Stroke = _customDryTint, StrokeThickness = System.Math.Max(1.5, stroke.DrawingAttributes.Size.Width) };
                        var pc = new PointCollection();
                        foreach (var ip in pts) { pc.Add(ip.Position); }
                        poly.Points = pc;
                        DryOverlay.Children.Add(poly);
                    }
                }

                _inkSynchronizer.EndDry();
                CustomDryText.Text =
                    $"collection #{_customDryCollections}: app rendered {n} stroke(s) (tinted)" +
                    (n == 0 ? "  <- EMPTY" : "") +
                    $"\ntotal dried = {_customDryStrokesTotal}";
                App.Log($"CustomDry BeginDry={n}, app-rendered, EndDry ok");
            }
            catch (Exception ex)
            {
                CustomDryText.Text = "BeginDry/EndDry threw: " + ex.Message;
                App.Log("CustomDry step failed: " + ex.Message);
            }
        }

        private StrokeSample ProcessStroke(InkStroke stroke)
        {
            if (stroke == null) return null;
            var points = stroke.GetInkPoints();
            if (points == null || points.Count < 2) return null;

            var ts = new List<double>(points.Count);
            foreach (var p in points) ts.Add(p.Timestamp);

            bool haveTimestamps = ts[ts.Count - 1] > 0 && ts[ts.Count - 1] != ts[0];

            var intervals = new List<double>(points.Count - 1);
            for (int i = 1; i < ts.Count; i++)
            {
                double dMs = UsToMs(ts[i] - ts[i - 1]);
                if (dMs >= 0 && dMs < 1000) intervals.Add(dMs);
            }

            var sample = new StrokeSample { Index = _samples.Count + 1, PointCount = points.Count };

            if (haveTimestamps && intervals.Count > 0)
            {
                sample.DurationMs = UsToMs(ts[ts.Count - 1] - ts[0]);
                sample.MeanIntervalMs = intervals.Average();
                sample.MinIntervalMs = intervals.Min();
                sample.MaxIntervalMs = intervals.Max();
                double mean = sample.MeanIntervalMs;
                double var = intervals.Sum(v => (v - mean) * (v - mean)) / intervals.Count;
                sample.JitterMs = Math.Sqrt(var);
                sample.SampleRateHz = mean > 0 ? 1000.0 / mean : 0;
            }

            _samples.Add(sample);
            return sample;
        }

        private void UpdateStartup() => StartupText.Text = $"ctor -> activated: {_ctorToLoadedMs:F1} ms";

        private void UpdateStroke(StrokeSample s)
        {
            if (s == null) { StrokeText.Text = "draw a stroke..."; return; }
            if (s.SampleRateHz <= 0)
            {
                StrokeText.Text = $"#{s.Index}  points={s.PointCount}\nInkPoint.Timestamp not populated -> use WPR for rate.";
                return;
            }
            StrokeText.Text =
                $"#{s.Index}  points={s.PointCount}\n" +
                $"duration   = {s.DurationMs:F1} ms\n" +
                $"sampleRate = {s.SampleRateHz:F0} Hz\n" +
                $"interval   = {s.MeanIntervalMs:F2} ms (min {s.MinIntervalMs:F2}, max {s.MaxIntervalMs:F2})\n" +
                $"jitter(sd) = {s.JitterMs:F2} ms";
        }

        private void UpdateAggregate()
        {
            var valid = _samples.Where(x => x.SampleRateHz > 0).ToList();
            if (valid.Count == 0) { AggregateText.Text = $"strokes = {_samples.Count}"; return; }
            AggregateText.Text =
                $"strokes    = {_samples.Count}\n" +
                $"avg rate   = {valid.Average(x => x.SampleRateHz):F0} Hz\n" +
                $"avg jitter = {valid.Average(x => x.JitterMs):F2} ms\n" +
                $"avg points = {valid.Average(x => x.PointCount):F0} / stroke";
        }

        private void UpdateFrame()
        {
            if (_frameIntervalsMs.Count == 0)
            {
                FrameText.Text = _frameCapture ? "capturing... draw now" : "toggle on, then draw";
                return;
            }
            var arr = _frameIntervalsMs.ToArray();
            Array.Sort(arr);
            double mean = arr.Average();
            double p95 = arr[(int)(arr.Length * 0.95)];
            double max = arr[arr.Length - 1];
            double fps = mean > 0 ? 1000.0 / mean : 0;
            FrameText.Text =
                $"frames = {arr.Length}\n" +
                $"mean   = {mean:F2} ms ({fps:F0} fps)\n" +
                $"p95    = {p95:F2} ms\n" +
                $"max    = {max:F2} ms";
        }

        private void UpdateMemory()
        {
            try
            {
                var p = Process.GetCurrentProcess();
                p.Refresh();
                MemoryText.Text =
                    $"working set  = {p.WorkingSet64 / (1024 * 1024)} MB\n" +
                    $"private mem  = {p.PrivateMemorySize64 / (1024 * 1024)} MB\n" +
                    $"threads      = {p.Threads.Count}";
            }
            catch { }
        }

        private void OnFrameCaptureToggled(object sender, RoutedEventArgs e)
        {
            _frameCapture = FrameCaptureToggle.IsOn;
            if (_frameCapture)
            {
                _frameIntervalsMs.Clear();
                _lastFrameQpc = Stopwatch.GetTimestamp();
                CompositionTarget.Rendering += OnRendering;
            }
            else
            {
                CompositionTarget.Rendering -= OnRendering;
            }
            UpdateFrame();
        }

        private void OnRendering(object sender, object e)
        {
            long now = Stopwatch.GetTimestamp();
            double ms = (now - _lastFrameQpc) * 1000.0 / Stopwatch.Frequency;
            _lastFrameQpc = now;
            if (ms > 0 && ms < 200 && _frameIntervalsMs.Count < 100000) _frameIntervalsMs.Add(ms);
            if (_frameIntervalsMs.Count % 30 == 0) UpdateFrame();
        }

        private void OnResetClick(object sender, RoutedEventArgs e)
        {
            _samples.Clear();
            _frameIntervalsMs.Clear();
            UpdateStroke(null);
            UpdateAggregate();
            UpdateFrame();
        }

        private void OnClearClick(object sender, RoutedEventArgs e) => InkSurface.InkPresenter.StrokeContainer.Clear();

        private void OnExportClick(object sender, RoutedEventArgs e)
        {
            try
            {
                string dir = App.DataFolder;
                Directory.CreateDirectory(dir);
                string file = Path.Combine(dir, $"InkPerf_{DateTime.Now:yyyyMMdd_HHmmss}.csv");
                File.WriteAllText(file, BuildCsv());
                LogText.Text = "CSV -> " + file + "\r\n" + LogText.Text;
                App.Log("Exported CSV: " + file);
            }
            catch (Exception ex) { LogText.Text = "CSV export failed: " + ex.Message + "\r\n" + LogText.Text; }
        }

        private void OnRefreshClick(object sender, RoutedEventArgs e)
        {
            UpdateMemory();
            RefreshCompositorPath();
        }

        // DIAGNOSTIC: attach/detach the toolbar from the canvas at runtime to isolate whether the
        // toolbar is what blocks inking. Detached = raw InkCanvas (default black pen, Mode=Inking).
        private void OnToolbarConnectToggled(object sender, RoutedEventArgs e)
        {
            bool on = ToolbarConnectToggle.IsOn;
            Toolbar.TargetInkCanvas = on ? InkSurface : null;
            ApplyInputDeviceTypes();
            App.Log("Toolbar connect = " + on);
            LogText.Text = "Toolbar " + (on ? "CONNECTED" : "DETACHED (raw canvas)") + "\r\n" + LogText.Text;
        }

        // DIAGNOSTIC: insert a stroke WITHOUT any input, to test whether the compositor path renders
        // ink at all (isolates rendering from input collection).
        private void OnAddTestStrokeClick(object sender, RoutedEventArgs e)
        {
            try
            {
                var builder = new Windows.UI.Input.Inking.InkStrokeBuilder();
                var pts = new List<Windows.Foundation.Point>();
                for (int i = 0; i <= 40; i++)
                    pts.Add(new Windows.Foundation.Point(40 + i * 10, 150 + 80 * Math.Sin(i / 3.0)));
                var stroke = builder.CreateStroke(pts);
                InkSurface.InkPresenter.StrokeContainer.AddStroke(stroke);
                int n = InkSurface.InkPresenter.StrokeContainer.GetStrokes().Count;
                App.Log("Added test stroke; container=" + n);
                LogText.Text = "Test stroke added (container=" + n + ") - if visible, rendering works\r\n" + LogText.Text;
            }
            catch (Exception ex)
            {
                App.Log("AddTestStroke failed: " + ex.Message);
                LogText.Text = "AddTestStroke failed: " + ex.Message + "\r\n" + LogText.Text;
            }
        }

        private string BuildCsv()
        {
            var sb = new StringBuilder();
            sb.AppendLine("index,points,durationMs,sampleRateHz,meanIntervalMs,minIntervalMs,maxIntervalMs,jitterMs");
            foreach (var s in _samples)
            {
                sb.AppendLine(string.Join(",", new[]
                {
                    s.Index.ToString(CultureInfo.InvariantCulture),
                    s.PointCount.ToString(CultureInfo.InvariantCulture),
                    s.DurationMs.ToString("F2", CultureInfo.InvariantCulture),
                    s.SampleRateHz.ToString("F1", CultureInfo.InvariantCulture),
                    s.MeanIntervalMs.ToString("F3", CultureInfo.InvariantCulture),
                    s.MinIntervalMs.ToString("F3", CultureInfo.InvariantCulture),
                    s.MaxIntervalMs.ToString("F3", CultureInfo.InvariantCulture),
                    s.JitterMs.ToString("F3", CultureInfo.InvariantCulture),
                }));
            }
            return sb.ToString();
        }

        // Reads the framework's compositor-decision log (%TEMP%\InkPerfCompositor.log) and shows the
        // last decision + the attach path that actually ran (proof of lifted vs system).
        private void RefreshCompositorPath()
        {
            try
            {
                string log = Path.Combine(Path.GetTempPath(), "InkPerfCompositor.log");
                if (!File.Exists(log)) { CompositorPathText.Text = "active path: (draw a stroke, then Refresh)"; return; }
                var lines = File.ReadAllLines(log);
                string decision = lines.LastOrDefault(l => l.Contains("IsSystemCompositor:")) ?? "";
                string attach = lines.LastOrDefault(l => l.Contains("Attach") && l.Contains(": ")) ?? "";
                CompositorPathText.Text = (decision + "\n" + attach).Trim();
            }
            catch { }
        }

        private void SelectCompositorMode(string mode)
        {
            foreach (var item in CompositorModeCombo.Items.OfType<ComboBoxItem>())
            {
                if ((item.Content as string) == mode) { CompositorModeCombo.SelectedItem = item; return; }
            }
        }

        private void OnCompositorModeChanged(object sender, SelectionChangedEventArgs e)
        {
            var mode = (CompositorModeCombo.SelectedItem as ComboBoxItem)?.Content as string;
            if (string.IsNullOrEmpty(mode)) return;
            App.WriteCompositorMode(mode);
            LogText.Text = $"compositor mode set to '{mode}' - click Restart to apply\r\n" + LogText.Text;
        }

        private void OnRestartClick(object sender, RoutedEventArgs e)
        {
            try
            {
                string exePath = Process.GetCurrentProcess().MainModule.FileName;
                Process.Start(new ProcessStartInfo { FileName = exePath, UseShellExecute = true });
                Application.Current.Exit();
            }
            catch (Exception ex) { LogText.Text = "restart failed: " + ex.Message + "\r\n" + LogText.Text; }
        }
    }
}
