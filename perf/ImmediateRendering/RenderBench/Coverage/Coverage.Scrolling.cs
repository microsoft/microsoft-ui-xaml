using Microsoft.UI;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Shapes;
using Windows.UI;

namespace RenderBench.Coverage;

// F9: scrolling, zoom and input-driven motion. Every page drives an animated (or zoomed) view change to a known end
// state; in-flight progress is asserted from the control's own intermediate notifications (the immediate walk must keep
// painting while DManip/InteractionTracker move the content) and the end state is compared with Composition.
public static class ScrollingCoveragePages
{
    private const string Family = "F9-scrolling";
    private static readonly TimeSpan Timeout = TimeSpan.FromSeconds(8);

    [CoveragePage(Family, "ScrollViewer-ChangeView-Animated")]
    public static CoveragePage ScrollViewerChangeViewAnimated(CoverageContext context)
    {
        var scrollViewer = CreateScrollViewer(zoom: false);
        return new CoveragePage
        {
            Root = Frame(scrollViewer),
            Exercise = async () =>
            {
                var progress = new OffsetProgress();
                await ChangeViewAndWait(scrollViewer, 420, 310, null, animate: true, progress);
                progress.AssertAnimated("ScrollViewer.ChangeView");
            },
        };
    }

    [CoveragePage(Family, "ScrollViewer-Zoom2x-Static")]
    public static CoveragePage ScrollViewerZoomStatic(CoverageContext context)
    {
        var scrollViewer = CreateScrollViewer(zoom: true);
        return new CoveragePage
        {
            Root = Frame(scrollViewer),
            Exercise = () => ChangeViewAndWait(scrollViewer, 150, 90, 2.0f, animate: false, progress: null),
        };
    }

    [CoveragePage(Family, "ScrollViewer-ZoomToFactor-Animated")]
    public static CoveragePage ScrollViewerZoomAnimated(CoverageContext context)
    {
        var scrollViewer = CreateScrollViewer(zoom: true);
        return new CoveragePage
        {
            Root = Frame(scrollViewer),
            Exercise = async () =>
            {
                var progress = new OffsetProgress();
                await ChangeViewAndWait(scrollViewer, 200, 120, 2.5f, animate: true, progress);
                progress.AssertAnimated("ScrollViewer.ChangeView(zoom)");
            },
        };
    }

    [CoveragePage(Family, "ScrollView-ScrollTo-Animated")]
    public static CoveragePage ScrollViewScrollToAnimated(CoverageContext context)
    {
        var scrollView = new ScrollView { Content = CreateChecker(), Width = 360, Height = 240 };
        return new CoveragePage
        {
            Root = Frame(scrollView),
            Exercise = async () =>
            {
                var progress = new OffsetProgress();
                await ScrollViewScrollAndWait(scrollView, progress, () =>
                    scrollView.ScrollTo(380, 290, new ScrollingScrollOptions(ScrollingAnimationMode.Enabled)));
                progress.AssertAnimated("ScrollView.ScrollTo");
            },
        };
    }

    [CoveragePage(Family, "ScrollView-ZoomTo-Animated")]
    public static CoveragePage ScrollViewZoomTo(CoverageContext context)
    {
        var scrollView = new ScrollView { Content = CreateChecker(), Width = 360, Height = 240, ZoomMode = ScrollingZoomMode.Enabled };
        return new CoveragePage
        {
            Root = Frame(scrollView),
            Exercise = async () =>
            {
                var progress = new OffsetProgress();
                await ScrollViewScrollAndWait(scrollView, progress, () =>
                    scrollView.ZoomTo(2.0f, new System.Numerics.Vector2(0, 0), new ScrollingZoomOptions(ScrollingAnimationMode.Enabled)));
                progress.AssertAnimated("ScrollView.ZoomTo");
            },
        };
    }

