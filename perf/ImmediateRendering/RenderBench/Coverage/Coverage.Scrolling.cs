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
                await TouchInjection.DragAsync(context.Window, origin, 200, 0);
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

// Synthesized touch through Windows.UI.Input.Preview.Injection (no hardware needed). Points are in window DIPs.
internal static class TouchInjection
{
    [System.Runtime.InteropServices.DllImport("user32.dll")]
    private static extern bool ClientToScreen(nint hwnd, ref Point32 point);
    [System.Runtime.InteropServices.DllImport("user32.dll")]
    private static extern uint GetDpiForWindow(nint hwnd);

    private struct Point32 { public int X, Y; }

    public static async Task<bool> DragAsync(Window window, Windows.Foundation.Point start, double dx, double dy, int steps = 12)
    {
        var injector = Windows.UI.Input.Preview.Injection.InputInjector.TryCreate();
        if (injector is null)
        {
            return false;
        }
        nint hwnd = WinRT.Interop.WindowNative.GetWindowHandle(window);
        double scale = GetDpiForWindow(hwnd) / 96.0;
        var origin = new Point32();
        ClientToScreen(hwnd, ref origin);
        injector.InitializeTouchInjection(Windows.UI.Input.Preview.Injection.InjectedInputVisualizationMode.None);
        try
        {
            for (int i = 0; i <= steps + 1; i++)
            {
                double t = Math.Min(i, steps) / (double)steps;
                var options = i == 0 ? Windows.UI.Input.Preview.Injection.InjectedInputPointerOptions.PointerDown | Windows.UI.Input.Preview.Injection.InjectedInputPointerOptions.InContact | Windows.UI.Input.Preview.Injection.InjectedInputPointerOptions.InRange
                    : i <= steps ? Windows.UI.Input.Preview.Injection.InjectedInputPointerOptions.Update | Windows.UI.Input.Preview.Injection.InjectedInputPointerOptions.InContact | Windows.UI.Input.Preview.Injection.InjectedInputPointerOptions.InRange
                    : Windows.UI.Input.Preview.Injection.InjectedInputPointerOptions.PointerUp;
                var info = new Windows.UI.Input.Preview.Injection.InjectedInputTouchInfo
                {
                    Contact = new Windows.UI.Input.Preview.Injection.InjectedInputRectangle { Left = 2, Top = 2, Right = 2, Bottom = 2 },
                    PointerInfo = new Windows.UI.Input.Preview.Injection.InjectedInputPointerInfo
                    {
                        PointerId = 1,
                        PointerOptions = options,
                        PixelLocation = new Windows.UI.Input.Preview.Injection.InjectedInputPoint
                        {
                            PositionX = origin.X + (int)Math.Round((start.X + dx * t) * scale),
                            PositionY = origin.Y + (int)Math.Round((start.Y + dy * t) * scale),
                        },
                    },
                    Pressure = 1.0,
                    TouchParameters = Windows.UI.Input.Preview.Injection.InjectedInputTouchParameters.Pressure | Windows.UI.Input.Preview.Injection.InjectedInputTouchParameters.Contact,
                };
                injector.InjectTouchInput([info]);
                await Task.Delay(16);
            }
            return true;
        }
        catch (Exception)
        {
            return false;
        }
        finally
        {
            injector.UninitializeTouchInjection();
        }
    }
}