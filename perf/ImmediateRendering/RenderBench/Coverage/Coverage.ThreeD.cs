using System.Numerics;
using Microsoft.UI.Composition;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Hosting;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Media.Media3D;

namespace RenderBench.Coverage;

// F4 3D (test plan, "XAML coverage"): Projection, Transform3D, 3D facades and hand-off visual transforms. Content that
// is not affine on screen is drawn into an offscreen layer and composited through the 4x4 (D2D: perspective bitmap
// draw; GDI+: a mesh of affine cells).
public static class ThreeDCoverage
{
    static Grid Centered(FrameworkElement content)
    {
        content.HorizontalAlignment = HorizontalAlignment.Center;
        content.VerticalAlignment = VerticalAlignment.Center;
        return ClipsCoverage.Stage(content);
    }

    static CoveragePage PlaneProjection(double rotationY)
    {
        var content = ClipsCoverage.Quadrants(240, 180);
        content.Projection = new PlaneProjection { RotationY = rotationY };
        return new CoveragePage { Root = Centered(content) };
    }

    [CoveragePage("F4-3d", "plane-projection-y30", MaxMeanAbsDiff = 0.005)]
    public static CoveragePage PlaneProjectionY30(CoverageContext context) => PlaneProjection(30);

    [CoveragePage("F4-3d", "plane-projection-y60", MaxMeanAbsDiff = 0.005)]
    public static CoveragePage PlaneProjectionY60(CoverageContext context) => PlaneProjection(60);

    [CoveragePage("F4-3d", "plane-projection-xyz-offsets", MaxMeanAbsDiff = 0.005)]
    public static CoveragePage PlaneProjectionXyz(CoverageContext context)
    {
        var content = ClipsCoverage.Quadrants(240, 180);
        content.Projection = new PlaneProjection
        {
            RotationX = 25, RotationY = -35, RotationZ = 10, LocalOffsetZ = -40, GlobalOffsetX = 20, CenterOfRotationX = 0.3,
        };
        return new CoveragePage { Root = Centered(content) };
    }

    [CoveragePage("F4-3d", "matrix3d-projection", MaxMeanAbsDiff = 0.005)]
    public static CoveragePage Matrix3DProjection(CoverageContext context)
    {
        var content = ClipsCoverage.Quadrants(240, 180);
        content.Projection = new Matrix3DProjection
        {
            // Perspective along x (w = 1 + 0.0015 x) plus a shear and an offset, in element pixels.
            ProjectionMatrix = new Matrix3D
            {
                M11 = 1, M12 = 0.1, M13 = 0, M14 = 0.0015,
                M21 = 0, M22 = 1, M23 = 0, M24 = 0,
                M31 = 0, M32 = 0, M33 = 1, M34 = 0,
                OffsetX = 10, OffsetY = 0, OffsetZ = 0, M44 = 1,
            },
        };
        return new CoveragePage { Root = Centered(content) };
    }

    [CoveragePage("F4-3d", "perspective-transform3d-nested", MaxMeanAbsDiff = 0.005)]
    public static CoveragePage PerspectiveTransform3DNested(CoverageContext context)
    {
        var child = ClipsCoverage.Quadrants(160, 120);
        child.Transform3D = new CompositeTransform3D { RotationX = 25, CenterX = 80, CenterY = 60 };
        var card = new Grid { Width = 280, Height = 200, Background = new SolidColorBrush(Microsoft.UI.Colors.SlateGray) };
        card.Children.Add(child);
        card.Transform3D = new CompositeTransform3D { RotationY = 35, CenterX = 140, CenterY = 100, TranslateZ = -50 };
        var scene = new Grid { Width = 480, Height = 320 };
        scene.Transform3D = new PerspectiveTransform3D { Depth = 600 };
        scene.Children.Add(card);
        return new CoveragePage { Root = ClipsCoverage.Stage(scene) };
    }

    [CoveragePage("F4-3d", "facades-2d", MaxMeanAbsDiff = 0.005)]
    public static CoveragePage Facades2D(CoverageContext context)
    {
        var content = ClipsCoverage.Quadrants(200, 150);
        content.CenterPoint = new Vector3(100, 75, 0);
        content.Rotation = 20;
        content.Scale = new Vector3(1.2f, 0.8f, 1);
        content.Translation = new Vector3(10, -5, 0);
        return new CoveragePage { Root = Centered(content) };
    }

    [CoveragePage("F4-3d", "facades-rotation-axis-y", MaxMeanAbsDiff = 0.005)]
    public static CoveragePage FacadesRotationAxisY(CoverageContext context)
    {
        // Without a perspective the 3D rotation is orthographic on screen (affine): drawn without a layer.
        var content = ClipsCoverage.Quadrants(200, 150);
        content.CenterPoint = new Vector3(100, 75, 0);
        content.RotationAxis = new Vector3(0, 1, 0);
        content.Rotation = 50;
        return new CoveragePage { Root = Centered(content) };
    }

    [CoveragePage("F4-3d", "facade-transform-matrix-perspective", MaxMeanAbsDiff = 0.005)]
    public static CoveragePage FacadeTransformMatrix(CoverageContext context)
    {
        var content = ClipsCoverage.Quadrants(200, 150);
        content.TransformMatrix = new Matrix4x4(
            1, 0.1f, 0, 0.001f,
            0, 1, 0, 0.0008f,
            0, 0, 1, 0,
            20, 10, 0, 1);
        return new CoveragePage { Root = Centered(content) };
    }

    // Static hand-off visual transforms (ElementCompositionPreview.GetElementVisual). The element sits at layout offset 0
    // inside a placed holder: Composition mode writes the layout offset to the same visual's Offset.
    static CoveragePage HandOff(Action<Visual> configure)
    {
        var content = ClipsCoverage.Quadrants(200, 150);
        var holder = new Grid { Width = 200, Height = 150, HorizontalAlignment = HorizontalAlignment.Center, VerticalAlignment = VerticalAlignment.Center };
        holder.Children.Add(content);
        configure(ElementCompositionPreview.GetElementVisual(content));
        return new CoveragePage { Root = ClipsCoverage.Stage(holder) };
    }

    [CoveragePage("F4-3d", "handoff-offset-scale-rotation", MaxMeanAbsDiff = 0.005)]
    public static CoveragePage HandOffOffsetScaleRotation(CoverageContext context) => HandOff(visual =>
    {
        visual.Offset = new Vector3(30, -20, 0);
        visual.CenterPoint = new Vector3(100, 75, 0);
        visual.Scale = new Vector3(1.1f, 0.9f, 1);
        visual.RotationAngleInDegrees = 15;
    });

    [CoveragePage("F4-3d", "handoff-opacity-rotation-axis", MaxMeanAbsDiff = 0.005)]
    public static CoveragePage HandOffOpacityRotationAxis(CoverageContext context) => HandOff(visual =>
    {
        // RotationAxis X without a perspective: orthographic (affine) on screen. Composition mode writes the visual's
        // TransformMatrix from XAML, so the app-settable 3D properties are RotationAxis/Orientation/Scale.
        visual.Opacity = 0.6f;
        visual.CenterPoint = new Vector3(100, 75, 0);
        visual.RotationAxis = new Vector3(1, 0, 0);
        visual.RotationAngleInDegrees = 50;
    });
}
