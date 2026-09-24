using System;
using System.Numerics;
using Microsoft.UI;
using Microsoft.UI.Composition;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Hosting;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Media.Animation;
using Windows.UI;

namespace FolderTreeApp
{
    public sealed partial class AnimationWindow : Window
    {
        private Compositor _compositor;
        private ContainerVisual _scene;

        public AnimationWindow()
        {
            this.InitializeComponent();
            Title = "Composition Animations";
        }

        // Everything is hand-built Composition: a dark animated gradient backdrop, an orbit of
        // sprites, pulsing rings, and a row of bouncing tiles. All driven by infinite keyframe
        // animations so the scene is alive the moment the window opens. A single ContainerVisual
        // (_scene) parents every visual, since SetElementChildVisual only takes one root.
        private void RootGrid_Loaded(object sender, RoutedEventArgs e)
        {
            _compositor = ElementCompositionPreview.GetElementVisual(RootGrid).Compositor;

            _scene = _compositor.CreateContainerVisual();
            _scene.RelativeSizeAdjustment = Vector2.One;
            ElementCompositionPreview.SetElementChildVisual(VisualHost, _scene);

            BuildBackdrop();
            BuildPulsingRings();
            BuildOrbit();
            BuildBouncingTiles();

            // XAML UIElements driven by Storyboards (DoubleAnimation / ColorAnimation).
            BuildXamlStoryboards();
        }

        // A full-window vertical gradient. Kept very light so the window reads as white while
        // still demonstrating an animated Composition gradient brush.
        private void BuildBackdrop()
        {
            var sprite = _compositor.CreateSpriteVisual();
            sprite.RelativeSizeAdjustment = Vector2.One;
            sprite.Comment = "Animation.Backdrop";

            var brush = _compositor.CreateLinearGradientBrush();
            brush.StartPoint = new Vector2(0, 0);
            brush.EndPoint = new Vector2(1, 1);
            var top = _compositor.CreateColorGradientStop(0f, Color.FromArgb(255, 255, 255, 255));
            var bottom = _compositor.CreateColorGradientStop(1f, Color.FromArgb(255, 244, 240, 255));
            brush.ColorStops.Add(top);
            brush.ColorStops.Add(bottom);
            sprite.Brush = brush;
            _scene.Children.InsertAtBottom(sprite);

            var sweep = _compositor.CreateColorKeyFrameAnimation();
            sweep.InsertKeyFrame(0f, Color.FromArgb(255, 244, 240, 255));
            sweep.InsertKeyFrame(0.5f, Color.FromArgb(255, 235, 248, 255));
            sweep.InsertKeyFrame(1f, Color.FromArgb(255, 244, 240, 255));
            sweep.Duration = TimeSpan.FromSeconds(8);
            sweep.IterationBehavior = AnimationIterationBehavior.Forever;
            bottom.StartAnimation("Color", sweep);
        }

        // Concentric rings that pulse in scale and fade — a soft radar effect.
        private void BuildPulsingRings()
        {
            for (int i = 0; i < 3; i++)
            {
                var visual = _compositor.CreateShapeVisual();
                visual.Size = new Vector2(360, 360);
                visual.Offset = new Vector3(140, 230, 0);
                visual.CenterPoint = new Vector3(180, 180, 0);
                visual.Comment = $"Animation.Ring{i}";

                var geo = _compositor.CreateEllipseGeometry();
                geo.Radius = new Vector2(120, 120);
                geo.Center = new Vector2(180, 180);
                var shape = _compositor.CreateSpriteShape(geo);
                shape.StrokeBrush = _compositor.CreateColorBrush(Colors.MediumSlateBlue);
                shape.StrokeThickness = 3;
                visual.Shapes.Add(shape);
                _scene.Children.InsertAtTop(visual);

                var scale = _compositor.CreateVector3KeyFrameAnimation();
                scale.InsertKeyFrame(0f, new Vector3(0.4f, 0.4f, 1f));
                scale.InsertKeyFrame(1f, new Vector3(1.4f, 1.4f, 1f));
                scale.Duration = TimeSpan.FromSeconds(3);
                scale.DelayTime = TimeSpan.FromSeconds(i);
                scale.IterationBehavior = AnimationIterationBehavior.Forever;
                visual.StartAnimation("Scale", scale);

                var fade = _compositor.CreateScalarKeyFrameAnimation();
                fade.InsertKeyFrame(0f, 0.8f);
                fade.InsertKeyFrame(1f, 0f);
                fade.Duration = TimeSpan.FromSeconds(3);
                fade.DelayTime = TimeSpan.FromSeconds(i);
                fade.IterationBehavior = AnimationIterationBehavior.Forever;
                visual.StartAnimation("Opacity", fade);
            }
        }

