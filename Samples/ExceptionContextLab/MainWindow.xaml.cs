using System;
using System.ComponentModel;
using System.IO;
using System.Runtime.InteropServices;
using System.Threading.Tasks;
using Microsoft.UI.Windowing;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Controls;
using Windows.Graphics;

namespace ExceptionContextLab;

public sealed partial class MainWindow : Window
{
    private readonly SessionLog _log;
    private readonly DispatcherTimer _timer = new() { Interval = TimeSpan.FromMilliseconds(500) };
    private bool _expanderLoaded;
    private bool _awaitingRecovery;
    private bool _lastHandled;
    private int _events;
    private int _ticks;
    private int _pings;

    internal bool MarkHandled => HandledBox.IsChecked == true;

    internal MainWindow(SessionLog log)
    {
        _log = log;
        InitializeComponent();

        var dpi = GetDpiForWindow(WinRT.Interop.WindowNative.GetWindowHandle(this));
        if (dpi == 0)
        {
            throw new Win32Exception(Marshal.GetLastWin32Error(), "GetDpiForWindow failed.");
        }
        var work = DisplayArea.GetFromWindowId(AppWindow.Id, DisplayAreaFallback.Primary).WorkArea;
        var width = Math.Min((int)(920 * dpi / 96.0), work.Width);
        var height = Math.Min((int)(700 * dpi / 96.0), work.Height);
        AppWindow.MoveAndResize(new RectInt32(work.X + (work.Width - width) / 2,
            work.Y + (work.Height - height) / 2, width, height));
        var labelPath = Path.Combine(AppContext.BaseDirectory, "runtime-label.txt");
        RuntimeText.Text = File.Exists(labelPath) ? File.ReadAllText(labelPath).Trim() : "Local WinUI build";

        _timer.Tick += (_, _) =>
        {
            HeartbeatText.Text = $"PID: {Environment.ProcessId} | UI heartbeat: {++_ticks}";
            if (_awaitingRecovery)
            {
                _awaitingRecovery = false;
                OutcomeText.Text = $"UI responsive after test exception (handled={_lastHandled.ToString().ToLowerInvariant()}).";
                _log.Write($"UI_ALIVE handled={_lastHandled}");
                ThrowButton.IsEnabled = true;
                HandledBox.IsEnabled = true;
                LoadButton.IsEnabled = !_expanderLoaded;
            }
        };
        Closed += (_, _) =>
        {
            _timer.Stop();
            _log.Write("WINDOW_CLOSED");
        };
        RefreshCase();
    }

    private void OnLoaded(object sender, RoutedEventArgs args)
    {
        _timer.Start();
        _log.Write("APP_READY");
    }

    private void OnLoadExpander(object sender, RoutedEventArgs args)
    {
        LoadButton.IsEnabled = false;
        ThrowButton.IsEnabled = false;
        var expander = new Expander
        {
            Header = "A real Expander, created only for this case",
            Content = "Its template is now loaded and expanded.",
            IsExpanded = true,
            HorizontalAlignment = HorizontalAlignment.Stretch,
            HorizontalContentAlignment = HorizontalAlignment.Stretch
        };
        AutomationProperties.SetAutomationId(expander, "TestExpander");
        expander.Loaded += (_, _) =>
        {
            _expanderLoaded = true;
            ThrowButton.IsEnabled = true;
            RefreshCase();
            _log.Write("EXPANDER_LOADED");
        };
        ExpanderHost.Content = expander;
    }

    private void OnHandledChanged(object sender, RoutedEventArgs args) => RefreshCase();

    private void RefreshCase() => CaseText.Text =
        $"Expander: {(_expanderLoaded ? "loaded" : "not loaded")} | e.Handled: {MarkHandled.ToString().ToLowerInvariant()}";

    private async void OnThrow(object sender, RoutedEventArgs args)
    {
        ThrowButton.IsEnabled = false;
        LoadButton.IsEnabled = false;
        HandledBox.IsEnabled = false;
        OutcomeText.Text = "Waiting for the intentional async exception...";
        _log.Write($"THROW expander={_expanderLoaded} handled={MarkHandled}");
        await Task.Delay(250);
        throw new InvalidOperationException(App.TestExceptionMessage);
    }

    internal void ObserveTestException(bool handled)
    {
        ExceptionText.Text = $"UnhandledException calls: {++_events} | e.Handled: {handled.ToString().ToLowerInvariant()}";
        _lastHandled = handled;
        _awaitingRecovery = true;
    }

    private void OnCheckUi(object sender, RoutedEventArgs args)
    {
        PingText.Text = $"UI checks: {++_pings}";
        _log.Write($"UI_CHECK count={_pings}");
    }

    [DllImport("user32.dll", SetLastError = true)]
    private static extern uint GetDpiForWindow(IntPtr hwnd);
}