    // Mouse wheel over a ScrollViewer is handed to DManip (pure inertia input) in Composition.
    [CoveragePage(Family, "ScrollViewer-MouseWheel")]
    public static CoveragePage ScrollViewerMouseWheel(CoverageContext context)
    {
        var scrollViewer = CreateScrollViewer(zoom: false);
        return new CoveragePage
        {
            Root = Frame(scrollViewer),
            Exercise = async () =>
            {
                var progress = new OffsetProgress();
                var done = new TaskCompletionSource();
                EventHandler<ScrollViewerViewChangedEventArgs> handler = (_, e) =>
                {
                    progress.Record(scrollViewer.VerticalOffset);
                    if (!e.IsIntermediate) done.TrySetResult();
                };
                scrollViewer.ViewChanged += handler;
                try
                {
                    var center = scrollViewer.TransformToVisual(null).TransformPoint(new Windows.Foundation.Point(180, 120));
                    if (!InputInjection.Wheel(context.Window, center, -360))
                    {
                        return; // No input desktop (disconnected session): the page then compares the unscrolled view.
                    }
                    await done.Task.WaitAsync(Timeout);
                    await Task.Delay(500);
                }
                finally
                {
                    scrollViewer.ViewChanged -= handler;
                }
                if (scrollViewer.VerticalOffset <= 0)
                {
                    throw new InvalidOperationException("Mouse wheel did not scroll the ScrollViewer.");
                }
            },
        };
    }

    // Touch pan of a ScrollViewer: DManip drives the content transform, then inertia settles it.
    [CoveragePage(Family, "ScrollViewer-TouchPan")]
    public static CoveragePage ScrollViewerTouchPan(CoverageContext context)
    {
        var scrollViewer = CreateScrollViewer(zoom: false);
        return new CoveragePage
        {
            Root = Frame(scrollViewer),
            Exercise = async () =>
            {
                var done = new TaskCompletionSource();
                EventHandler<ScrollViewerViewChangedEventArgs> handler = (_, e) =>
                {
                    if (!e.IsIntermediate) done.TrySetResult();
                };
                scrollViewer.ViewChanged += handler;
                try
                {
                    var origin = scrollViewer.TransformToVisual(null).TransformPoint(new Windows.Foundation.Point(250, 200));
                    if (!await InputInjection.DragAsync(context.Window, origin, -150, -150, steps: 20))
                    {
                        return; // No input desktop (disconnected session): the page then compares the unscrolled view.
                    }
                    await done.Task.WaitAsync(Timeout);
                    await Task.Delay(500);
                }
                finally
                {
                    scrollViewer.ViewChanged -= handler;
                }
                if (scrollViewer.VerticalOffset <= 0 || scrollViewer.HorizontalOffset <= 0)
                {
                    throw new InvalidOperationException("Touch pan did not scroll the ScrollViewer.");
                }
            },
        };
    }

    [CoveragePage(Family, "ItemsView-Scrolled")]
    public static CoveragePage ItemsViewScrolled(CoverageContext context)
    {
        var items = Enumerable.Range(0, 200).Select(i => (object)Swatch(i, 340, 36)).ToList();
        var itemsView = new ItemsView
        {
            Width = 360,
            Height = 260,
            ItemsSource = items,
            ItemTemplate = (DataTemplate)Microsoft.UI.Xaml.Markup.XamlReader.Load(
                "<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>" +
                "<ItemContainer><ContentPresenter Content='{Binding}'/></ItemContainer></DataTemplate>"),
        };
        return new CoveragePage
        {
            Root = Frame(itemsView),
            Exercise = async () =>
            {
                var scrollView = itemsView.ScrollView ?? throw new InvalidOperationException("ItemsView has no ScrollView.");
                var progress = new OffsetProgress();
                await ScrollViewScrollAndWait(scrollView, progress, () =>
                    scrollView.ScrollTo(0, 1000, new ScrollingScrollOptions(ScrollingAnimationMode.Enabled)));
                progress.AssertAnimated("ItemsView.ScrollView.ScrollTo");
            },
        };
    }

