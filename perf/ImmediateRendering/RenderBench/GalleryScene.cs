using System.Diagnostics;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Media.Imaging;
using Windows.Graphics.Imaging;
using Windows.Storage.Streams;
using Windows.UI;

namespace RenderBench;

// S5: gallery. A GridView of Items seeded 256x256 PNGs shown at 128x128. By default DecodePixelWidth is unset, so
// every realized image is decoded at full size (decode-memory stress); --decode-to-size sets DecodePixelWidth=128.
// Interaction: continuous scrolling plus hover zoom (ScaleTransform 1.1) on a few visible items.
sealed class GalleryScene : IScene
{
    public const int ImageSize = 256, TileSize = 128;

    const string Template = """
        <DataTemplate xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation">
          <Border Width="128" Height="128" CornerRadius="4" Background="#FFE0E0E0" RenderTransformOrigin="0.5,0.5">
            <Border.RenderTransform><ScaleTransform/></Border.RenderTransform>
            <Image Stretch="UniformToFill"/>
          </Border>
        </DataTemplate>
        """;

    readonly BenchOptions _options;
    readonly GridView _grid;
    readonly string _directory;
    readonly HashSet<Border> _hooked = [];
    readonly List<Border> _zoomed = [];
    readonly Random _random;
    ScrollViewer? _viewer;
    ScrollDriver? _driver;

    public string Name => _options.DecodeToSize ? "Gallery-DecodeToSize" : "Gallery";
    public bool IsDynamic => false;
    public bool ContinuousInteraction => true;
    public FrameworkElement Root { get; }
    public IReadOnlyList<FrameworkElement> InputTargets => [];

    public GalleryScene(BenchOptions options)
    {
        _options = options;
        _random = new Random(options.Seed);
        _directory = ImageDirectory(options.Seed);
        _grid = new GridView
        {
            ItemsSource = Enumerable.Range(0, options.Items).ToList(),
            ItemTemplate = (DataTemplate)XamlReader.Load(Template),
            SelectionMode = ListViewSelectionMode.None,
            Padding = new Thickness(12),
        };
        _grid.ContainerContentChanging += OnContainerContentChanging;
        Root = new Grid { Background = new SolidColorBrush(Color.FromArgb(255, 0xF3, 0xF3, 0xF3)), Children = { _grid } };
    }

    public static string ImageDirectory(int seed) => Path.Combine(Path.GetTempPath(), "RenderBench", "images", $"seed{seed}");

    // Generates the images on first use for this seed (deterministic content; a marker records completion).
    public Task PrepareAsync() => GenerateImagesAsync(_directory, _options.Seed, _options.Items);

    public static async Task GenerateImagesAsync(string directory, int seed, int count)
    {
        string marker = Path.Combine(directory, "complete.txt");
        if (File.Exists(marker) && int.TryParse(File.ReadAllText(marker), out int done) && done >= count)
        {
            return;
        }
        Directory.CreateDirectory(directory);
        await Parallel.ForEachAsync(Enumerable.Range(0, count), new ParallelOptions { MaxDegreeOfParallelism = 8 }, async (i, _) =>
        {
            byte[] pixels = Render(seed, i);
            using var stream = new InMemoryRandomAccessStream();
            var encoder = await BitmapEncoder.CreateAsync(BitmapEncoder.PngEncoderId, stream);
            encoder.SetPixelData(BitmapPixelFormat.Bgra8, BitmapAlphaMode.Ignore, ImageSize, ImageSize, 96, 96, pixels);
            await encoder.FlushAsync();
            stream.Seek(0);
            using var file = File.Create(ImagePath(directory, i));
            await stream.AsStreamForRead().CopyToAsync(file);
        });
        File.WriteAllText(marker, count.ToString(System.Globalization.CultureInfo.InvariantCulture));
    }

    static string ImagePath(string directory, int index) => Path.Combine(directory, $"img{index:0000}.png");

    // Seeded diagonal gradient with a few filled circles and rectangles.
    static byte[] Render(int seed, int index)
    {
        var random = new Random(seed * 100_003 + index);
        var pixels = new byte[ImageSize * ImageSize * 4];
        Color a = RandomColor(random), b = RandomColor(random);
        for (int y = 0; y < ImageSize; y++)
        {
            for (int x = 0; x < ImageSize; x++)
            {
                double t = (x + y) / (2.0 * (ImageSize - 1));
                int o = (y * ImageSize + x) * 4;
                pixels[o] = (byte)(a.B + (b.B - a.B) * t);
                pixels[o + 1] = (byte)(a.G + (b.G - a.G) * t);
                pixels[o + 2] = (byte)(a.R + (b.R - a.R) * t);
                pixels[o + 3] = 255;
            }
        }
        int shapes = 2 + random.Next(4);
        for (int s = 0; s < shapes; s++)
        {
            Color c = RandomColor(random);
            int cx = random.Next(ImageSize), cy = random.Next(ImageSize), r = 12 + random.Next(60);
            bool circle = random.Next(2) == 0;
            for (int y = Math.Max(0, cy - r); y < Math.Min(ImageSize, cy + r); y++)
            {
                for (int x = Math.Max(0, cx - r); x < Math.Min(ImageSize, cx + r); x++)
                {
                    if (circle && (x - cx) * (x - cx) + (y - cy) * (y - cy) > r * r)
                    {
                        continue;
                    }
                    int o = (y * ImageSize + x) * 4;
                    pixels[o] = c.B;
                    pixels[o + 1] = c.G;
                    pixels[o + 2] = c.R;
                }
            }
        }
        return pixels;
    }

