using System;
using System.ComponentModel;
using System.Runtime.InteropServices;
using Microsoft.UI.Input;
using Microsoft.UI.Windowing;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Shapes;
using Windows.Foundation;
using Windows.Graphics;

namespace NonRectangularWindow
{
    public sealed class MainWindow : Window
    {
        private const double HoleRadius = 44;
        private const double HoleRightInset = 90;
        private const double HoleTop = 130;
        private readonly IntPtr _hwnd;
        private readonly InputNonClientPointerSource _nonClientPointerSource;
        private readonly Grid _root = new Grid();
        private readonly Path _surface = new Path();
        private readonly Border _dragHandle;
        private readonly CheckBox _useRegion = new CheckBox { Content = "Use Win32 input region", IsChecked = true };
        private (int Width, int Height, int OffsetX, int OffsetY, uint Dpi, bool UseRegion,
            double DragWidth, double DragHeight)? _lastShape;
        private bool _closed;

        public MainWindow() : base(new WindowOptions { SupportsTransparency = true })
        {
            Title = "Non-rectangular WinUI prototype";
            _hwnd = WinRT.Interop.WindowNative.GetWindowHandle(this);
            var presenter = (OverlappedPresenter)AppWindow.Presenter;
            presenter.SetBorderAndTitleBar(false, false);
            presenter.IsResizable = false;
            presenter.IsMaximizable = false;
            _nonClientPointerSource = InputNonClientPointerSource.GetForWindowId(AppWindow.Id);

            _surface.Fill = (Brush)Application.Current.Resources["SolidBackgroundFillColorBaseBrush"];
            _root.Children.Add(_surface);

            var title = new TextBlock
            {
                Text = "Non-rectangular WinUI",
                FontSize = 24,
                FontWeight = Microsoft.UI.Text.FontWeights.SemiBold
            };
            _dragHandle = new Border
            {
                Child = title,
                Background = new SolidColorBrush(Microsoft.UI.Colors.Transparent),
                Padding = new Thickness(0, 8, 0, 8)
            };
            _dragHandle.SizeChanged += (_, _) => UpdateShape();

            var status = new TextBlock { Text = "XAML button clicks: 0" };
            int clicks = 0;
            var count = new Button { Content = "Test XAML input" };
            count.Click += (_, _) => status.Text = $"XAML button clicks: {++clicks}";
            var close = new Button { Content = "Close both windows" };
            close.Click += (_, _) => Close();

            var panel = new StackPanel
            {
                Width = 300,
                Spacing = 12,
                Margin = new Thickness(135, 65, 0, 0),
                HorizontalAlignment = HorizontalAlignment.Left,
                VerticalAlignment = VerticalAlignment.Top
            };
            panel.Children.Add(_dragHandle);
            panel.Children.Add(new TextBlock
            {
                Text = "Drag the heading. Click the hole on the right to reach the blue window behind.",
                TextWrapping = TextWrapping.Wrap
            });
            panel.Children.Add(_useRegion);
            panel.Children.Add(count);
            panel.Children.Add(status);
            panel.Children.Add(close);
            _root.Children.Add(panel);
            Content = _root;

            _root.SizeChanged += (_, _) => UpdateShape();
            _root.Loaded += (_, _) =>
            {
                _root.XamlRoot.Changed += OnXamlRootChanged;
                UpdateShape();
            };
            _useRegion.Checked += (_, _) => UpdateShape();
            _useRegion.Unchecked += (_, _) => UpdateShape();
            Closed += (_, _) =>
            {
                _closed = true;
                if (_root.XamlRoot != null)
                {
                    _root.XamlRoot.Changed -= OnXamlRootChanged;
                }
            };

            double scale = NativeMethods.GetDpiForWindow(_hwnd) / 96.0;
            AppWindow.MoveAndResize(new RectInt32(180, 180, (int)(640 * scale), (int)(440 * scale)));
            UpdateShape();
        }

        private void OnXamlRootChanged(XamlRoot sender, XamlRootChangedEventArgs args) => UpdateShape();

