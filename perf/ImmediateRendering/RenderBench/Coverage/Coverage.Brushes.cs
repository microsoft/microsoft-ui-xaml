using Microsoft.UI;
using Microsoft.UI.Composition;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Documents;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Media.Imaging;
using Microsoft.UI.Xaml.Shapes;
using Windows.Foundation;
using Windows.UI;

namespace RenderBench.Coverage;

// F2 brushes: LinearGradientBrush spread / interpolation / transforms, MUXC RadialGradientBrush, gradient text,
// ImageBrush stretch / alignment / transforms.
public static class BrushesCoverage
{
    static Canvas Surface(double width = 360, double height = 240) =>
        new() { Width = width, Height = height, Background = new SolidColorBrush(Colors.White) };

    static T At<T>(T element, double left, double top) where T : UIElement
    {
        Canvas.SetLeft(element, left);
        Canvas.SetTop(element, top);
        return element;
    }

    static GradientStopCollection Stops() =>
    [
        new GradientStop { Color = Color.FromArgb(255, 0xE0, 0x20, 0x20), Offset = 0 },
        new GradientStop { Color = Color.FromArgb(255, 0xF0, 0xD0, 0x20), Offset = 0.4 },
        new GradientStop { Color = Color.FromArgb(255, 0x20, 0x40, 0xE0), Offset = 1 },
    ];

    static Rectangle Swatch(Brush fill, double width = 100, double height = 60) => new() { Width = width, Height = height, Fill = fill };

    // SpreadMethod Pad / Reflect / Repeat, absolute mapping, and sRGB vs linear (ScRgb) interpolation.
    [CoveragePage("F2-brushes", "linear-spread-interpolation", MaxMeanAbsDiff = 0.01)]
    public static CoveragePage LinearSpread(CoverageContext context)
    {
        var root = Surface();
        GradientSpreadMethod[] spreads = [GradientSpreadMethod.Pad, GradientSpreadMethod.Reflect, GradientSpreadMethod.Repeat];
        for (int i = 0; i < spreads.Length; i++)
        {
            root.Children.Add(At(Swatch(new LinearGradientBrush
            {
                StartPoint = new(0.35, 0), EndPoint = new(0.65, 0), SpreadMethod = spreads[i], GradientStops = Stops(),
            }), 12 + i * 115, 12));
            root.Children.Add(At(Swatch(new LinearGradientBrush
            {
                MappingMode = BrushMappingMode.Absolute, StartPoint = new(20, 10), EndPoint = new(45, 40), SpreadMethod = spreads[i], GradientStops = Stops(),
            }), 12 + i * 115, 84));
        }
        root.Children.Add(At(Swatch(new LinearGradientBrush
        {
            StartPoint = new(0, 0), EndPoint = new(1, 0), ColorInterpolationMode = ColorInterpolationMode.SRgbLinearInterpolation, GradientStops = Stops(),
        }, 160, 30), 12, 160));
        root.Children.Add(At(Swatch(new LinearGradientBrush
        {
            StartPoint = new(0, 0), EndPoint = new(1, 0), ColorInterpolationMode = ColorInterpolationMode.ScRgbLinearInterpolation, GradientStops = Stops(),
        }, 160, 30), 12, 196));
        root.Children.Add(At(Swatch(new LinearGradientBrush
        {
            StartPoint = new(0, 0), EndPoint = new(1, 0), Opacity = 0.5, GradientStops =
            {
                new GradientStop { Color = Color.FromArgb(0, 0xE0, 0x20, 0x20), Offset = 0 },
                new GradientStop { Color = Color.FromArgb(255, 0x20, 0x40, 0xE0), Offset = 1 },
            },
        }, 160, 66), 190, 160));
        return new CoveragePage { Root = root };
    }