    // The SwipeControl has no programmatic open API: the swipe is driven by injected touch (InputInjector). Where touch
    // injection is not permitted the page still validates the closed state with created swipe items.
    [CoveragePage(Family, "SwipeControl-Revealed", MaxMeanAbsDiff = 0.02)]
    public static CoveragePage SwipeControlRevealed(CoverageContext context)
    {
        var swipe = new SwipeControl
        {
            Width = 360,
            Height = 64,
            Content = new Border { Background = new SolidColorBrush(Color.FromArgb(255, 240, 240, 240)) },
            LeftItems = new SwipeItems { Mode = SwipeMode.Reveal },
        };
        swipe.LeftItems.Add(new SwipeItem { Text = "", Background = new SolidColorBrush(Colors.SteelBlue) });
        swipe.LeftItems.Add(new SwipeItem { Text = "", Background = new SolidColorBrush(Colors.IndianRed) });
        return new CoveragePage
        {
            Root = Frame(swipe),
            Exercise = async () =>
            {
                var origin = swipe.TransformToVisual(null).TransformPoint(new Windows.Foundation.Point(20, 32));
                await InputInjection.DragAsync(context.Window, origin, 200, 0);
                await Task.Delay(800);
            },
        };
    }

    // RefreshContainer.RequestRefresh drives the visualizer to the refreshing state (rotating progress glyph, translated
    // into view). The deferral is held while the page is captured; only the small glyph rotation is time dependent.
    [CoveragePage(Family, "RefreshContainer-Refreshing", MaxMeanAbsDiff = 0.02)]
    public static CoveragePage RefreshContainerRefreshing(CoverageContext context)
    {
        Windows.Foundation.Deferral? deferral = null;
        var list = new StackPanel();
        for (int i = 0; i < 12; i++)
        {
            list.Children.Add(Swatch(i, 340, 30));
        }
        var refresh = new RefreshContainer { Width = 360, Height = 260, Content = new ScrollViewer { Content = list } };
        var requested = new TaskCompletionSource();
        refresh.RefreshRequested += (_, e) =>
        {
            deferral = e.GetDeferral();
            requested.TrySetResult();
        };
        return new CoveragePage
        {
            Root = Frame(refresh),
            Exercise = async () =>
            {
                refresh.RequestRefresh();
                await requested.Task.WaitAsync(Timeout);
                await Task.Delay(400);
            },
            Detach = () => deferral?.Complete(),
        };
    }

    [CoveragePage(Family, "ParallaxView-MidScroll")]
    public static CoveragePage ParallaxViewMidScroll(CoverageContext context)
    {
        var list = new StackPanel();
        for (int i = 0; i < 30; i++)
        {
            list.Children.Add(Swatch(i, 300, 40));
        }
        var scrollViewer = new ScrollViewer { Content = list };
        var parallax = new ParallaxView
        {
            Source = scrollViewer,
            VerticalShift = 120,
            Child = CreateChecker(),
        };
        var root = new Grid { Width = 360, Height = 260 };
        root.Children.Add(parallax);
        root.Children.Add(scrollViewer);
        return new CoveragePage
        {
            Root = Frame(root),
            Exercise = async () =>
            {
                var progress = new OffsetProgress();
                await ChangeViewAndWait(scrollViewer, null, 500, null, animate: true, progress);
                progress.AssertAnimated("ParallaxView source ChangeView");
            },
        };
    }

    [CoveragePage(Family, "FlipView-Next")]
    public static CoveragePage FlipViewNext(CoverageContext context)
    {
        var flipView = new FlipView { Width = 360, Height = 220, UseTouchAnimationsForAllNavigation = true };
        for (int i = 0; i < 4; i++)
        {
            flipView.Items.Add(Swatch(i * 7 + 3, 360, 220));
        }
        return new CoveragePage
        {
            Root = Frame(flipView),
            Exercise = async () =>
            {
                flipView.SelectedIndex = 1;
                await Task.Delay(1000);
            },
        };
    }

