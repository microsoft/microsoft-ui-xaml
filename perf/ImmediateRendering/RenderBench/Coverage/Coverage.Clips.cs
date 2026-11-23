using System.Numerics;
using Microsoft.UI;
using Microsoft.UI.Composition;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Hosting;
using Microsoft.UI.Xaml.Media;
using Windows.Foundation;
using Windows.UI;

namespace RenderBench.Coverage;

// F3 clips and transforms (test plan, "XAML coverage"): clips under arbitrary 2D transforms and non-rectangular clips
// are antialiased geometry clips in every immediate backend.
public static class ClipsCoverage
{
    // Four solid quadrants: rotations, skews and clips are visible in a capture, and solid fills keep diffs tight.
    internal static Grid Quadrants(double width, double height)
    {
        var grid = new Grid { Width = width, Height = height };
        grid.RowDefinitions.Add(new RowDefinition());
        grid.RowDefinitions.Add(new RowDefinition());
        grid.ColumnDefinitions.Add(new ColumnDefinition());
        grid.ColumnDefinitions.Add(new ColumnDefinition());
        Color[] colors = [Color.FromArgb(255, 0, 120, 212), Color.FromArgb(255, 232, 17, 35), Color.FromArgb(255, 16, 124, 16), Color.FromArgb(255, 255, 185, 0)];
        for (int i = 0; i < 4; i++)
        {
            var cell = new Border { Background = new SolidColorBrush(colors[i]) };
            Grid.SetRow(cell, i / 2);
            Grid.SetColumn(cell, i % 2);
            grid.Children.Add(cell);
        }
        return grid;
    }

    internal static Grid Stage(UIElement content)
    {
        var root = new Grid { Width = 480, Height = 320, Background = new SolidColorBrush(Colors.White) };
        root.Children.Add(content);
        return root;
    }

    static FrameworkElement Place(FrameworkElement element, double left, double top)
    {
        element.HorizontalAlignment = HorizontalAlignment.Left;
        element.VerticalAlignment = VerticalAlignment.Top;
        element.Margin = new Thickness(left, top, 0, 0);
        return element;
    }

    [CoveragePage("F3-clips", "rotated-rect-clip", MaxMeanAbsDiff = 0.005)]
    public static CoveragePage RotatedRectClip(CoverageContext context)
    {
        var content = Place(Quadrants(240, 200), 120, 60);
        content.Clip = new RectangleGeometry
        {
            Rect = new Rect(30, 30, 180, 140),
            Transform = new RotateTransform { Angle = 25, CenterX = 120, CenterY = 100 },
        };
        return new CoveragePage { Root = Stage(content) };
    }

    [CoveragePage("F3-clips", "rotated-clip-on-rotated-parent", MaxMeanAbsDiff = 0.005)]
    public static CoveragePage RotatedClipOnRotatedParent(CoverageContext context)
    {
        var content = Quadrants(200, 160);
        content.Clip = new RectangleGeometry
        {
            Rect = new Rect(20, 20, 160, 120),
            Transform = new RotateTransform { Angle = -10, CenterX = 100, CenterY = 80 },
        };
        var parent = Place(new Grid { Width = 200, Height = 160 }, 140, 80);
        parent.RenderTransform = new RotateTransform { Angle = 30, CenterX = 100, CenterY = 80 };
        ((Grid)parent).Children.Add(content);
        return new CoveragePage { Root = Stage(parent) };
    }

    [CoveragePage("F3-clips", "nested-clips-under-skew", MaxMeanAbsDiff = 0.005)]
    public static CoveragePage NestedClipsUnderSkew(CoverageContext context)
    {
        var inner = Quadrants(200, 160);
        inner.Clip = new RectangleGeometry { Rect = new Rect(40, 10, 150, 140), Transform = new ScaleTransform { ScaleX = 0.9, ScaleY = 1.1 } };
        var outer = new Grid { Width = 220, Height = 180, Background = new SolidColorBrush(Color.FromArgb(255, 96, 96, 96)), Padding = new Thickness(10) };
        outer.Clip = new RectangleGeometry { Rect = new Rect(0, 20, 200, 140) };
        outer.Children.Add(inner);
        var skewed = Place(new Grid(), 110, 60);
        skewed.RenderTransform = new SkewTransform { AngleX = 20, AngleY = 8 };
        ((Grid)skewed).Children.Add(outer);
        return new CoveragePage { Root = Stage(skewed) };
    }

