using Microsoft.UI;
using System.Runtime.InteropServices.WindowsRuntime;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Media.Imaging;
using Microsoft.UI.Xaml.Shapes;
using Windows.Graphics.Imaging;
using Windows.Storage.Streams;
using Windows.UI;

namespace RenderBench.Coverage;

// F7 images and icons (test plan, "XAML coverage"). Every page builds its bitmaps in code (seeded, deterministic) so the
// Composition and immediate renderings decode the same pixels.
static class ImagesCoverage
{
    const string Family = "F7-images";

    // ---- helpers ----

    // A size x size premultiplied BGRA test pattern: quadrant colors, a diagonal and a one-pixel frame, so stretch,
    // alignment, nine-grid slicing and filtering errors are visible.
    internal static byte[] Pattern(int width, int height, int seed = 1)
    {
        var random = new Random(seed);
        Color[] quadrants = [Color.FromArgb(255, 0xE8, 0x48, 0x3C), Color.FromArgb(255, 0x2E, 0x86, 0xDE), Color.FromArgb(255, 0x27, 0xAE, 0x60), Color.FromArgb(255, 0xF3, 0x9C, 0x12)];
        quadrants[random.Next(4)] = Color.FromArgb(255, 0x8E, 0x44, 0xAD);
        var pixels = new byte[width * height * 4];
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                Color c = quadrants[(x * 2 / width) + 2 * (y * 2 / height)];
                if (x == 0 || y == 0 || x == width - 1 || y == height - 1)
                {
                    c = Color.FromArgb(255, 0x20, 0x20, 0x20);
                }
                else if (Math.Abs(x * height - y * width) < width)
                {
                    c = Color.FromArgb(255, 255, 255, 255);
                }
                int o = (y * width + x) * 4;
                pixels[o] = c.B;
                pixels[o + 1] = c.G;
                pixels[o + 2] = c.R;
                pixels[o + 3] = c.A;
            }
        }
        return pixels;
    }

    internal static async Task<IRandomAccessStream> EncodeAsync(Guid encoderId, int width, int height, byte[] pixels, BitmapAlphaMode alpha = BitmapAlphaMode.Ignore)
    {
        var stream = new InMemoryRandomAccessStream();
        var encoder = await BitmapEncoder.CreateAsync(encoderId, stream);
        encoder.SetPixelData(BitmapPixelFormat.Bgra8, alpha, (uint)width, (uint)height, 96, 96, pixels);
        await encoder.FlushAsync();
        stream.Seek(0);
        return stream;
    }

    internal static async Task<BitmapImage> PngAsync(int width, int height, int seed = 1, int decodePixelWidth = 0)
    {
        var bitmap = new BitmapImage();
        if (decodePixelWidth > 0)
        {
            bitmap.DecodePixelWidth = decodePixelWidth;
        }
        using var stream = await EncodeAsync(BitmapEncoder.PngEncoderId, width, height, Pattern(width, height, seed));
        await bitmap.SetSourceAsync(stream);
        return bitmap;
    }

    static Border Frame(UIElement child, double width, double height) => new()
    {
        Width = width,
        Height = height,
        Background = new SolidColorBrush(Color.FromArgb(255, 0xEE, 0xEE, 0xEE)),
        BorderBrush = new SolidColorBrush(Color.FromArgb(255, 0x99, 0x99, 0x99)),
        BorderThickness = new Thickness(1),
        Child = child,
    };

    static StackPanel Row(params UIElement[] children)
    {
        var row = new StackPanel { Orientation = Orientation.Horizontal, Spacing = 12 };
        foreach (var child in children)
        {
            row.Children.Add(child);
        }
        return row;
    }

    static Grid PageRoot(params UIElement[] rows)
    {
        var stack = new StackPanel { Spacing = 12, Padding = new Thickness(16) };
        foreach (var row in rows)
        {
            stack.Children.Add(row);
        }
        return new Grid { Background = new SolidColorBrush(Colors.White), Children = { stack } };
    }

    // ---- pages ----

    // Stretch modes and alignments of a 64x48 PNG in 96x72 / 48x96 frames.
    [CoveragePage(Family, "stretch-alignment", MaxMeanAbsDiff = 0.002)]
    public static CoveragePage StretchAlignment(CoverageContext context)
    {
        var rows = new StackPanel { Spacing = 12 };
        var root = PageRoot(rows);
        return new CoveragePage
        {
            Root = root,
            Exercise = async () =>
            {
                foreach (var (w, h) in new[] { (96.0, 72.0), (48.0, 96.0) })
                {
                    var row = Row();
                    foreach (Stretch stretch in Enum.GetValues<Stretch>())
                    {
                        foreach (var (ha, va) in new[] { (HorizontalAlignment.Left, VerticalAlignment.Top), (HorizontalAlignment.Center, VerticalAlignment.Center), (HorizontalAlignment.Right, VerticalAlignment.Bottom) })
                        {
                            var image = new Image { Source = await PngAsync(64, 48, context.Seed), Stretch = stretch, HorizontalAlignment = ha, VerticalAlignment = va };
                            row.Children.Add(Frame(image, w, h));
                        }
                    }
                    rows.Children.Add(row);
                }
                await Task.Delay(200);
            },
        };
    }

    // Image.NineGrid: corners keep their size, edges and center stretch (CompositionNineGridBrush in Composition).
    [CoveragePage(Family, "nine-grid", MaxMeanAbsDiff = 0.003)]
    public static CoveragePage NineGrid(CoverageContext context)
    {
        var row = Row();
        return new CoveragePage
        {
            Root = PageRoot(row),
            Exercise = async () =>
            {
                var source = await PngAsync(48, 48, context.Seed);
                row.Children.Add(new Image { Source = source, NineGrid = new Thickness(12), Stretch = Stretch.Fill, Width = 200, Height = 80 });
                row.Children.Add(new Image { Source = source, NineGrid = new Thickness(4, 16, 20, 8), Stretch = Stretch.Fill, Width = 120, Height = 160 });
                row.Children.Add(new Image { Source = source, NineGrid = new Thickness(30), Stretch = Stretch.Fill, Width = 40, Height = 40 }); // overlapping insets
                row.Children.Add(new Image { Source = source, NineGrid = new Thickness(12), Stretch = Stretch.Uniform, Width = 100, Height = 100 });
                await Task.Delay(200);
            },
        };
    }

    // ImageBrush as Border.Background, Rectangle/Ellipse Fill, with Stretch and AlignmentX/Y.
    [CoveragePage(Family, "image-brush", MaxMeanAbsDiff = 0.002)]
    public static CoveragePage ImageBrushes(CoverageContext context)
    {
        var row = Row();
        return new CoveragePage
        {
            Root = PageRoot(row),
            Exercise = async () =>
            {
                var source = await PngAsync(64, 64, context.Seed);
                row.Children.Add(new Border { Width = 120, Height = 80, CornerRadius = new CornerRadius(8), Background = new ImageBrush { ImageSource = source, Stretch = Stretch.UniformToFill } });
                row.Children.Add(new Rectangle { Width = 120, Height = 80, Fill = new ImageBrush { ImageSource = source, Stretch = Stretch.Uniform, AlignmentX = AlignmentX.Left } });
                row.Children.Add(new Ellipse { Width = 100, Height = 100, Fill = new ImageBrush { ImageSource = source, Stretch = Stretch.Fill } });
                row.Children.Add(new Border { Width = 80, Height = 120, Background = new ImageBrush { ImageSource = source, Stretch = Stretch.None, AlignmentY = AlignmentY.Bottom } });
                await Task.Delay(200);
            },
        };
    }

    // DecodePixelWidth smaller and larger than the displayed size; JPEG and BMP decoders; a transparent PNG over color.
    [CoveragePage(Family, "decode-formats", MaxMeanAbsDiff = 0.002)]
    public static CoveragePage DecodeFormats(CoverageContext context)
    {
        var row = Row();
        return new CoveragePage
        {
            Root = PageRoot(row),
            Exercise = async () =>
            {
                row.Children.Add(new Image { Source = await PngAsync(256, 256, context.Seed, decodePixelWidth: 64), Width = 128, Height = 128 });
                row.Children.Add(new Image { Source = await PngAsync(64, 64, context.Seed, decodePixelWidth: 128), Width = 128, Height = 128 });
                foreach (var encoder in new[] { BitmapEncoder.JpegEncoderId, BitmapEncoder.BmpEncoderId })
                {
                    var bitmap = new BitmapImage();
                    using var stream = await EncodeAsync(encoder, 96, 96, Pattern(96, 96, context.Seed));
                    await bitmap.SetSourceAsync(stream);
                    row.Children.Add(new Image { Source = bitmap, Width = 96, Height = 96 });
                }
                var alpha = Pattern(96, 96, context.Seed);
                for (int i = 0; i < alpha.Length; i += 4)
                {
                    int x = (i / 4) % 96, y = (i / 4) / 96;
                    byte a = (byte)(255 * x / 95);
                    alpha[i] = (byte)(alpha[i] * a / 255); alpha[i + 1] = (byte)(alpha[i + 1] * a / 255); alpha[i + 2] = (byte)(alpha[i + 2] * a / 255); alpha[i + 3] = a;
                    _ = y;
                }
                var transparent = new BitmapImage();
                using (var stream = await EncodeAsync(BitmapEncoder.PngEncoderId, 96, 96, alpha, BitmapAlphaMode.Premultiplied))
                {
                    await transparent.SetSourceAsync(stream);
                }
                row.Children.Add(new Border { Background = new SolidColorBrush(Colors.Teal), Child = new Image { Source = transparent, Width = 96, Height = 96 } });
                await Task.Delay(300);
            },
        };
    }

    // WriteableBitmap: drawn, then its pixels rewritten in place and Invalidate()d (the update must damage and repaint).
    [CoveragePage(Family, "writeable-bitmap", MaxMeanAbsDiff = 0.002)]
    public static CoveragePage Writeable(CoverageContext context)
    {
        var bitmap = new WriteableBitmap(80, 60);
        void Fill(int seed)
        {
            byte[] pixels = Pattern(80, 60, seed);
            using var stream = bitmap.PixelBuffer.AsStream();
            stream.Write(pixels, 0, pixels.Length);
            bitmap.Invalidate();
        }
        Fill(context.Seed);
        return new CoveragePage
        {
            Root = PageRoot(Row(new Image { Source = bitmap, Width = 160, Height = 120 }, new Image { Source = bitmap, Width = 80, Height = 60, Stretch = Stretch.None })),
            Exercise = async () =>
            {
                await Task.Delay(200);
                Fill(context.Seed + 7);
                await Task.Delay(200);
            },
        };
    }

    // SoftwareBitmapSource and a RenderTargetBitmap of live content used as image sources.
    [CoveragePage(Family, "software-and-rtb-sources", MaxMeanAbsDiff = 0.003)]
    public static CoveragePage SoftwareAndRtb(CoverageContext context)
    {
        var captured = new Border
        {
            Width = 100,
            Height = 60,
            Background = new LinearGradientBrush { StartPoint = new(0, 0), EndPoint = new(1, 1), GradientStops = { new GradientStop { Color = Colors.Gold, Offset = 0 }, new GradientStop { Color = Colors.Crimson, Offset = 1 } } },
            Child = new TextBlock { Text = "RTB", FontSize = 24, HorizontalAlignment = HorizontalAlignment.Center, VerticalAlignment = VerticalAlignment.Center },
        };
        var row = Row(captured);
        return new CoveragePage
        {
            Root = PageRoot(row),
            Exercise = async () =>
            {
                var software = SoftwareBitmap.CreateCopyFromBuffer(Pattern(64, 64, context.Seed).AsBuffer(), BitmapPixelFormat.Bgra8, 64, 64, BitmapAlphaMode.Premultiplied);
                var softwareSource = new SoftwareBitmapSource();
                await softwareSource.SetBitmapAsync(software);
                row.Children.Add(new Image { Source = softwareSource, Width = 96, Height = 96 });

                await Task.Delay(100);
                var rtb = new RenderTargetBitmap();
                await rtb.RenderAsync(captured);
                row.Children.Add(new Image { Source = rtb, Width = 100, Height = 60 });
                await Task.Delay(200);
            },
        };
    }

    // SvgImageSource (rasterized by XAML) at two sizes, and a still (AutoPlay=false) GIF.
    [CoveragePage(Family, "svg-gif", MaxMeanAbsDiff = 0.002)]
    public static CoveragePage SvgGif(CoverageContext context)
    {
        var row = Row();
        return new CoveragePage
        {
            Root = PageRoot(row),
            Exercise = async () =>
            {
                const string svg = """
                    <svg xmlns="http://www.w3.org/2000/svg" width="100" height="100" viewBox="0 0 100 100">
                      <rect x="5" y="5" width="90" height="90" rx="12" fill="#2E86DE"/>
                      <circle cx="50" cy="50" r="30" fill="#F39C12" stroke="#202020" stroke-width="4"/>
                      <path d="M20 80 L50 20 L80 80 Z" fill="none" stroke="#FFFFFF" stroke-width="5"/>
                    </svg>
                    """;
                foreach (double size in new[] { 64.0, 140.0 })
                {
                    var source = new SvgImageSource { RasterizePixelWidth = size, RasterizePixelHeight = size };
                    using var stream = new InMemoryRandomAccessStream();
                    await stream.WriteAsync(System.Text.Encoding.UTF8.GetBytes(svg).AsBuffer());
                    stream.Seek(0);
                    await source.SetSourceAsync(stream);
                    row.Children.Add(new Image { Source = source, Width = size, Height = size });
                }
                var gif = new BitmapImage { AutoPlay = false };
                using (var stream = await EncodeAsync(BitmapEncoder.GifEncoderId, 64, 64, Pattern(64, 64, context.Seed)))
                {
                    await gif.SetSourceAsync(stream);
                }
                row.Children.Add(new Image { Source = gif, Width = 128, Height = 128 });
                await Task.Delay(300);
            },
        };
    }

    // Icons: FontIcon, SymbolIcon, PathIcon, BitmapIcon (color and monochrome), ImageIcon and IconSourceElement.
    [CoveragePage(Family, "icons", MaxMeanAbsDiff = 0.002)]
    public static CoveragePage Icons(CoverageContext context)
    {
        var row = Row(
            new FontIcon { Glyph = "\uE734", FontSize = 32, Foreground = new SolidColorBrush(Colors.Crimson) },
            new SymbolIcon { Symbol = Symbol.Mail, Foreground = new SolidColorBrush(Colors.SteelBlue) },
            new PathIcon { Data = (Geometry)Microsoft.UI.Xaml.Markup.XamlBindingHelper.ConvertValue(typeof(Geometry), "M0,0 L20,0 L10,20 Z"), Foreground = new SolidColorBrush(Colors.SeaGreen), Width = 32, Height = 32 },
            new IconSourceElement { IconSource = new SymbolIconSource { Symbol = Symbol.Favorite, Foreground = new SolidColorBrush(Colors.Orange) } });
        return new CoveragePage
        {
            Root = PageRoot(row),
            Exercise = async () =>
            {
                // BitmapIcon/ImageIcon need a URI; write the pattern to a temp file.
                string folder = System.IO.Path.Combine(System.IO.Path.GetTempPath(), "RenderBench", "coverage");
                Directory.CreateDirectory(folder);
                string path = System.IO.Path.Combine(folder, $"icon{context.Seed}.png");
                if (!File.Exists(path))
                {
                    using var stream = await EncodeAsync(BitmapEncoder.PngEncoderId, 32, 32, Pattern(32, 32, context.Seed));
                    using var file = File.Create(path);
                    await stream.AsStreamForRead().CopyToAsync(file);
                }
                var uri = new Uri(path);
                row.Children.Add(new BitmapIcon { UriSource = uri, ShowAsMonochrome = false, Width = 32, Height = 32 });
                row.Children.Add(new BitmapIcon { UriSource = uri, ShowAsMonochrome = true, Foreground = new SolidColorBrush(Colors.Purple), Width = 32, Height = 32 });
                row.Children.Add(new ImageIcon { Source = new BitmapImage(uri), Width = 32, Height = 32 });
                await Task.Delay(500);
            },
        };
    }

    // CalendarView: day items with density bars, today, selected and blackout days (CalendarViewBaseItem chrome).
    [CoveragePage(Family, "calendar-view", MaxMeanAbsDiff = 0.005)]
    public static CoveragePage Calendar(CoverageContext context)
    {
        var start = new DateTimeOffset(2024, 3, 1, 0, 0, 0, TimeSpan.Zero);
        var calendar = new CalendarView
        {
            SelectionMode = CalendarViewSelectionMode.Multiple,
            MinDate = start.AddMonths(-1),
            MaxDate = start.AddMonths(2),
        };
        calendar.CalendarViewDayItemChanging += (_, args) =>
        {
            if (args.Phase == 0)
            {
                args.RegisterUpdateCallback((_, a) =>
                {
                    int day = a.Item.Date.Day;
                    if (day % 5 == 0)
                    {
                        Color[] colors = [Colors.Crimson, Colors.SteelBlue, Colors.Green];
                        a.Item.SetDensityColors(colors[..(1 + day % 3)]);
                    }
                    a.Item.IsBlackout = day == 13;
                });
            }
        };
        return new CoveragePage
        {
            Root = PageRoot(calendar),
            Exercise = async () =>
            {
                calendar.SetDisplayDate(start.AddDays(10));
                calendar.SelectedDates.Add(start.AddDays(3));
                calendar.SelectedDates.Add(start.AddDays(8));
                await Task.Delay(800);
            },
        };
    }
}