    [CoveragePage(Family, "Pivot-HeaderScroll", MaxMeanAbsDiff = 0.02)]
    public static CoveragePage PivotHeaderScroll(CoverageContext context)
    {
        var pivot = new Pivot { Width = 360, Height = 240 };
        for (int i = 0; i < 6; i++)
        {
            pivot.Items.Add(new PivotItem { Header = $"Header {i}", Content = Swatch(i * 5 + 1, 320, 140) });
        }
        return new CoveragePage
        {
            Root = Frame(pivot),
            Exercise = async () =>
            {
                pivot.SelectedIndex = 3;
                await Task.Delay(1200);
            },
        };
    }

    [CoveragePage(Family, "TabView-HeaderScroll", MaxMeanAbsDiff = 0.02)]
    public static CoveragePage TabViewHeaderScroll(CoverageContext context)
    {
        var tabView = new TabView { Width = 360, Height = 200, IsAddTabButtonVisible = false };
        for (int i = 0; i < 16; i++)
        {
            tabView.TabItems.Add(new TabViewItem { Header = $"Tab {i}", IsClosable = false, Content = Swatch(i, 340, 120) });
        }
        return new CoveragePage
        {
            Root = Frame(tabView),
            Exercise = async () =>
            {
                tabView.SelectedIndex = 14;
                await Task.Delay(1000);
            },
        };
    }

    [CoveragePage(Family, "PipsPager-Scrolled")]
    public static CoveragePage PipsPagerScrolled(CoverageContext context)
    {
        var pager = new PipsPager { NumberOfPages = 30, MaxVisiblePips = 5 };
        return new CoveragePage
        {
            Root = Frame(pager),
            Exercise = async () =>
            {
                pager.SelectedPageIndex = 17;
                await Task.Delay(1000);
            },
        };
    }

    private static FrameworkElement Frame(FrameworkElement content) => new Grid
    {
        Background = new SolidColorBrush(Colors.White),
        Padding = new Thickness(12),
        HorizontalAlignment = HorizontalAlignment.Left,
        VerticalAlignment = VerticalAlignment.Top,
        Children = { content },
    };

    private static ScrollViewer CreateScrollViewer(bool zoom) => new()
    {
        Width = 360,
        Height = 240,
        HorizontalScrollBarVisibility = ScrollBarVisibility.Auto,
        HorizontalScrollMode = ScrollMode.Enabled,
        ZoomMode = zoom ? ZoomMode.Enabled : ZoomMode.Disabled,
        MaxZoomFactor = 4,
        Content = CreateChecker(),
    };

    // 1200x1200 grid of 100px cells with deterministic colors and a 1px darker frame per cell.
    private static FrameworkElement CreateChecker()
    {
        var canvas = new Canvas { Width = 1200, Height = 1200 };
        for (int row = 0; row < 12; row++)
        {
            for (int column = 0; column < 12; column++)
            {
                var cell = new Rectangle
                {
                    Width = 100,
                    Height = 100,
                    Fill = new SolidColorBrush(ColorFor(row * 12 + column)),
                    Stroke = new SolidColorBrush(Color.FromArgb(255, 40, 40, 40)),
                    StrokeThickness = 1,
                };
                Canvas.SetLeft(cell, column * 100);
                Canvas.SetTop(cell, row * 100);
                canvas.Children.Add(cell);
            }
        }
        return canvas;
    }

    private static Border Swatch(int index, double width, double height) => new()
    {
        Width = width,
        Height = height,
        Background = new SolidColorBrush(ColorFor(index)),
        BorderBrush = new SolidColorBrush(Color.FromArgb(255, 40, 40, 40)),
        BorderThickness = new Thickness(1),
    };

    private static Color ColorFor(int index) => Color.FromArgb(255,
        (byte)(60 + (index * 37) % 180), (byte)(60 + (index * 71) % 180), (byte)(60 + (index * 113) % 180));

    private sealed class OffsetProgress
    {
        private double? last;
        public int Intermediate { get; private set; }
        public bool Monotonic { get; private set; } = true;
        public int Direction { get; set; }

        public void Record(double value)
        {
            if (last is double previous && value != previous)
            {
                int direction = Math.Sign(value - previous);
                if (Direction == 0)
                {
                    Direction = direction;
                }
                else if (direction != Direction)
                {
                    Monotonic = false;
                }
            }
            last = value;
            Intermediate++;
        }