    [CoveragePage("F3-clips", "layout-clip-rotated", MaxMeanAbsDiff = 0.005)]
    public static CoveragePage LayoutClipRotated(CoverageContext context)
    {
        // A child larger than its slot gets a layout clip (Grid arranges it at 140x100).
        var slot = new Grid { Width = 140, Height = 100 };
        slot.Children.Add(new Grid { Children = { Quadrants(240, 200) }, Width = 240, Height = 200 });
        var parent = Place(new Grid { Children = { slot } }, 160, 100);
        parent.RenderTransform = new RotateTransform { Angle = 35, CenterX = 70, CenterY = 50 };
        return new CoveragePage { Root = Stage(parent) };
    }

    // Composition clips on the element's hand-off visual (ElementCompositionPreview.GetElementVisual). The element sits
    // at layout offset 0 (Composition mode writes the layout offset to the same visual).
    static CoveragePage HandOffClip(Func<Compositor, CompositionClip> createClip, double margin = 0)
    {
        var content = Quadrants(260, 200);
        var holder = new Grid { Width = 260, Height = 200, Margin = new Thickness(margin), HorizontalAlignment = HorizontalAlignment.Center, VerticalAlignment = VerticalAlignment.Center };
        holder.Children.Add(content);
        Visual visual = ElementCompositionPreview.GetElementVisual(content);
        visual.Clip = createClip(visual.Compositor);
        return new CoveragePage { Root = Stage(holder) };
    }

    [CoveragePage("F3-clips", "handoff-ellipse-clip", MaxMeanAbsDiff = 0.005)]
    public static CoveragePage HandOffEllipseClip(CoverageContext context) => HandOffClip(compositor =>
    {
        var ellipse = compositor.CreateEllipseGeometry();
        ellipse.Center = new Vector2(130, 100);
        ellipse.Radius = new Vector2(110, 80);
        return compositor.CreateGeometricClip(ellipse);
    });

    [CoveragePage("F3-clips", "handoff-rounded-rect-clip", MaxMeanAbsDiff = 0.005)]
    public static CoveragePage HandOffRoundedRectClip(CoverageContext context) => HandOffClip(compositor =>
    {
        var rounded = compositor.CreateRoundedRectangleGeometry();
        rounded.Offset = new Vector2(20, 20);
        rounded.Size = new Vector2(220, 160);
        rounded.CornerRadius = new Vector2(40, 30);
        return compositor.CreateGeometricClip(rounded);
    });

    [CoveragePage("F3-clips", "handoff-rectangle-clip", MaxMeanAbsDiff = 0.005)]
    public static CoveragePage HandOffRectangleClip(CoverageContext context) => HandOffClip(compositor =>
    {
        RectangleClip clip = compositor.CreateRectangleClip(30, 20, 230, 180);
        clip.TopLeftRadius = new Vector2(24, 24);
        clip.BottomRightRadius = new Vector2(48, 32);
        clip.RotationAngleInDegrees = 10;
        clip.CenterPoint = new Vector2(130, 100);
        return clip;
    });

    // A path geometry clip cannot be read back from Composition: the window falls back.
    [CoveragePage("F3-clips", "handoff-path-clip", ExpectFallback = true, ExpectedUnsupported = new[] { "Hand-off visual clip" })]
    public static CoveragePage HandOffPathClip(CoverageContext context) => HandOffClip(compositor =>
    {
        var path = compositor.CreatePathGeometry();
        return compositor.CreateGeometricClip(path);
    });
}
