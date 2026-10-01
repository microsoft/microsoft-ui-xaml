using System;
using System.Runtime.InteropServices;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.UI.Dispatching;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Charts;
using Microsoft.UI.Xaml.Hosting;
using Microsoft.UI.Xaml.Markup;
using Windows.Graphics;

namespace ChartsSample
{
    internal sealed class SecondaryChartWindow
    {
        private readonly object _gate = new();
        private readonly Thread _thread;
        private readonly TaskCompletionSource<string> _ready = new(TaskCreationOptions.RunContinuationsAsynchronously);
        private readonly TaskCompletionSource<bool> _stopped = new(TaskCreationOptions.RunContinuationsAsynchronously);
        private DispatcherQueue _dispatcher;
        private bool _stopRequested;
        private string _stage = "starting the secondary UI thread";

        internal SecondaryChartWindow()
        {
            _thread = new Thread(Run) { Name = "Secondary chart UI" };
            _thread.SetApartmentState(ApartmentState.STA);
            _thread.Start();
        }

        internal Task<string> Ready => _ready.Task;
        internal bool IsRunning => _thread.IsAlive;

        internal async Task StopAsync()
        {
            lock (_gate)
            {
                if (!_stopRequested)
                {
                    _stopRequested = true;
                    // Only the agile dispatcher crosses threads; UI objects stay in RunWindow.
                    _dispatcher?.TryEnqueue(() => DispatcherQueue.GetForCurrentThread().EnqueueEventLoopExit());
                }
            }
            await _stopped.Task;
            await Task.Run(() => _thread.Join());
        }

        private void Run()
        {
            bool apartmentInitialized = false;
            try
            {
                _stage = "initializing the secondary apartment";
                Marshal.ThrowExceptionForHR(RoInitialize(0));
                apartmentInitialized = true;
                RunWindow();
            }
            catch (COMException ex)
            {
                _ready.TrySetResult($"The secondary chart window failed while {_stage} (HRESULT 0x{ex.HResult:X8}): {ex.Message}");
            }
            catch (InvalidOperationException ex)
            {
                _ready.TrySetResult($"The secondary chart window failed while {_stage}: {ex.Message}");
            }
            finally
            {
                lock (_gate) _dispatcher = null;
                if (apartmentInitialized) RoUninitialize();
                _ready.TrySetResult("The secondary window closed before it was ready.");
                _stopped.TrySetResult(true);
            }
        }

        private void RunWindow()
        {
            var controller = DispatcherQueueController.CreateOnCurrentThread();
            try
            {
                var dispatcher = controller.DispatcherQueue;
                _stage = "starting XAML on the secondary thread";
                using var xaml = WindowsXamlManager.InitializeForCurrentThread();
                var values = new ObservableVector<double> { 14, 28, 19, 43, 31 };
                var samples = new Samples { ItemsSource = values };
                var line = new LineSeries { Title = "Secondary UI thread", YValues = samples, StrokeThickness = 3, ShowDataMarkers = true };
                var chart = new Chart { Height = 280, ShowLegend = true };
                chart.Data.Add(samples);
                chart.Series.Add(line);
                AutomationProperties.SetName(chart, "Independently updating secondary line chart");
                AutomationProperties.SetHelpText(chart, "Five values update independently of the primary window. Current values are listed below.");

                var data = new TextBlock { TextWrapping = TextWrapping.Wrap, IsTextSelectionEnabled = true };
                void UpdateText() => data.Text = $"Current values: {string.Join(", ", values)}";
                UpdateText();
                var content = new StackPanel { Spacing = 12, Padding = new Thickness(16) };
                content.Children.Add(new TextBlock
                {
                    Text = "This window owns a separate UI thread, chart, data collection and timer.",
                    TextWrapping = TextWrapping.Wrap
                });
                content.Children.Add(chart);
                content.Children.Add(data);
                var root = (Grid)XamlReader.Load(
                    "<Grid xmlns=\"http://schemas.microsoft.com/winfx/2006/xaml/presentation\" Background=\"{ThemeResource ApplicationPageBackgroundThemeBrush}\" />");
                root.Children.Add(new ScrollViewer { Content = content, HorizontalScrollBarVisibility = ScrollBarVisibility.Disabled });
                var window = new Window { Title = "Charts secondary UI thread", Content = root };
                window.AppWindow.Resize(new SizeInt32(620, 500));
                bool closed = false;
                void OnClosed(object sender, WindowEventArgs args)
                {
                    closed = true;
                    dispatcher.EnqueueEventLoopExit();
                }
                window.Closed += OnClosed;
                var timer = new DispatcherTimer { Interval = TimeSpan.FromMilliseconds(120) };
                int index = 0;
                void OnTick(object sender, object args)
                {
                    double value = values[index];
                    values[index] = value >= 48 ? value - 31 : value + 9;
                    index = (index + 1) % values.Count;
                    UpdateText();
                }
                timer.Tick += OnTick;
                try
                {
                    _stage = "activating the secondary window";
                    window.Activate();
                    timer.Start();
                    lock (_gate)
                    {
                        _dispatcher = dispatcher;
                        if (_stopRequested) dispatcher.EnqueueEventLoopExit();
                    }
                    dispatcher.TryEnqueue(() => _ready.TrySetResult(null));
                    _stage = "running the secondary window";
                    dispatcher.RunEventLoop();
                }
                finally
                {
                    lock (_gate) _dispatcher = null;
                    timer.Stop();
                    timer.Tick -= OnTick;
                    window.Closed -= OnClosed;
                    if (!closed) window.Close();
                }
            }
            finally
            {
                _stage = "shutting down the secondary window";
                controller.ShutdownQueue();
            }
        }

        [DllImport("combase.dll", ExactSpelling = true)]
        private static extern int RoInitialize(uint initializationType);

        [DllImport("combase.dll", ExactSpelling = true)]
        private static extern void RoUninitialize();
    }
}
