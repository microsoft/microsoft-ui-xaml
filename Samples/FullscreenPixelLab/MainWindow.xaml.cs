using System;
using System.ComponentModel;
using System.IO;
using Microsoft.UI.Windowing;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Input;
using Windows.Graphics;

namespace FullscreenPixelLab;

public sealed partial class MainWindow : Window
{
    private readonly IntPtr _hwnd;
    private readonly DispatcherTimer _timer = new() { Interval = TimeSpan.FromMilliseconds(250) };
    private bool _ready;

    public MainWindow()
    {
        InitializeComponent();
        _hwnd = WinRT.Interop.WindowNative.GetWindowHandle(this);
        ExtendsContentIntoTitleBar = true;
        SetTitleBar(DragRegion);

        var scale = WindowGeometry.GetScale(_hwnd);
        var workArea = DisplayArea.GetFromWindowId(AppWindow.Id, DisplayAreaFallback.Primary).WorkArea;
        var width = Math.Min((int)(960 * scale), workArea.Width);
        var height = Math.Min((int)(760 * scale), workArea.Height);
        AppWindow.MoveAndResize(new RectInt32(
            workArea.X + (workArea.Width - width) / 2,
            workArea.Y + (workArea.Height - height) / 2, width, height));
        var labelPath = Path.Combine(AppContext.BaseDirectory, "runtime-label.txt");
        RuntimeText.Text = File.Exists(labelPath) ? File.ReadAllText(labelPath).Trim() : "Local WinUI build";

        _timer.Tick += (_, _) => RefreshMeasurement();
        Closed += (_, _) => _timer.Stop();
        _ready = true;
    }

    private bool IsFullscreen => AppWindow.Presenter.Kind == AppWindowPresenterKind.FullScreen;

    private void OnLoaded(object sender, RoutedEventArgs e)
    {
        RefreshMeasurement();
        _timer.Start();
    }

    private void OnEnterFullscreen(object sender, RoutedEventArgs e) => SetFullscreen(true);
    private void OnExitFullscreen(object sender, RoutedEventArgs e) => SetFullscreen(false);

    private void SetFullscreen(bool fullscreen)
    {
        AppWindow.SetPresenter(fullscreen ? AppWindowPresenterKind.FullScreen : AppWindowPresenterKind.Overlapped);
        RefreshMeasurement();
    }

    private void OnExtendToggled(object sender, RoutedEventArgs e)
    {
        if (!_ready)
        {
            return;
        }

        ExtendsContentIntoTitleBar = ExtendToggle.IsOn;
        SetTitleBar(ExtendToggle.IsOn ? DragRegion : null);
        RefreshMeasurement();
    }

    private void OnFullscreenKey(KeyboardAccelerator sender, KeyboardAcceleratorInvokedEventArgs args)
    {
        SetFullscreen(!IsFullscreen);
        args.Handled = true;
    }

    private void OnEscapeKey(KeyboardAccelerator sender, KeyboardAcceleratorInvokedEventArgs args)
    {
        if (IsFullscreen)
        {
            SetFullscreen(false);
            args.Handled = true;
        }
    }

    private void RefreshMeasurement()
    {
        if (!_ready)
        {
            return;
        }

        EnterButton.IsEnabled = !IsFullscreen;
        ExitButton.IsEnabled = IsFullscreen;
        StateText.Text = $"Presenter: {AppWindow.Presenter.Kind} | Window ECITB: {ExtendsContentIntoTitleBar}";

        try
        {
            var geometry = WindowGeometry.Read(_hwnd);
            var expected = !IsFullscreen && ExtendsContentIntoTitleBar && !geometry.Maximized ? 1 : 0;
            OffsetText.Text = $"Actual top offset: {geometry.TopOffset} physical px";
            ResultText.Text = geometry.TopOffset == expected
                ? $"PASS - expected {expected} physical px"
                : $"FAIL - expected {expected} physical px; uncovered top row";
            GeometryText.Text = $"Client: {geometry.ClientWidth} x {geometry.ClientHeight} px\n"
                + $"Bridge: {geometry.BridgeWidth} x {geometry.BridgeHeight} px\n"
                + $"DPI: {geometry.Dpi} | Style: 0x{geometry.Style:X8} | ExStyle: 0x{geometry.ExStyle:X8}";
            MeasurementError.IsOpen = false;
        }
        catch (Win32Exception error)
        {
            ShowMeasurementError(error.Message);
        }
        catch (InvalidOperationException error)
        {
            ShowMeasurementError(error.Message);
        }
    }

    private void ShowMeasurementError(string message)
    {
        OffsetText.Text = "Top offset unavailable";
        ResultText.Text = "ERROR - no measurement";
        GeometryText.Text = string.Empty;
        MeasurementError.Message = message;
        MeasurementError.IsOpen = true;
    }
}
