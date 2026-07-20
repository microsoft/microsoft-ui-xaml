using System;
using System.ComponentModel;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Threading.Tasks;
using Microsoft.UI;
using Microsoft.UI.Dispatching;
using Microsoft.UI.Windowing;
using Microsoft.UI.Xaml;
using Windows.ApplicationModel.DataTransfer;
using Windows.Graphics;
using WinRT.Interop;
using OSFrameworkLens.Detection;

namespace OSFrameworkLens.UI;

public partial class InspectorWindow : Window
{
    private const int SnapshotTickMs = 33;
    private const int HotkeyId = 0xBEEF;
    private const nuint SubclassId = 1;
    private const int InspectorWidth = 460;
    private const int InspectorHeight = 720;
    private const int InspectorMargin = 20;

    private readonly InspectorViewModel _vm = new();
    private readonly SnapshotService _service = new();
    private readonly DispatcherQueueTimer _timer;
    private readonly HighlightOverlay _overlay = new();

    private readonly IntPtr _thisHwnd;
    private IntPtr _overlayHwnd;
    // Keeps the GC from collecting the subclass proc while Win32 holds its pointer.
    private readonly Native.SUBCLASSPROC _subclassProc;
    // Single-flight gate: a slow snapshot (target up to ~100 ms in SendMessageTimeout)
    // drains on a worker while the 30 Hz timer keeps firing — see DESIGN.md §D5.
    private Task _pendingSnapshot = Task.CompletedTask;

    internal InspectorViewModel ViewModel => _vm;

    public InspectorWindow()
    {
        InitializeComponent();
        Title = "OS Framework Lens";

        _thisHwnd = WindowNative.GetWindowHandle(this);
        ConfigureAppWindow();

        _overlay.Show();
        _overlayHwnd = _overlay.Hwnd;

        var queue = DispatcherQueue.GetForCurrentThread();
        _timer = queue.CreateTimer();
        _timer.Interval = TimeSpan.FromMilliseconds(SnapshotTickMs);
        _timer.Tick += (_, _) => OnTick();
        _timer.Start();

        _subclassProc = SubclassProc;
        Native.SetWindowSubclass(_thisHwnd, _subclassProc, SubclassId, 0);
        Native.RegisterHotKey(_thisHwnd, HotkeyId,
            Native.MOD_CONTROL | Native.MOD_SHIFT | Native.MOD_NOREPEAT, Native.VK_F);

        Closed += OnClosedHandler;
    }

    private void ConfigureAppWindow()
    {
        var windowId = Win32Interop.GetWindowIdFromWindow(_thisHwnd);
        var appWindow = AppWindow.GetFromWindowId(windowId);
        if (appWindow is null) return;

        var area = DisplayArea.GetFromWindowId(windowId, DisplayAreaFallback.Primary);
        appWindow.MoveAndResize(new RectInt32(
            area.WorkArea.X + area.WorkArea.Width - InspectorWidth - InspectorMargin,
            area.WorkArea.Y + InspectorMargin,
            InspectorWidth, InspectorHeight));

        if (appWindow.Presenter is OverlappedPresenter op)
        {
            op.IsAlwaysOnTop = true;
            op.IsMaximizable = false;
        }
    }

    private void OnTick()
    {
        if (_vm.IsFrozen) return;
        if (!_pendingSnapshot.IsCompleted) return;
        if (!Native.GetCursorPos(out var pt)) return;

        var top = Native.WindowFromPoint(pt);
        if (top == IntPtr.Zero || top == _thisHwnd || top == _overlayHwnd) return;
        var root = Native.GetAncestor(top, Native.GA_ROOT);
        if (root == _thisHwnd || root == _overlayHwnd) return;

        _pendingSnapshot = ProcessSnapshotAsync(pt.X, pt.Y);
    }