        private void UpdateShape()
        {
            if (_closed)
            {
                return;
            }

            var clientOrigin = new NativeMethods.Point();
            if (!NativeMethods.GetWindowRect(_hwnd, out var bounds) ||
                !NativeMethods.GetClientRect(_hwnd, out var clientBounds) ||
                !NativeMethods.ClientToScreen(_hwnd, ref clientOrigin))
            {
                throw new Win32Exception(Marshal.GetLastWin32Error());
            }
            uint dpi = NativeMethods.GetDpiForWindow(_hwnd);
            double scale = dpi / 96.0;
            int width = clientBounds.Right - clientBounds.Left;
            int height = clientBounds.Bottom - clientBounds.Top;
            int offsetX = clientOrigin.X - bounds.Left;
            int offsetY = clientOrigin.Y - bounds.Top;
            if (width <= 0 || height <= 0)
            {
                return;
            }
            var shape = (width, height, offsetX, offsetY, dpi, _useRegion.IsChecked == true,
                _dragHandle.ActualWidth, _dragHandle.ActualHeight);
            if (_lastShape == shape)
            {
                return;
            }
            // SetWindowRgn sends window-position messages, which can reenter this handler.
            _lastShape = shape;
            if (_dragHandle.ActualWidth > 0 && _dragHandle.ActualHeight > 0)
            {
                Rect dragBounds = _dragHandle.TransformToVisual(_root).TransformBounds(
                    new Rect(0, 0, _dragHandle.ActualWidth, _dragHandle.ActualHeight));
                _nonClientPointerSource.SetRegionRects(NonClientRegionKind.Caption, new[]
                {
                    new RectInt32(
                        (int)Math.Round(dragBounds.X * scale),
                        (int)Math.Round(dragBounds.Y * scale),
                        (int)Math.Round(dragBounds.Width * scale),
                        (int)Math.Round(dragBounds.Height * scale))
                });
            }
            double logicalWidth = width / scale;
            double logicalHeight = height / scale;
            var outline = new GeometryGroup { FillRule = FillRule.EvenOdd };
            outline.Children.Add(new EllipseGeometry
            {
                Center = new Point(logicalWidth / 2, logicalHeight / 2),
                RadiusX = logicalWidth / 2,
                RadiusY = logicalHeight / 2
            });
            outline.Children.Add(new EllipseGeometry
            {
                Center = new Point(logicalWidth - HoleRightInset, HoleTop),
                RadiusX = HoleRadius,
                RadiusY = HoleRadius
            });
            _surface.Data = outline;

            if (_useRegion.IsChecked != true)
            {
                if (NativeMethods.SetWindowRgn(_hwnd, IntPtr.Zero, true) == 0)
                {
                    throw new InvalidOperationException("SetWindowRgn could not remove the window region.");
                }
                return;
            }

            // XAML uses the client origin; SetWindowRgn uses the outer window origin.
            IntPtr region = NativeMethods.CreateEllipticRgn(offsetX, offsetY, offsetX + width, offsetY + height);
            IntPtr hole = NativeMethods.CreateEllipticRgn(
                offsetX + (int)Math.Round((logicalWidth - HoleRightInset - HoleRadius) * scale),
                offsetY + (int)Math.Round((HoleTop - HoleRadius) * scale),
                offsetX + (int)Math.Round((logicalWidth - HoleRightInset + HoleRadius) * scale),
                offsetY + (int)Math.Round((HoleTop + HoleRadius) * scale));
            try
            {
                if (region == IntPtr.Zero || hole == IntPtr.Zero ||
                    NativeMethods.CombineRgn(region, region, hole, 4 /* RGN_DIFF */) == 0)
                {
                    throw new InvalidOperationException("Could not create the window region.");
                }
                if (NativeMethods.SetWindowRgn(_hwnd, region, true) == 0)
                {
                    throw new InvalidOperationException("SetWindowRgn could not apply the window region.");
                }
                // After success, Windows owns this HRGN. The temporary hole remains ours.
                region = IntPtr.Zero;
            }
            finally
            {
                if (region != IntPtr.Zero) NativeMethods.DeleteObject(region);
                if (hole != IntPtr.Zero) NativeMethods.DeleteObject(hole);
            }
        }
    }

    internal static class NativeMethods
    {
        [StructLayout(LayoutKind.Sequential)]
        internal struct Rect
        {
            internal int Left, Top, Right, Bottom;
        }

        [StructLayout(LayoutKind.Sequential)]
        internal struct Point
        {
            internal int X, Y;
        }

        [DllImport("user32.dll")]
        internal static extern uint GetDpiForWindow(IntPtr hwnd);

        [DllImport("user32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        internal static extern bool GetWindowRect(IntPtr hwnd, out Rect rect);

        [DllImport("user32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        internal static extern bool GetClientRect(IntPtr hwnd, out Rect rect);

        [DllImport("user32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        internal static extern bool ClientToScreen(IntPtr hwnd, ref Point point);

        [DllImport("user32.dll")]
        internal static extern int SetWindowRgn(IntPtr hwnd, IntPtr region, [MarshalAs(UnmanagedType.Bool)] bool redraw);

        [DllImport("gdi32.dll")]
        internal static extern IntPtr CreateEllipticRgn(int left, int top, int right, int bottom);

        [DllImport("gdi32.dll")]
        internal static extern int CombineRgn(IntPtr destination, IntPtr source1, IntPtr source2, int mode);

        [DllImport("gdi32.dll")]
        [return: MarshalAs(UnmanagedType.Bool)]
        internal static extern bool DeleteObject(IntPtr obj);
    }
}
