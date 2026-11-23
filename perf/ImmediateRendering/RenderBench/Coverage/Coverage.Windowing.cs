using Microsoft.UI;
using Microsoft.UI.Windowing;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Hosting;
using Microsoft.UI.Xaml.Media;
using Windows.UI;

namespace RenderBench.Coverage;

// F13 windowing and hosting (test plan, "XAML coverage"). Pages change the window around a small content tree (backdrop,
// title bar) or host XAML in additional top-level windows; none of them may fall back. Secondary windows are closed in
// Detach.
static class WindowingCoverage
{
    const string Family = "F13-windowing";

    static StackPanel Content(string title) => new()
    {
        Spacing = 12,
        Padding = new Thickness(16),
        Children =
        {
            new TextBlock { Text = title, FontSize = 20 },
            new Button { Content = "Button" },
            new Border { Width = 160, Height = 40, CornerRadius = new CornerRadius(6), Background = new SolidColorBrush(Color.FromArgb(0xC0, 0x46, 0x82, 0xB4)) },
            new ToggleSwitch { IsOn = true },
        },
    };

    // SystemBackdrop: the root is transparent so the system-drawn Mica / DesktopAcrylic shows through the premultiplied
    // swap chain; no Composition content is needed in the window.
    [CoveragePage(Family, "mica-backdrop", MaxMeanAbsDiff = 0.02)]
    public static CoveragePage Mica(CoverageContext context)
    {
        var previous = context.Window.SystemBackdrop;
        context.Window.SystemBackdrop = new MicaBackdrop();
        return new CoveragePage
        {
            Root = new Grid { Background = new SolidColorBrush(Colors.Transparent), Children = { Content("Mica") } },
            Exercise = () => Task.Delay(500),
            Detach = () => context.Window.SystemBackdrop = previous,
        };
    }

    [CoveragePage(Family, "desktop-acrylic-backdrop", MaxMeanAbsDiff = 0.02)]
    public static CoveragePage DesktopAcrylic(CoverageContext context)
    {
        var previous = context.Window.SystemBackdrop;
        context.Window.SystemBackdrop = new DesktopAcrylicBackdrop();
        return new CoveragePage
        {
            Root = new Grid { Children = { Content("DesktopAcrylic") } },
            Exercise = () => Task.Delay(500),
            Detach = () => context.Window.SystemBackdrop = previous,
        };
    }

    // ExtendsContentIntoTitleBar with a XAML drag region; the caption buttons are drawn by the system.
    [CoveragePage(Family, "custom-title-bar")]
    public static CoveragePage TitleBar(CoverageContext context)
    {
        var titleBar = new Grid
        {
            Height = 32,
            Background = new SolidColorBrush(Color.FromArgb(255, 0x20, 0x40, 0x60)),
            Children = { new TextBlock { Text = "Custom title bar", Margin = new Thickness(12, 0, 0, 0), VerticalAlignment = VerticalAlignment.Center, Foreground = new SolidColorBrush(Colors.White) } },
        };
        var root = new Grid { Background = new SolidColorBrush(Colors.White), RowDefinitions = { new RowDefinition { Height = GridLength.Auto }, new RowDefinition() } };
        root.Children.Add(titleBar);
        var body = Content("Content under a custom title bar");
        Grid.SetRow(body, 1);
        root.Children.Add(body);
        return new CoveragePage
        {
            Root = root,
            Exercise = async () =>
            {
                context.Window.ExtendsContentIntoTitleBar = true;
                context.Window.SetTitleBar(titleBar);
                await Task.Delay(400);
            },
            Detach = () =>
            {
                context.Window.SetTitleBar(null);
                context.Window.ExtendsContentIntoTitleBar = false;
            },
        };
    }

    // A second XAML Window and a DesktopWindowXamlSource island in a plain AppWindow (Win32 host), resized and
    // minimized/restored, then hidden before the capture (a screen grab of the main window). The secondary windows must\r
    // stay immediate and not fail.
    [CoveragePage(Family, "secondary-hosts")]
    public static CoveragePage SecondaryHosts(CoverageContext context)
    {
        Window? second = null;
        AppWindow? host = null;
        DesktopWindowXamlSource? source = null;
        return new CoveragePage
        {
            Root = new Grid { Background = new SolidColorBrush(Colors.White), Children = { Content("Main window") } },
            Exercise = async () =>
            {
                second = new Window { Content = new Grid { Background = new SolidColorBrush(Colors.Beige), Children = { Content("Second window") } } };
                second.AppWindow.Resize(new Windows.Graphics.SizeInt32(400, 300));
                second.Activate();

                host = AppWindow.Create();
                host.Resize(new Windows.Graphics.SizeInt32(420, 320));
                source = new DesktopWindowXamlSource();
                source.Initialize(host.Id);
                source.Content = new Grid { Background = new SolidColorBrush(Colors.Lavender), Children = { Content("DesktopWindowXamlSource") } };
                host.Show();
                await Task.Delay(400);

                // Resize storm and minimize/restore on both secondary windows.
                for (int i = 0; i < 10; i++)
                {
                    second.AppWindow.Resize(new Windows.Graphics.SizeInt32(400 + i * 23, 300 + i * 17));
                    host.Resize(new Windows.Graphics.SizeInt32(420 + i * 19, 320 + i * 11));
                    await Task.Delay(30);
                }
                if (host.Presenter is OverlappedPresenter overlapped)
                {
                    overlapped.Minimize();
                    await Task.Delay(200);
                    overlapped.Restore();
                }
                await Task.Delay(300);

                // The capture is a screen grab of the main window's client area: hide the secondary windows first.
                host.Hide();
                second.AppWindow.Hide();
                await Task.Delay(200);
            },
            Detach = () =>
            {
                source?.Dispose();
                host?.Destroy();
                second?.Close();
            },
        };
    }

    // Moving an element between windows: the same Border is removed from the page, shown in a second window, then
    // put back (XamlRoot changes twice).
    [CoveragePage(Family, "xamlroot-move")]
    public static CoveragePage XamlRootMove(CoverageContext context)
    {
        var moving = new Border { Width = 200, Height = 60, CornerRadius = new CornerRadius(8), Background = new SolidColorBrush(Colors.Teal), Child = new TextBlock { Text = "Moving element", Foreground = new SolidColorBrush(Colors.White), Margin = new Thickness(12) } };
        var slot = new Grid();
        slot.Children.Add(moving);
        Window? second = null;
        return new CoveragePage
        {
            Root = new Grid { Background = new SolidColorBrush(Colors.White), Children = { new StackPanel { Padding = new Thickness(16), Spacing = 12, Children = { new TextBlock { Text = "XamlRoot move" }, slot } } } },
            Exercise = async () =>
            {
                await Task.Delay(200);
                slot.Children.Remove(moving);
                var other = new Grid();
                second = new Window { Content = other };
                second.Activate();
                other.Children.Add(moving);
                await Task.Delay(300);
                other.Children.Remove(moving);
                slot.Children.Add(moving);
                await Task.Delay(300);
                second.Close();
                second = null;
                await Task.Delay(200);
            },
            Detach = () => second?.Close(),
        };
    }
}