        // Sprites orbiting a center point, each phase-offset, all parented to a slowly rotating
        // container so the whole cluster spins too.
        private void BuildOrbit()
        {
            var center = _compositor.CreateContainerVisual();
            center.Offset = new Vector3(320, 410, 0);
            center.Comment = "Animation.Orbit";
            _scene.Children.InsertAtTop(center);

            var colors = new[] { Colors.Tomato, Colors.Gold, Colors.MediumSeaGreen, Colors.DeepSkyBlue, Colors.Violet };
            for (int i = 0; i < colors.Length; i++)
            {
                var sprite = _compositor.CreateSpriteVisual();
                sprite.Size = new Vector2(28, 28);
                sprite.CenterPoint = new Vector3(14, 14, 0);
                sprite.Brush = _compositor.CreateColorBrush(colors[i]);
                sprite.Comment = $"Animation.OrbitSprite{i}";
                center.Children.InsertAtTop(sprite);

                var orbit = _compositor.CreateVector3KeyFrameAnimation();
                float angle = i * (360f / colors.Length);
                orbit.InsertKeyFrame(0f, Offset(angle, 130));
                orbit.InsertKeyFrame(0.25f, Offset(angle + 90, 130));
                orbit.InsertKeyFrame(0.5f, Offset(angle + 180, 130));
                orbit.InsertKeyFrame(0.75f, Offset(angle + 270, 130));
                orbit.InsertKeyFrame(1f, Offset(angle + 360, 130));
                orbit.Duration = TimeSpan.FromSeconds(5);
                orbit.IterationBehavior = AnimationIterationBehavior.Forever;
                sprite.StartAnimation("Offset", orbit);
            }

            var spin = _compositor.CreateScalarKeyFrameAnimation();
            spin.InsertKeyFrame(1f, 360f, _compositor.CreateLinearEasingFunction());
            spin.Duration = TimeSpan.FromSeconds(20);
            spin.IterationBehavior = AnimationIterationBehavior.Forever;
            center.StartAnimation("RotationAngleInDegrees", spin);
        }

        private static Vector3 Offset(float degrees, float radius)
        {
            double r = degrees * Math.PI / 180.0;
            return new Vector3((float)(Math.Cos(r) * radius), (float)(Math.Sin(r) * radius), 0);
        }

        // A row of tiles that bounce vertically with eased keyframes, staggered for a wave look.
        private void BuildBouncingTiles()
        {
            var ease = _compositor.CreateCubicBezierEasingFunction(new Vector2(0.4f, 0f), new Vector2(0.2f, 1f));
            var colors = new[] { Colors.Coral, Colors.Khaki, Colors.SpringGreen, Colors.SkyBlue, Colors.Orchid, Colors.Salmon };
            for (int i = 0; i < colors.Length; i++)
            {
                var tile = _compositor.CreateSpriteVisual();
                tile.Size = new Vector2(40, 40);
                tile.Offset = new Vector3(120 + i * 60, 620, 0);
                tile.Brush = _compositor.CreateColorBrush(colors[i]);
                tile.Comment = $"Animation.Tile{i}";
                _scene.Children.InsertAtTop(tile);

                var bounce = _compositor.CreateScalarKeyFrameAnimation();
                bounce.InsertKeyFrame(0f, 620f);
                bounce.InsertKeyFrame(0.5f, 520f, ease);
                bounce.InsertKeyFrame(1f, 620f, ease);
                bounce.Duration = TimeSpan.FromSeconds(1.4);
                bounce.DelayTime = TimeSpan.FromMilliseconds(i * 120);
                bounce.IterationBehavior = AnimationIterationBehavior.Forever;
                tile.StartAnimation("Offset.Y", bounce);
            }
        }