        public void AssertAnimated(string what)
        {
            // With OS animations off (common on VMs) every animated view change jumps in Composition too.
            if (!new Windows.UI.ViewManagement.UISettings().AnimationsEnabled)
            {
                return;
            }
            // An animated view change spans many frames; fewer than 3 intermediate values means it jumped.
            if (Intermediate < 3)
            {
                throw new InvalidOperationException($"{what}: only {Intermediate} intermediate view values.");
            }
            if (!Monotonic)
            {
                throw new InvalidOperationException($"{what}: intermediate view values are not monotonic.");
            }
        }
    }

    private static async Task ChangeViewAndWait(ScrollViewer scrollViewer, double? x, double? y, float? zoom, bool animate, OffsetProgress? progress)
    {
        var done = new TaskCompletionSource();
        EventHandler<ScrollViewerViewChangedEventArgs> handler = (_, e) =>
        {
            if (e.IsIntermediate)
            {
                progress?.Record(zoom is null ? scrollViewer.VerticalOffset + scrollViewer.HorizontalOffset : scrollViewer.ZoomFactor);
            }
            else
            {
                done.TrySetResult();
            }
        };
        scrollViewer.ViewChanged += handler;
        try
        {
            if (!scrollViewer.ChangeView(x, y, zoom, !animate))
            {
                throw new InvalidOperationException("ChangeView was rejected.");
            }
            await done.Task.WaitAsync(Timeout);
        }
        finally
        {
            scrollViewer.ViewChanged -= handler;
        }
    }

    private static async Task ScrollViewScrollAndWait(ScrollView scrollView, OffsetProgress progress, Func<int> start)
    {
        var done = new TaskCompletionSource();
        int correlationId = -1;
        Windows.Foundation.TypedEventHandler<ScrollView, object> viewChanged = (_, _) =>
            progress.Record(scrollView.VerticalOffset + scrollView.HorizontalOffset + scrollView.ZoomFactor);
        Windows.Foundation.TypedEventHandler<ScrollView, ScrollingScrollCompletedEventArgs> scrollCompleted = (_, e) =>
        {
            if (e.CorrelationId == correlationId) done.TrySetResult();
        };
        Windows.Foundation.TypedEventHandler<ScrollView, ScrollingZoomCompletedEventArgs> zoomCompleted = (_, e) =>
        {
            if (e.CorrelationId == correlationId) done.TrySetResult();
        };
        scrollView.ViewChanged += viewChanged;
        scrollView.ScrollCompleted += scrollCompleted;
        scrollView.ZoomCompleted += zoomCompleted;
        try
        {
            correlationId = start();
            await done.Task.WaitAsync(Timeout);
        }
        finally
        {
            scrollView.ViewChanged -= viewChanged;
            scrollView.ScrollCompleted -= scrollCompleted;
            scrollView.ZoomCompleted -= zoomCompleted;
        }
    }
}

// Synthesized input through user32 SendInput / InjectTouchInput (no hardware or package capability needed; the session
// must be able to receive input). Points are in window DIPs.
internal static class InputInjection
{
    private struct Point32 { public int X, Y; }

    // INPUT with the MOUSEINPUT union member (x64 layout: the union starts at offset 8, total size 40).
    [System.Runtime.InteropServices.StructLayout(System.Runtime.InteropServices.LayoutKind.Explicit, Size = 40)]
    private struct MouseInput
    {
        [System.Runtime.InteropServices.FieldOffset(0)] public uint Type;
        [System.Runtime.InteropServices.FieldOffset(8)] public int Dx;
        [System.Runtime.InteropServices.FieldOffset(12)] public int Dy;
        [System.Runtime.InteropServices.FieldOffset(16)] public uint MouseData;
        [System.Runtime.InteropServices.FieldOffset(20)] public uint Flags;
        [System.Runtime.InteropServices.FieldOffset(24)] public uint Time;
        [System.Runtime.InteropServices.FieldOffset(32)] public nint ExtraInfo;
    }