    // LinearGradientBrush Transform and RelativeTransform (rotation / skew of the gradient, not the element).
    [CoveragePage("F2-brushes", "linear-transforms", MaxMeanAbsDiff = 0.01)]
    public static CoveragePage LinearTransforms(CoverageContext context)
    {
        var root = Surface();
        root.Children.Add(At(Swatch(new LinearGradientBrush
        {
            StartPoint = new(0, 0), EndPoint = new(1, 0), GradientStops = Stops(),
            RelativeTransform = new RotateTransform { Angle = 45, CenterX = 0.5, CenterY = 0.5 },
        }, 150, 90), 12, 12));
        root.Children.Add(At(Swatch(new LinearGradientBrush
        {
            StartPoint = new(0, 0), EndPoint = new(1, 0), GradientStops = Stops(), SpreadMethod = GradientSpreadMethod.Reflect,
            Transform = new CompositeTransform { ScaleX = 0.3, SkewY = 20, TranslateX = 20 },
        }, 150, 90), 190, 12));
        root.Children.Add(At(new Ellipse
        {
            Width = 150, Height = 100, Fill = GradientStopsFill(),
        }, 12, 120));
        root.Children.Add(At(new Border
        {
            Width = 150, Height = 100, CornerRadius = new CornerRadius(20),
            Background = new LinearGradientBrush { StartPoint = new(0, 1), EndPoint = new(1, 0), GradientStops = Stops(), RelativeTransform = new ScaleTransform { ScaleX = 0.5 } },
        }, 190, 120));
        return new CoveragePage { Root = root };
    }

    static Brush GradientStopsFill() => new LinearGradientBrush { StartPoint = new(0, 0), EndPoint = new(0, 1), GradientStops = Stops(), RelativeTransform = new RotateTransform { Angle = -30 } };

    // MUXC RadialGradientBrush: center, gradient origin (focal point), radii, mapping, spread and interpolation space.
    [CoveragePage("F2-brushes", "radial-gradients", MaxMeanAbsDiff = 0.01)]
    public static CoveragePage RadialGradients(CoverageContext context)
    {
        var root = Surface();
        RadialGradientBrush Radial(Action<RadialGradientBrush> configure)
        {
            var brush = new RadialGradientBrush();
            foreach (var stop in Stops())
            {
                brush.GradientStops.Add(stop);
            }
            configure(brush);
            return brush;
        }
        root.Children.Add(At(Swatch(Radial(_ => { }), 100, 70), 12, 12));
        root.Children.Add(At(Swatch(Radial(b => { b.GradientOrigin = new(0.25, 0.3); b.RadiusX = 0.6; b.RadiusY = 0.4; }), 100, 70), 127, 12));
        root.Children.Add(At(Swatch(Radial(b => { b.RadiusX = 0.2; b.RadiusY = 0.2; b.SpreadMethod = GradientSpreadMethod.Reflect; }), 100, 70), 242, 12));
        root.Children.Add(At(Swatch(Radial(b => { b.RadiusX = 0.2; b.RadiusY = 0.2; b.SpreadMethod = GradientSpreadMethod.Repeat; }), 100, 70), 12, 92));
        root.Children.Add(At(Swatch(Radial(b =>
        {
            b.MappingMode = BrushMappingMode.Absolute; b.Center = new(30, 30); b.GradientOrigin = new(40, 20); b.RadiusX = 50; b.RadiusY = 30;
        }), 100, 70), 127, 92));
        root.Children.Add(At(Swatch(Radial(b => b.InterpolationSpace = CompositionColorSpace.RgbLinear), 100, 70), 242, 92));
        root.Children.Add(At(new Ellipse { Width = 120, Height = 60, Fill = Radial(b => b.GradientOrigin = new(0.7, 0.7)) }, 12, 172));
        root.Children.Add(At(new Border
        {
            Width = 120, Height = 60, CornerRadius = new CornerRadius(10), BorderThickness = new Thickness(6),
            BorderBrush = Radial(_ => { }), Background = new SolidColorBrush(Color.FromArgb(255, 0xF0, 0xF0, 0xF0)),
        }, 160, 172));
        return new CoveragePage { Root = root };
    }

