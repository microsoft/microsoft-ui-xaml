using Microsoft.UI;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Documents;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Shapes;
using Windows.Foundation;
using Windows.UI;

namespace RenderBench.Coverage;

// F1 shapes and strokes: stroke dashes, caps, joins and miter limits, gradient strokes, the Glyphs element.
public static class ShapesCoverage
{
    static readonly SolidColorBrush Ink = new(Color.FromArgb(255, 0x20, 0x40, 0x90));

    static Canvas Surface(double width = 360, double height = 240) =>
        new() { Width = width, Height = height, Background = new SolidColorBrush(Colors.White) };

    static T At<T>(T element, double left, double top) where T : UIElement
    {
        Canvas.SetLeft(element, left);
        Canvas.SetTop(element, top);
        return element;
    }

    static PointCollection Points(params double[] xy)
    {
        var points = new PointCollection();
        for (int i = 0; i + 1 < xy.Length; i += 2)
        {
            points.Add(new Point(xy[i], xy[i + 1]));
        }
        return points;
    }

    static DoubleCollection Dashes(params double[] values)
    {
        var dashes = new DoubleCollection();
        foreach (double value in values)
        {
            dashes.Add(value);
        }
        return dashes;
    }

    static LinearGradientBrush Gradient(Point start, Point end) => new()
    {
        StartPoint = start,
        EndPoint = end,
        GradientStops =
        {
            new GradientStop { Color = Color.FromArgb(255, 0xE0, 0x20, 0x20), Offset = 0 },
            new GradientStop { Color = Color.FromArgb(255, 0x20, 0xA0, 0x40), Offset = 0.5 },
            new GradientStop { Color = Color.FromArgb(255, 0x20, 0x40, 0xE0), Offset = 1 },
        },
    };

    // Dash arrays (scaled by StrokeThickness), dash offset and dash caps on lines, rectangles, rounded rectangles and
    // ellipses (where the outline starts matters).
    [CoveragePage("F1-shapes", "dashed-strokes", MaxMeanAbsDiff = 0.01)]
    public static CoveragePage DashedStrokes(CoverageContext context)
    {
        var root = Surface();
        PenLineCap[] caps = [PenLineCap.Flat, PenLineCap.Square, PenLineCap.Round, PenLineCap.Triangle];
        for (int i = 0; i < caps.Length; i++)
        {
            root.Children.Add(At(new Line
            {
                X1 = 0, Y1 = 0, X2 = 150, Y2 = 0,
                Stroke = Ink, StrokeThickness = 6,
                StrokeDashArray = Dashes(3, 2), StrokeDashCap = caps[i], StrokeDashOffset = i * 0.5,
            }, 16, 16 + i * 16));
        }
        root.Children.Add(At(new Rectangle { Width = 120, Height = 70, Stroke = Ink, StrokeThickness = 3, StrokeDashArray = Dashes(4, 2) }, 190, 12));
        root.Children.Add(At(new Rectangle
        {
            Width = 120, Height = 70, RadiusX = 16, RadiusY = 16, Stroke = Ink, StrokeThickness = 3,
            StrokeDashArray = Dashes(2, 1, 0.5, 1), StrokeDashCap = PenLineCap.Round,
        }, 190, 100));
        root.Children.Add(At(new Ellipse { Width = 140, Height = 80, Stroke = Ink, StrokeThickness = 4, StrokeDashArray = Dashes(3), StrokeDashOffset = 1.5 }, 16, 100));
        root.Children.Add(At(new Polyline
        {
            Points = Points(0, 30, 40, 0, 80, 30, 120, 0), Stroke = Ink, StrokeThickness = 3,
            StrokeDashArray = Dashes(5, 2, 1, 2), StrokeLineJoin = PenLineJoin.Round,
        }, 190, 196));
        return new CoveragePage { Root = root };
    }

    // StrokeStartLineCap / StrokeEndLineCap including Triangle, on open lines and polylines.
    [CoveragePage("F1-shapes", "line-caps", MaxMeanAbsDiff = 0.01)]
    public static CoveragePage LineCaps(CoverageContext context)
    {
        var root = Surface();
        PenLineCap[] caps = [PenLineCap.Flat, PenLineCap.Square, PenLineCap.Round, PenLineCap.Triangle];
        for (int i = 0; i < caps.Length; i++)
        {
            root.Children.Add(At(new Line
            {
                X1 = 0, Y1 = 0, X2 = 120, Y2 = 0, Stroke = Ink, StrokeThickness = 14,
                StrokeStartLineCap = caps[i], StrokeEndLineCap = caps[(i + 1) % caps.Length],
            }, 30, 24 + i * 30));
            root.Children.Add(At(new Polyline
            {
                Points = Points(0, 0, 30, 40, 60, 0), Stroke = Ink, StrokeThickness = 8,
                StrokeStartLineCap = caps[i], StrokeEndLineCap = caps[i], StrokeLineJoin = PenLineJoin.Round,
            }, 200 + (i % 2) * 80, 20 + (i / 2) * 90));
        }
        // Diagonal: caps follow the segment direction.
        root.Children.Add(At(new Line { X1 = 0, Y1 = 40, X2 = 60, Y2 = 0, Stroke = Ink, StrokeThickness = 10, StrokeStartLineCap = PenLineCap.Triangle, StrokeEndLineCap = PenLineCap.Square }, 40, 160));
        return new CoveragePage { Root = root };
    }