    [System.Runtime.InteropServices.StructLayout(System.Runtime.InteropServices.LayoutKind.Sequential)]
    private struct PointerTouchInfo
    {
        public uint PointerType, PointerId, FrameId, PointerFlags;
        public nint SourceDevice, HwndTarget;
        public Point32 PixelLocation, HimetricLocation, PixelLocationRaw, HimetricLocationRaw;
        public uint Time, HistoryCount;
        public int InputData;
        public uint KeyStates;
        public ulong PerformanceCount;
        public int ButtonChangeType;
        // POINTER_INFO is 8-byte aligned (96 bytes): touchFlags starts after 4 bytes of padding.
        public int Padding;
        public uint TouchFlags, TouchMask;
        public int ContactLeft, ContactTop, ContactRight, ContactBottom;
        public int RawLeft, RawTop, RawRight, RawBottom;
        public uint Orientation, Pressure;
    }

    [System.Runtime.InteropServices.DllImport("user32.dll")]
    private static extern bool ClientToScreen(nint hwnd, ref Point32 point);
    [System.Runtime.InteropServices.DllImport("user32.dll")]
    private static extern uint GetDpiForWindow(nint hwnd);
    [System.Runtime.InteropServices.DllImport("user32.dll")]
    private static extern bool SetCursorPos(int x, int y);
    [System.Runtime.InteropServices.DllImport("user32.dll")]
    private static extern uint SendInput(uint count, MouseInput[] inputs, int size);
    [System.Runtime.InteropServices.DllImport("user32.dll")]
    private static extern bool InitializeTouchInjection(uint maxCount, uint mode);
    [System.Runtime.InteropServices.DllImport("user32.dll")]
    private static extern bool InjectTouchInput(uint count, PointerTouchInfo[] contacts);

    private static Point32 ToScreen(Window window, Windows.Foundation.Point at)
    {
        nint hwnd = WinRT.Interop.WindowNative.GetWindowHandle(window);
        double scale = GetDpiForWindow(hwnd) / 96.0;
        var origin = new Point32();
        ClientToScreen(hwnd, ref origin);
        return new Point32 { X = origin.X + (int)Math.Round(at.X * scale), Y = origin.Y + (int)Math.Round(at.Y * scale) };
    }

    public static bool Wheel(Window window, Windows.Foundation.Point at, int delta)
    {
        var point = ToScreen(window, at);
        SetCursorPos(point.X, point.Y);
        var input = new MouseInput { Type = 0 /*INPUT_MOUSE*/, MouseData = unchecked((uint)delta), Flags = 0x0800 /*MOUSEEVENTF_WHEEL*/ };
        return SendInput(1, [input], System.Runtime.InteropServices.Marshal.SizeOf<MouseInput>()) == 1;
    }

    // INPUT with the KEYBDINPUT union member (x64: wVk at 8, dwFlags at 12, total size 40).
    [System.Runtime.InteropServices.StructLayout(System.Runtime.InteropServices.LayoutKind.Explicit, Size = 40)]
    private struct KeyInput
    {
        [System.Runtime.InteropServices.FieldOffset(0)] public uint Type;
        [System.Runtime.InteropServices.FieldOffset(8)] public ushort VirtualKey;
        [System.Runtime.InteropServices.FieldOffset(10)] public ushort Scan;
        [System.Runtime.InteropServices.FieldOffset(12)] public uint Flags;
        [System.Runtime.InteropServices.FieldOffset(16)] public uint Time;
        [System.Runtime.InteropServices.FieldOffset(24)] public nint ExtraInfo;
    }

    [System.Runtime.InteropServices.DllImport("user32.dll", EntryPoint = "SendInput")]
    private static extern uint SendKeyInput(uint count, KeyInput[] inputs, int size);