    static Color RandomColor(Random random) => Color.FromArgb(255, (byte)random.Next(256), (byte)random.Next(256), (byte)random.Next(256));

    void OnContainerContentChanging(ListViewBase sender, ContainerContentChangingEventArgs args)
    {
        if (args.ItemContainer.ContentTemplateRoot is not Border border || border.Child is not Image image)
        {
            return;
        }
        args.Handled = true;
        if (args.InRecycleQueue)
        {
            image.Source = null;
            return;
        }
        if (_hooked.Add(border))
        {
            border.PointerEntered += (_, _) => Zoom(border, true);
            border.PointerExited += (_, _) => Zoom(border, false);
        }
        var bitmap = new BitmapImage();
        if (_options.DecodeToSize)
        {
            bitmap.DecodePixelWidth = TileSize;
        }
        long start = Stopwatch.GetTimestamp();
        bitmap.ImageOpened += (_, _) => BenchEventSource.Log.SceneValue("ImageLoadMs", Stopwatch.GetElapsedTime(start).TotalMilliseconds);
        image.Source = bitmap;
        _ = LoadAsync(bitmap, ImagePath(_directory, (int)args.Item));
    }

    static async Task LoadAsync(BitmapImage bitmap, string path)
    {
        try
        {
            using var file = File.OpenRead(path);
            await bitmap.SetSourceAsync(file.AsRandomAccessStream());
        }
        catch (Exception ex)
        {
            Console.Error.WriteLine($"{path}: {ex.Message}");
        }
    }

    static void Zoom(Border border, bool zoomed)
    {
        if (border.RenderTransform is ScaleTransform scale)
        {
            scale.ScaleX = scale.ScaleY = zoomed ? 1.1 : 1.0;
        }
    }

    public int ApplyUpdate() => 0;

    public void Attach(ISceneHost host)
    {
        _viewer = Tree.Find<ScrollViewer>(_grid);
        if (_viewer is not null)
        {
            _driver = new ScrollDriver(_viewer, _options.ScrollSpeed);
        }
    }

    public Task? WarmupAsync(ISceneHost host) => _viewer is null ? null : WarmupScrollAsync(_viewer);

    static async Task WarmupScrollAsync(ScrollViewer viewer)
    {
        viewer.ChangeView(null, 1500, null, disableAnimation: true);
        await Task.Delay(500);
        viewer.ChangeView(null, 0, null, disableAnimation: true);
        await Task.Delay(500);
    }

    public Task? RunInteractionAsync(ISceneHost host, CancellationToken token)
    {
        if (_driver is null)
        {
            return null;
        }
        if (host.InputMode == InputMode.SendInput)
        {
            // Wheel notches scroll; hover moves the real cursor over a visible tile in between.
            return Task.WhenAll(
                host.Every(5, () => _driver.Wheel(host, _grid), token),
                host.Every(Math.Max(0.5, host.Options.InputHz), () => HoverWithMouse(host), token));
        }
        return Task.WhenAll(
            host.EveryFrame(_driver.Advance, token),
            host.Every(Math.Max(0.5, host.Options.InputHz), () => HoverProgrammatic(host), token));
    }

    List<Border> VisibleTiles()
    {
        var result = new List<Border>();
        if (_grid.ItemsPanelRoot is not { } panel)
        {
            return result;
        }
        foreach (var child in panel.Children)
        {
            if (child is GridViewItem { ContentTemplateRoot: Border border } item)
            {
                var top = item.TransformToVisual(_grid).TransformPoint(default);
                if (top.Y >= 0 && top.Y + item.ActualHeight <= _grid.ActualHeight && top.X + item.ActualWidth <= _grid.ActualWidth)
                {
                    result.Add(border);
                }
            }
        }
        return result;
    }

    void HoverProgrammatic(ISceneHost host)
    {
        foreach (var border in _zoomed)
        {
            Zoom(border, false);
        }
        _zoomed.Clear();
        var visible = VisibleTiles();
        for (int i = 0; i < 3 && visible.Count > 0; i++)
        {
            var border = visible[_random.Next(visible.Count)];
            host.MarkInput("hover", "GridViewItem", programmatic: true);
            Zoom(border, true);
            _zoomed.Add(border);
        }
    }

    void HoverWithMouse(ISceneHost host)
    {
        var visible = VisibleTiles();
        if (visible.Count == 0)
        {
            return;
        }
        var (x, y) = host.ScreenPoint(visible[_random.Next(visible.Count)]);
        host.MarkInput("hover", "GridViewItem", programmatic: false);
        Win32.MouseMove(x, y);
    }

    public IEnumerable<(string Name, double Value)> Counters()
    {
        if (_grid.ItemsPanelRoot is { } panel)
        {
            int realized = panel.Children.Count;
            int side = _options.DecodeToSize ? TileSize : ImageSize;
            yield return ("RealizedItems", realized);
            yield return ("EstimatedDecodedImageMB", realized * side * side * 4 / (1024.0 * 1024.0));
        }
    }
}