    private async Task ProcessSnapshotAsync(int screenX, int screenY)
    {
        ElementSnapshot? snap;
        try
        {
            snap = await Task.Run(() => _service.Snapshot(screenX, screenY));
        }
        catch (Exception ex) when (IsTransient(ex))
        {
            Debug.WriteLine($"InspectorWindow.OnTick: transient snapshot failure: {ex.GetType().Name}: {ex.Message}");
            snap = null;
        }

        _vm.Apply(snap);

        if (snap is null) { _overlay.HideRect(); return; }
        _overlay.ShowRect(snap.Bounds.Left, snap.Bounds.Top,
                          snap.Bounds.Right, snap.Bounds.Bottom,
                          snap.FrameworkColorHex);
    }

    private IntPtr SubclassProc(IntPtr hWnd, uint uMsg, IntPtr wParam, IntPtr lParam,
                                nuint uIdSubclass, nuint dwRefData)
    {
        if (uMsg == Native.WM_HOTKEY && wParam.ToInt32() == HotkeyId)
        {
            DispatcherQueue.GetForCurrentThread()?.TryEnqueue(ToggleFreeze);
            return IntPtr.Zero;
        }
        return Native.DefSubclassProc(hWnd, uMsg, wParam, lParam);
    }

    private void OnFreezeClicked(object sender, RoutedEventArgs e) => ToggleFreeze();
    private void ToggleFreeze() => _vm.IsFrozen = !_vm.IsFrozen;

    private void OnCopyClicked(object sender, RoutedEventArgs e)
    {
        var report = _vm.BuildClipboardReport();
        if (string.IsNullOrEmpty(report)) return;
        try
        {
            var pkg = new DataPackage();
            pkg.SetText(report);
            Clipboard.SetContent(pkg);
            _vm.StatusText = "Report copied to clipboard.";
        }
        catch (COMException ex)
        {
            Debug.WriteLine($"Clipboard.SetContent failed: {ex.Message}");
            _vm.StatusText = "Clipboard busy — try again.";
        }
    }

    private async void OnAnalyzeClicked(object sender, RoutedEventArgs e)
    {
        var target = _vm.LastSnapshot?.RootHwnd ?? IntPtr.Zero;
        if (target == IntPtr.Zero)
        {
            if (!Native.GetCursorPos(out var pt)) { _vm.StatusText = "Hover over a window first, then click Analyze."; return; }
            var leaf = Native.WindowFromPoint(pt);
            if (leaf == IntPtr.Zero) { _vm.StatusText = "No window under cursor."; return; }
            target = Native.GetAncestor(leaf, Native.GA_ROOT);
        }

        var previousStatus = _vm.StatusText;
        _vm.StatusText = "Analyzing…";
        try
        {
            var analysis = await Task.Run(() => _service.AnalyzeWindow(target));
            new WindowAnalysisWindow(analysis).Activate();
            _vm.StatusText = previousStatus;
        }
        catch (Exception ex) when (IsTransient(ex))
        {
            Debug.WriteLine($"Analyze failed: {ex}");
            _vm.StatusText = $"Analyze failed: {ex.Message}";
        }
    }

    // Allow-list of transient inspection failures — bugs in our own code still crash visibly.
    private static bool IsTransient(Exception ex) =>
        ex is Win32Exception or COMException or UnauthorizedAccessException
           or InvalidOperationException or TimeoutException;

    private async void OnClosedHandler(object sender, WindowEventArgs args)
    {
        _timer.Stop();
        try { await _pendingSnapshot; }
        catch (Exception ex) when (IsTransient(ex))
        {
            Debug.WriteLine($"Pending snapshot drained with transient error: {ex.Message}");
        }
        try
        {
            if (_thisHwnd != IntPtr.Zero)
            {
                Native.UnregisterHotKey(_thisHwnd, HotkeyId);
                Native.RemoveWindowSubclass(_thisHwnd, _subclassProc, SubclassId);
            }
        }
        catch (Win32Exception ex)
        {
            Debug.WriteLine($"Cleanup failed: {ex.Message}");
        }
        _overlay.Close();
        _service.Dispose();
    }
}