    // StrokeLineJoin and StrokeMiterLimit on sharp and shallow corners (a clipped miter is not a bevel).
    [CoveragePage("F1-shapes", "line-joins", MaxMeanAbsDiff = 0.01)]
    public static CoveragePage LineJoins(CoverageContext context)
    {
        var root = Surface();
        PenLineJoin[] joins = [PenLineJoin.Miter, PenLineJoin.Bevel, PenLineJoin.Round];
        for (int i = 0; i < joins.Length; i++)
        {
            root.Children.Add(At(new Polyline { Points = Points(0, 80, 30, 0, 60, 80), Stroke = Ink, StrokeThickness = 10, StrokeLineJoin = joins[i] }, 20 + i * 80, 30));
        }
        double[] limits = [1, 1.5, 3, 10];
        for (int i = 0; i < limits.Length; i++)
        {
            root.Children.Add(At(new Polyline
            {
                Points = Points(0, 60, 20, 0, 40, 60), Stroke = Ink, StrokeThickness = 8,
                StrokeLineJoin = PenLineJoin.Miter, StrokeMiterLimit = limits[i],
            }, 20 + i * 80, 150));
        }
        root.Children.Add(At(new Polygon { Points = Points(0, 0, 70, 20, 10, 60), Stroke = Ink, StrokeThickness = 6, StrokeMiterLimit = 2 }, 260, 30));
        return new CoveragePage { Root = root };
    }

    // Gradient strokes on Rectangle, Ellipse, Path, Line and Border (brush mapped to the shape's bounds).
    [CoveragePage("F1-shapes", "gradient-strokes", MaxMeanAbsDiff = 0.01)]
    public static CoveragePage GradientStrokes(CoverageContext context)
    {
        var root = Surface();
        root.Children.Add(At(new Rectangle { Width = 120, Height = 70, Stroke = Gradient(new(0, 0), new(1, 0)), StrokeThickness = 8 }, 16, 16));
        root.Children.Add(At(new Ellipse { Width = 120, Height = 80, Stroke = Gradient(new(0, 0), new(0, 1)), StrokeThickness = 10 }, 180, 12));
        root.Children.Add(At(new Microsoft.UI.Xaml.Shapes.Path
        {
            Data = new PathGeometry
            {
                Figures =
                {
                    new PathFigure
                    {
                        StartPoint = new(0, 60),
                        Segments = { new BezierSegment { Point1 = new(30, -30), Point2 = new(90, 120), Point3 = new(130, 20) } },
                    },
                },
            },
            Stroke = Gradient(new(0, 0), new(1, 1)), StrokeThickness = 7, StrokeStartLineCap = PenLineCap.Round, StrokeEndLineCap = PenLineCap.Round,
        }, 16, 120));
        root.Children.Add(At(new Line { X1 = 0, Y1 = 0, X2 = 140, Y2 = 0, Stroke = Gradient(new(0, 0), new(1, 0)), StrokeThickness = 6, StrokeDashArray = Dashes(2, 1) }, 180, 120));
        root.Children.Add(At(new Border { Width = 140, Height = 70, BorderThickness = new Thickness(6), CornerRadius = new CornerRadius(12), BorderBrush = Gradient(new(0, 0), new(1, 1)) }, 180, 150));
        return new CoveragePage { Root = root };
    }

    // The Glyphs element: glyph indices with explicit advances and offsets, and a gradient fill.
    [CoveragePage("F1-shapes", "glyphs", MaxMeanAbsDiff = 0.01)]
    public static CoveragePage GlyphsElement(CoverageContext context)
    {
        var root = Surface();
        // Glyphs.FontUri resolves through the app resource manager (ms-appx, physical-file fallback), so use a copy of a
        // system font next to the app instead of shipping a font file.
        string font = "ms-appx:///CoverageGlyphs.ttf";
        string copy = System.IO.Path.Combine(AppContext.BaseDirectory, "CoverageGlyphs.ttf");
        if (!System.IO.File.Exists(copy))
        {
            System.IO.File.Copy(System.IO.Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.Fonts), "arial.ttf"), copy);
        }
        root.Children.Add(At(new Glyphs
        {
            FontUri = new Uri(font), FontRenderingEmSize = 28, UnicodeString = "Glyphs element", Fill = Ink, OriginX = 0, OriginY = 30,
        }, 16, 16));
        root.Children.Add(At(new Glyphs
        {
            FontUri = new Uri(font), FontRenderingEmSize = 36, UnicodeString = "Gradient", OriginX = 0, OriginY = 40,
            Indices = ",30;,25;,20;,25;,20;,22;,22;", Fill = Gradient(new(0, 0), new(1, 0)),
        }, 16, 80));
        root.Children.Add(At(new Glyphs
        {
            FontUri = new Uri(font), FontRenderingEmSize = 22, UnicodeString = "StyleSimulations", OriginX = 0, OriginY = 26,
            StyleSimulations = StyleSimulations.BoldItalicSimulation, Fill = Ink,
        }, 16, 160));
        return new CoveragePage { Root = root };
    }
}