        // ----- XAML Storyboard animation layer ---------------------------------------------
        // Drives the real UIElements declared in AnimationWindow.xaml using classic Storyboard
        // timelines (DoubleAnimation on CompositeTransform / Opacity, ColorAnimation on the
        // SolidColorBrush fills). All loop forever so the window is lively as soon as it opens.
        private void BuildXamlStoryboards()
        {
            // Spinning, pulsing, color-cycling orbs.
            StartRotation(Orb0T, 6);
            StartPulse(Orb0T, 1.0, 1.35, 1.6);
            StartColorCycle(Orb0Brush, Color.FromArgb(255, 0xEF, 0x53, 0x50), Color.FromArgb(255, 0xEC, 0x40, 0x7A), 3);

            StartRotation(Orb1T, -8);
            StartFloatY(Orb1T, 60, 2.2);
            StartOpacityBlink(Orb1, 0.45, 1.0, 1.8);
            StartColorCycle(Orb1Brush, Color.FromArgb(255, 0x42, 0xA5, 0xF5), Color.FromArgb(255, 0x26, 0xC6, 0xDA), 3.4);

            // A row of cards that bounce in a staggered wave, wobble, and cycle color.
            StartBounce(Card0T, -130, 1.3, 0.0);
            StartBounce(Card1T, -130, 1.3, 0.2);
            StartBounce(Card2T, -130, 1.3, 0.4);
            StartWobble(Card0T, 12, 1.5);
            StartWobble(Card1T, 12, 1.7);
            StartWobble(Card2T, 12, 1.9);
            StartColorCycle(Card0Brush, Color.FromArgb(255, 0x66, 0xBB, 0x6A), Color.FromArgb(255, 0x9C, 0xCC, 0x65), 3.2);
            StartColorCycle(Card1Brush, Color.FromArgb(255, 0xFF, 0xCA, 0x28), Color.FromArgb(255, 0xFF, 0xA7, 0x26), 3.6);
            StartColorCycle(Card2Brush, Color.FromArgb(255, 0xAB, 0x47, 0xBC), Color.FromArgb(255, 0x7E, 0x57, 0xC2), 4.0);

            // Twinkling, spinning star.
            StartRotation(Star0T, 10);
            StartPulse(Star0T, 0.8, 1.4, 1.0);

            // Gentle title breathing.
            StartOpacityBlink(TitleText, 0.65, 1.0, 2.6);
        }

        private static void Run(params Timeline[] animations)
        {
            var sb = new Storyboard();
            foreach (var a in animations)
                sb.Children.Add(a);
            sb.Begin();
        }

        private static DoubleAnimation MakeDouble(
            DependencyObject target, string property, double from, double to,
            double seconds, double beginSeconds = 0, bool autoReverse = false,
            EasingFunctionBase ease = null)
        {
            var a = new DoubleAnimation
            {
                From = from,
                To = to,
                Duration = new Duration(TimeSpan.FromSeconds(seconds)),
                BeginTime = TimeSpan.FromSeconds(beginSeconds),
                AutoReverse = autoReverse,
                RepeatBehavior = RepeatBehavior.Forever,
                EnableDependentAnimation = true,
                EasingFunction = ease,
            };
            Storyboard.SetTarget(a, target);
            Storyboard.SetTargetProperty(a, property);
            return a;
        }

        // period > 0 spins clockwise, < 0 counter-clockwise.
        private static void StartRotation(CompositeTransform t, double period)
        {
            double to = period < 0 ? -360 : 360;
            Run(MakeDouble(t, "Rotation", 0, to, Math.Abs(period)));
        }

        private static void StartPulse(CompositeTransform t, double min, double max, double seconds)
        {
            var ease = new QuadraticEase { EasingMode = EasingMode.EaseInOut };
            Run(
                MakeDouble(t, "ScaleX", min, max, seconds, 0, true, ease),
                MakeDouble(t, "ScaleY", min, max, seconds, 0, true, ease));
        }

        private static void StartFloatY(CompositeTransform t, double delta, double seconds)
        {
            Run(MakeDouble(t, "TranslateY", 0, delta, seconds, 0, true,
                new SineEase { EasingMode = EasingMode.EaseInOut }));
        }

        private static void StartBounce(CompositeTransform t, double delta, double seconds, double begin)
        {
            var ease = new BounceEase { Bounces = 2, Bounciness = 2, EasingMode = EasingMode.EaseOut };
            Run(MakeDouble(t, "TranslateY", 0, delta, seconds, begin, true, ease));
        }

        private static void StartWobble(CompositeTransform t, double degrees, double seconds)
        {
            Run(MakeDouble(t, "Rotation", -degrees, degrees, seconds, 0, true,
                new QuadraticEase { EasingMode = EasingMode.EaseInOut }));
        }

        private static void StartOpacityBlink(UIElement element, double min, double max, double seconds)
        {
            Run(MakeDouble(element, "Opacity", max, min, seconds, 0, true,
                new SineEase { EasingMode = EasingMode.EaseInOut }));
        }

        private static void StartColorCycle(SolidColorBrush brush, Color from, Color to, double seconds)
        {
            var anim = new ColorAnimation
            {
                From = from,
                To = to,
                Duration = new Duration(TimeSpan.FromSeconds(seconds)),
                AutoReverse = true,
                RepeatBehavior = RepeatBehavior.Forever,
                EnableDependentAnimation = true,
                EasingFunction = new SineEase { EasingMode = EasingMode.EaseInOut },
            };
            Storyboard.SetTarget(anim, brush);
            Storyboard.SetTargetProperty(anim, "Color");
            Run(anim);
        }
    }
}
