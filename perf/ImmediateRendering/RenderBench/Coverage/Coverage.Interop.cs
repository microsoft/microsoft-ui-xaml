using Microsoft.UI;
using System.Numerics;
using Microsoft.UI.Composition;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Hosting;
using Microsoft.UI.Xaml.Media;
using Windows.Graphics.Imaging;
using Windows.UI;

namespace RenderBench.Coverage;

// F11 interop (test plan, "XAML coverage"). Every page puts one kind of app Composition content into an otherwise plain
// page: the window must fall back to Composition on the first frame that contains it (no immediate frame with the content
// missing), and the fallen-back window must then match the Comp rendering. Content is static (or animations finish within
// the exercise) so the capture is deterministic.
static class InteropCoverage
{
    const string Family = "F11-interop";

    static Grid PageRoot(UIElement content) => new()
    {
        Background = new SolidColorBrush(Colors.White),
        Children =
        {
            new StackPanel
            {
                Spacing = 12,
                Padding = new Thickness(16),
                Children =
                {
                    new TextBlock { Text = "XAML text next to interop content", FontSize = 16 },
                    new Border { Width = 160, Height = 40, CornerRadius = new CornerRadius(6), Background = new SolidColorBrush(Colors.SteelBlue) },
                    content,
                },
            },
        },
    };

    static Compositor CompositorFor(UIElement element) => ElementCompositionPreview.GetElementVisual(element).Compositor;

    // SwapChainPanel with a D3D11 composition swap chain cleared once to a fixed color.
    [CoveragePage(Family, "swap-chain-panel", ExpectFallback = true)]
    public static CoveragePage SwapChain(CoverageContext context)
    {
        var panel = new SwapChainPanel { Width = 200, Height = 120 };
        D3DSwapChain? chain = null;
        return new CoveragePage
        {
            Root = PageRoot(panel),
            Exercise = async () =>
            {
                double scale = panel.XamlRoot.RasterizationScale;
                chain = new D3DSwapChain((int)Math.Ceiling(200 * scale), (int)Math.Ceiling(120 * scale));
                chain.Attach(panel);
                chain.Present(0.9f, 0.4f, 0.1f);
                await Task.Delay(300);
            },
            Detach = () => chain?.Dispose(),
        };
    }

    // MediaPlayerElement (MediaPlayerPresenter) without a source: the presenter alone is media interop.
    [CoveragePage(Family, "media-player-element", ExpectFallback = true)]
    public static CoveragePage Media(CoverageContext context) => new()
    {
        Root = PageRoot(new MediaPlayerElement { Width = 200, Height = 120, AreTransportControlsEnabled = false }),
        Exercise = () => Task.Delay(300),
    };

    // ElementCompositionPreview.SetElementChildVisual with a static SpriteVisual.
    [CoveragePage(Family, "element-child-visual", ExpectFallback = true)]
    public static CoveragePage ChildVisual(CoverageContext context)
    {
        var host = new Border { Width = 200, Height = 120, Background = new SolidColorBrush(Colors.LightGray) };
        return new CoveragePage
        {
            Root = PageRoot(host),
            Exercise = async () =>
            {
                var compositor = CompositorFor(host);
                var sprite = compositor.CreateSpriteVisual();
                sprite.Size = new Vector2(120, 60);
                sprite.Offset = new Vector3(40, 30, 0);
                sprite.Brush = compositor.CreateColorBrush(Color.FromArgb(255, 0x8E, 0x44, 0xAD));
                ElementCompositionPreview.SetElementChildVisual(host, sprite);
                await Task.Delay(300);
            },
        };
    }