    // Gradient foregrounds on TextBlock and RichTextBlock glyph runs (mapped to each line's bounds).
    [CoveragePage("F2-brushes", "gradient-text", MaxMeanAbsDiff = 0.01)]
    public static CoveragePage GradientText(CoverageContext context)
    {
        var root = Surface();
        root.Children.Add(At(new TextBlock
        {
            Text = "Linear gradient text", FontSize = 30, FontWeight = Microsoft.UI.Text.FontWeights.Bold,
            Foreground = new LinearGradientBrush { StartPoint = new(0, 0), EndPoint = new(1, 0), GradientStops = Stops() },
        }, 12, 8));
        root.Children.Add(At(new TextBlock
        {
            Text = "Two lines of\nvertical gradient", FontSize = 24,
            Foreground = new LinearGradientBrush { StartPoint = new(0, 0), EndPoint = new(0, 1), GradientStops = Stops() },
        }, 12, 56));
        var radial = new RadialGradientBrush();
        foreach (var stop in Stops())
        {
            radial.GradientStops.Add(stop);
        }
        var paragraph = new Paragraph { FontSize = 26 };
        paragraph.Inlines.Add(new Run { Text = "Radial " });
        paragraph.Inlines.Add(new Run { Text = "rich text", Foreground = new LinearGradientBrush { StartPoint = new(0, 0), EndPoint = new(1, 1), GradientStops = Stops() } });
        root.Children.Add(At(new RichTextBlock { Foreground = radial, Blocks = { paragraph } }, 12, 150));
        return new CoveragePage { Root = root };
    }

    // ImageBrush Stretch, AlignmentX / AlignmentY and Transform / RelativeTransform on rectangles, ellipses and borders.
    [CoveragePage("F2-brushes", "image-brush", MaxMeanAbsDiff = 0.015)]
    public static CoveragePage ImageBrushes(CoverageContext context)
    {
        var root = Surface();
        var bitmap = new BitmapImage();
        ImageBrush Image(Stretch stretch, AlignmentX x = AlignmentX.Center, AlignmentY y = AlignmentY.Center) =>
            new() { ImageSource = bitmap, Stretch = stretch, AlignmentX = x, AlignmentY = y };
        Stretch[] stretches = [Stretch.None, Stretch.Fill, Stretch.Uniform, Stretch.UniformToFill];
        for (int i = 0; i < stretches.Length; i++)
        {
            root.Children.Add(At(Swatch(Image(stretches[i], AlignmentX.Left, AlignmentY.Bottom), 80, 60), 12 + i * 86, 12));
            root.Children.Add(At(Swatch(Image(stretches[i], AlignmentX.Right, AlignmentY.Top), 60, 80), 12 + i * 86, 80));
        }
        var rotated = Image(Stretch.UniformToFill);
        rotated.RelativeTransform = new RotateTransform { Angle = 30, CenterX = 0.5, CenterY = 0.5 };
        root.Children.Add(At(new Ellipse { Width = 100, Height = 70, Fill = rotated }, 12, 166));
        var scaled = Image(Stretch.Uniform);
        scaled.Transform = new ScaleTransform { ScaleX = 0.5, ScaleY = 0.75 };
        root.Children.Add(At(new Border { Width = 100, Height = 70, CornerRadius = new CornerRadius(12), Background = scaled }, 130, 166));
        return new CoveragePage
        {
            Root = root,
            Exercise = async () =>
            {
                string directory = GalleryScene.ImageDirectory(context.Seed);
                await GalleryScene.GenerateImagesAsync(directory, context.Seed, 1);
                var opened = new TaskCompletionSource();
                bitmap.ImageOpened += (_, _) => opened.TrySetResult();
                bitmap.ImageFailed += (_, e) => opened.TrySetException(new InvalidOperationException(e.ErrorMessage));
                using (var file = System.IO.File.OpenRead(System.IO.Path.Combine(directory, "img0000.png")))
                {
                    await bitmap.SetSourceAsync(System.IO.WindowsRuntimeStreamExtensions.AsRandomAccessStream(file));
                }
                await opened.Task.WaitAsync(TimeSpan.FromSeconds(10));
            },
        };
    }
}