    // A key press (down and up) to the focused window; vk is a VK_* code (0x22 VK_NEXT, 0x23 VK_END, 0x24 VK_HOME).
    public static bool Key(ushort vk)
    {
        uint extended = vk is >= 0x21 and <= 0x28 ? 1u /*KEYEVENTF_EXTENDEDKEY*/ : 0u;
        var inputs = new[]
        {
            new KeyInput { Type = 1 /*INPUT_KEYBOARD*/, VirtualKey = vk, Flags = extended },
            new KeyInput { Type = 1, VirtualKey = vk, Flags = extended | 2 /*KEYEVENTF_KEYUP*/ },
        };
        return SendKeyInput(2, inputs, System.Runtime.InteropServices.Marshal.SizeOf<KeyInput>()) == 2;
    }

    private static PointerTouchInfo Contact(uint id, int x, int y, uint flags) => new()
    {
        PointerType = 2 /*PT_TOUCH*/,
        PointerId = id,
        PointerFlags = flags,
        PixelLocation = new Point32 { X = x, Y = y },
        TouchMask = 0x5 /*CONTACTAREA|PRESSURE*/,
        ContactLeft = x - 2, ContactTop = y - 2, ContactRight = x + 2, ContactBottom = y + 2,
        Pressure = 512,
    };

    // Injects frame 0..frames-1 on a background thread at exact 16 ms intervals (Task.Delay jitter changes the release
    // velocity, and so the inertia distance, from run to run). Screen points are computed on the caller's thread.
    private static Task<bool> InjectTimedAsync(int frames, Func<int, PointerTouchInfo[]> contacts)
    {
        var all = Enumerable.Range(0, frames).Select(contacts).ToArray();
        return Task.Run(() =>
        {
            var clock = System.Diagnostics.Stopwatch.StartNew();
            for (int i = 0; i < frames; i++)
            {
                double due = i * 16.0;
                while (clock.Elapsed.TotalMilliseconds < due - 2)
                {
                    Thread.Sleep(1);
                }
                while (clock.Elapsed.TotalMilliseconds < due)
                {
                    Thread.SpinWait(50);
                }
                if (!InjectTouchInput((uint)all[i].Length, all[i]))
                {
                    return false;
                }
            }
            return true;
        });
    }

    // Two-finger pinch centered on `center`: the contacts move apart horizontally from fromDistance to toDistance DIPs.
    public static Task<bool> PinchAsync(Window window, Windows.Foundation.Point center, double fromDistance, double toDistance, int steps = 15)
    {
        if (!InitializeTouchInjection(2, 3 /*TOUCH_FEEDBACK_NONE*/))
        {
            return Task.FromResult(false);
        }
        return InjectTimedAsync(steps + 2, i =>
        {
            double t = Math.Min(i, steps) / (double)steps;
            double half = (fromDistance + (toDistance - fromDistance) * t) / 2;
            var a = ToScreen(window, new Windows.Foundation.Point(center.X - half, center.Y));
            var b = ToScreen(window, new Windows.Foundation.Point(center.X + half, center.Y));
            uint flags = i == 0 ? 0x10006u : i <= steps ? 0x20006u : 0x40000u;
            return [Contact(0, a.X, a.Y, flags), Contact(1, b.X, b.Y, flags)];
        });
    }

    public static Task<bool> DragAsync(Window window, Windows.Foundation.Point start, double dx, double dy, int steps = 12)
    {
        if (!InitializeTouchInjection(1, 3 /*TOUCH_FEEDBACK_NONE*/))
        {
            return Task.FromResult(false);
        }
        var from = ToScreen(window, start);
        var to = ToScreen(window, new Windows.Foundation.Point(start.X + dx, start.Y + dy));
        return InjectTimedAsync(steps + 2, i =>
        {
            double t = Math.Min(i, steps) / (double)steps;
            int x = from.X + (int)Math.Round((to.X - from.X) * t);
            int y = from.Y + (int)Math.Round((to.Y - from.Y) * t);
            // DOWN|INRANGE|INCONTACT, then UPDATE|INRANGE|INCONTACT, then UP.
            return [Contact(0, x, y, i == 0 ? 0x10006u : i <= steps ? 0x20006u : 0x40000u)];
        });
    }
}