    // App UIElement.StartAnimation on a facade (Translation), short enough to finish before the capture.
    [CoveragePage(Family, "facade-start-animation", ExpectFallback = true)]
    public static CoveragePage FacadeAnimation(CoverageContext context)
    {
        var target = new Border { Width = 120, Height = 60, Background = new SolidColorBrush(Colors.Crimson) };
        return new CoveragePage
        {
            Root = PageRoot(target),
            Exercise = async () =>
            {
                var animation = CompositorFor(target).CreateVector3KeyFrameAnimation();
                animation.Target = "Translation";
                animation.InsertKeyFrame(1, new Vector3(40, 10, 0));
                animation.Duration = TimeSpan.FromMilliseconds(50);
                target.StartAnimation(animation);
                await Task.Delay(400);
            },
        };
    }

    // App Composition animation on the element's hand-off visual (ElementCompositionPreview.GetElementVisual), short enough
    // to finish before the capture. Static hand-off properties are painted immediately (F4 handoff-* pages, clips3d).
    [CoveragePage(Family, "hand-off-visual", ExpectFallback = true)]
    public static CoveragePage HandOff(CoverageContext context)
    {
        var target = new Border { Width = 120, Height = 60, Background = new SolidColorBrush(Colors.SeaGreen) };
        return new CoveragePage
        {
            Root = PageRoot(target),
            Exercise = async () =>
            {
                var visual = ElementCompositionPreview.GetElementVisual(target);
                visual.CenterPoint = new Vector3(60, 30, 0);
                var animation = visual.Compositor.CreateScalarKeyFrameAnimation();
                animation.InsertKeyFrame(1, 12);
                animation.Duration = TimeSpan.FromMilliseconds(50);
                visual.StartAnimation("RotationAngleInDegrees", animation);
                await Task.Delay(400);
            },
        };
    }

    // ElementCompositionPreview.SetImplicitShowAnimation on an element made visible during the exercise.
    [CoveragePage(Family, "implicit-show-animation", ExpectFallback = true)]
    public static CoveragePage ImplicitShow(CoverageContext context)
    {
        var target = new Border { Width = 120, Height = 60, Background = new SolidColorBrush(Colors.DarkOrange), Visibility = Visibility.Collapsed };
        return new CoveragePage
        {
            Root = PageRoot(target),
            Exercise = async () =>
            {
                var animation = CompositorFor(target).CreateScalarKeyFrameAnimation();
                animation.Target = "Opacity";
                animation.InsertKeyFrame(0, 0);
                animation.InsertKeyFrame(1, 1);
                animation.Duration = TimeSpan.FromMilliseconds(50);
                ElementCompositionPreview.SetImplicitShowAnimation(target, animation);
                target.Visibility = Visibility.Visible;
                await Task.Delay(400);
            },
        };
    }

    // AnimatedVisualPlayer with an app IAnimatedVisualSource (a static SpriteVisual, no Lottie).
    [CoveragePage(Family, "app-animated-visual-player", ExpectFallback = true)]
    public static CoveragePage AppAnimatedVisual(CoverageContext context) => new()
    {
        Root = PageRoot(new AnimatedVisualPlayer { Width = 120, Height = 120, AutoPlay = false, Source = new SquareVisualSource() }),
        Exercise = () => Task.Delay(400),
    };

    // App XamlCompositionBrushBase (a CompositionColorBrush) on a Border and a Rectangle.
    [CoveragePage(Family, "app-composition-brush", ExpectFallback = true)]
    public static CoveragePage AppBrush(CoverageContext context) => new()
    {
        Root = PageRoot(new StackPanel
        {
            Orientation = Orientation.Horizontal,
            Spacing = 12,
            Children =
            {
                new Border { Width = 120, Height = 60, Background = new ColorCompositionBrush(Color.FromArgb(255, 0x2E, 0x86, 0xDE)) },
                new Microsoft.UI.Xaml.Shapes.Rectangle { Width = 120, Height = 60, Fill = new ColorCompositionBrush(Color.FromArgb(255, 0xF3, 0x9C, 0x12)) },
            },
        }),
        Exercise = () => Task.Delay(300),
    };

    // App XamlLight (Composition AmbientLight) on a panel with colored children.
    [CoveragePage(Family, "app-xaml-light", ExpectFallback = true)]
    public static CoveragePage AppLight(CoverageContext context)
    {
        var lit = new StackPanel
        {
            Orientation = Orientation.Horizontal,
            Spacing = 12,
            Children =
            {
                new Border { Width = 120, Height = 60, Background = new SolidColorBrush(Color.FromArgb(255, 0x2E, 0x86, 0xDE)) },
                new Border { Width = 120, Height = 60, Background = new SolidColorBrush(Color.FromArgb(255, 0xF3, 0x9C, 0x12)) },
            },
        };
        lit.Lights.Add(new DimAmbientLight());
        return new CoveragePage { Root = PageRoot(lit), Exercise = () => Task.Delay(300) };
    }

    // LoadedImageSurface in a CompositionSurfaceBrush, through an app XamlCompositionBrushBase.
    [CoveragePage(Family, "loaded-image-surface", ExpectFallback = true)]
    public static CoveragePage LoadedSurface(CoverageContext context)
    {
        var target = new Border { Width = 128, Height = 128 };
        return new CoveragePage
        {
            Root = PageRoot(target),
            Exercise = async () =>
            {
                var stream = await ImagesCoverage.EncodeAsync(BitmapEncoder.PngEncoderId, 64, 64, ImagesCoverage.Pattern(64, 64, context.Seed));
                var surface = LoadedImageSurface.StartLoadFromStream(stream);
                var loaded = new TaskCompletionSource();
                surface.LoadCompleted += (_, _) => loaded.TrySetResult();
                target.Background = new SurfaceCompositionBrush(surface);
                await Task.WhenAny(loaded.Task, Task.Delay(2000));
                await Task.Delay(200);
            },
        };
    }

    sealed partial class ColorCompositionBrush(Color color) : XamlCompositionBrushBase
    {
        protected override void OnConnected() => CompositionBrush ??= CompositionTarget.GetCompositorForCurrentThread().CreateColorBrush(color);

        protected override void OnDisconnected()
        {
            CompositionBrush?.Dispose();
            CompositionBrush = null;
        }
    }

    // App XamlLight: a dim ambient Composition light on its targets.
    sealed partial class DimAmbientLight : XamlLight
    {
        protected override void OnConnected(UIElement newElement)
        {
            var light = CompositionTarget.GetCompositorForCurrentThread().CreateAmbientLight();
            light.Color = Color.FromArgb(255, 0x60, 0x60, 0x60);
            CompositionLight = light;
            AddTargetElement(GetId(), newElement);
        }

        protected override void OnDisconnected(UIElement oldElement)
        {
            RemoveTargetElement(GetId(), oldElement);
            CompositionLight?.Dispose();
            CompositionLight = null;
        }

        protected override string GetId() => typeof(DimAmbientLight).FullName!;
    }

    sealed partial class SurfaceCompositionBrush(LoadedImageSurface surface) : XamlCompositionBrushBase
    {
        protected override void OnConnected()
        {
            var brush = CompositionTarget.GetCompositorForCurrentThread().CreateSurfaceBrush(surface);
            brush.Stretch = CompositionStretch.Fill;
            CompositionBrush ??= brush;
        }

        protected override void OnDisconnected()
        {
            CompositionBrush?.Dispose();
            CompositionBrush = null;
        }
    }

    sealed partial class SquareVisualSource : IAnimatedVisualSource
    {
        public IAnimatedVisual TryCreateAnimatedVisual(Compositor compositor, out object diagnostics)
        {
            diagnostics = null!;
            var sprite = compositor.CreateSpriteVisual();
            sprite.Size = new Vector2(100, 100);
            sprite.Brush = compositor.CreateColorBrush(Colors.MediumPurple);
            return new SquareVisual(sprite);
        }
    }

    sealed partial class SquareVisual(Visual root) : IAnimatedVisual
    {
        public TimeSpan Duration => TimeSpan.FromSeconds(1);
        public Visual RootVisual => root;
        public Vector2 Size => new(100, 100);
        public void Dispose() => root.Dispose();
    }
}
