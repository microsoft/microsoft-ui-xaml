#include "pch.h"
#include "AnimationWindow.xaml.h"
#if __has_include("AnimationWindow.g.cpp")
#include "AnimationWindow.g.cpp"
#endif

#include <winrt/Windows.Foundation.Numerics.h>

using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Numerics;
using namespace Windows::UI;
using namespace Microsoft::UI;
using namespace Microsoft::UI::Composition;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Hosting;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Animation;

namespace winrt::FolderTreeAppCpp::implementation
{
    AnimationWindow::AnimationWindow()
    {
        InitializeComponent();
        Title(L"Composition Animations");
    }

    // Everything is hand-built Composition: a dark animated gradient backdrop, an orbit of
    // sprites, pulsing rings, and a row of bouncing tiles. All driven by infinite keyframe
    // animations so the scene is alive the moment the window opens. A single ContainerVisual
    // (_scene) parents every visual, since SetElementChildVisual only takes one root.
    void AnimationWindow::RootGrid_Loaded(IInspectable const&, RoutedEventArgs const&)
    {
        m_compositor = ElementCompositionPreview::GetElementVisual(RootGrid()).Compositor();

        m_scene = m_compositor.CreateContainerVisual();
        m_scene.RelativeSizeAdjustment({ 1.0f, 1.0f });
        ElementCompositionPreview::SetElementChildVisual(VisualHost(), m_scene);

        BuildBackdrop();
        BuildPulsingRings();
        BuildOrbit();
        BuildBouncingTiles();

        // XAML UIElements driven by Storyboards (DoubleAnimation / ColorAnimation).
        BuildXamlStoryboards();
    }

    // A full-window vertical gradient. Kept very light so the window reads as white while
    // still demonstrating an animated Composition gradient brush.
    void AnimationWindow::BuildBackdrop()
    {
        auto const sprite = m_compositor.CreateSpriteVisual();
        sprite.RelativeSizeAdjustment({ 1.0f, 1.0f });
        sprite.Comment(L"Animation.Backdrop");

        auto const brush = m_compositor.CreateLinearGradientBrush();
        brush.StartPoint({ 0.0f, 0.0f });
        brush.EndPoint({ 1.0f, 1.0f });
        auto const top = m_compositor.CreateColorGradientStop(0.0f, Color{ 255, 255, 255, 255 });
        auto const bottom = m_compositor.CreateColorGradientStop(1.0f, Color{ 255, 244, 240, 255 });
        brush.ColorStops().Append(top);
        brush.ColorStops().Append(bottom);
        sprite.Brush(brush);
        m_scene.Children().InsertAtBottom(sprite);

        auto const sweep = m_compositor.CreateColorKeyFrameAnimation();
        sweep.InsertKeyFrame(0.0f, Color{ 255, 244, 240, 255 });
        sweep.InsertKeyFrame(0.5f, Color{ 255, 235, 248, 255 });
        sweep.InsertKeyFrame(1.0f, Color{ 255, 244, 240, 255 });
        sweep.Duration(std::chrono::seconds{ 8 });
        sweep.IterationBehavior(AnimationIterationBehavior::Forever);
        bottom.StartAnimation(L"Color", sweep);
    }

    // Concentric rings that pulse in scale and fade — a soft radar effect.
    void AnimationWindow::BuildPulsingRings()
    {
        for (int32_t index = 0; index < 3; ++index)
        {
            auto const visual = m_compositor.CreateShapeVisual();
            visual.Size({ 360.0f, 360.0f });
            visual.Offset({ 140.0f, 230.0f, 0.0f });
            visual.CenterPoint({ 180.0f, 180.0f, 0.0f });
            visual.Comment(hstring{ std::wstring{ L"Animation.Ring" }.append(to_hstring(index)) });

            auto const geometry = m_compositor.CreateEllipseGeometry();
            geometry.Radius({ 120.0f, 120.0f });
            geometry.Center({ 180.0f, 180.0f });
            auto const shape = m_compositor.CreateSpriteShape(geometry);
            shape.StrokeBrush(m_compositor.CreateColorBrush(Microsoft::UI::Colors::MediumSlateBlue()));
            shape.StrokeThickness(3.0f);
            visual.Shapes().Append(shape);
            m_scene.Children().InsertAtTop(visual);

            auto const scale = m_compositor.CreateVector3KeyFrameAnimation();
            scale.InsertKeyFrame(0.0f, { 0.4f, 0.4f, 1.0f });
            scale.InsertKeyFrame(1.0f, { 1.4f, 1.4f, 1.0f });
            scale.Duration(std::chrono::seconds{ 3 });
            scale.DelayTime(std::chrono::seconds{ index });
            scale.IterationBehavior(AnimationIterationBehavior::Forever);
            visual.StartAnimation(L"Scale", scale);

            auto const fade = m_compositor.CreateScalarKeyFrameAnimation();
            fade.InsertKeyFrame(0.0f, 0.8f);
            fade.InsertKeyFrame(1.0f, 0.0f);
            fade.Duration(std::chrono::seconds{ 3 });
            fade.DelayTime(std::chrono::seconds{ index });
            fade.IterationBehavior(AnimationIterationBehavior::Forever);
            visual.StartAnimation(L"Opacity", fade);
        }
    }

    // Sprites orbiting a center point, each phase-offset, all parented to a slowly rotating
    // container so the whole cluster spins too.
    void AnimationWindow::BuildOrbit()
    {
        auto const center = m_compositor.CreateContainerVisual();
        center.Offset({ 320.0f, 410.0f, 0.0f });
        center.Comment(L"Animation.Orbit");
        m_scene.Children().InsertAtTop(center);

        std::array<Color, 5> const colors{
            Microsoft::UI::Colors::Tomato(),
            Microsoft::UI::Colors::Gold(),
            Microsoft::UI::Colors::MediumSeaGreen(),
            Microsoft::UI::Colors::DeepSkyBlue(),
            Microsoft::UI::Colors::Violet()
        };
        for (uint32_t index = 0; index < colors.size(); ++index)
        {
            auto const sprite = m_compositor.CreateSpriteVisual();
            sprite.Size({ 28.0f, 28.0f });
            sprite.CenterPoint({ 14.0f, 14.0f, 0.0f });
            sprite.Brush(m_compositor.CreateColorBrush(colors[index]));
            sprite.Comment(hstring{ std::wstring{ L"Animation.OrbitSprite" }.append(to_hstring(index)) });
            center.Children().InsertAtTop(sprite);

            auto const orbit = m_compositor.CreateVector3KeyFrameAnimation();
            float const angle = index * (360.0f / colors.size());
            orbit.InsertKeyFrame(0.0f, Offset(angle, 130.0f));
            orbit.InsertKeyFrame(0.25f, Offset(angle + 90.0f, 130.0f));
            orbit.InsertKeyFrame(0.5f, Offset(angle + 180.0f, 130.0f));
            orbit.InsertKeyFrame(0.75f, Offset(angle + 270.0f, 130.0f));
            orbit.InsertKeyFrame(1.0f, Offset(angle + 360.0f, 130.0f));
            orbit.Duration(std::chrono::seconds{ 5 });
            orbit.IterationBehavior(AnimationIterationBehavior::Forever);
            sprite.StartAnimation(L"Offset", orbit);
        }

        auto const spin = m_compositor.CreateScalarKeyFrameAnimation();
        spin.InsertKeyFrame(1.0f, 360.0f, m_compositor.CreateLinearEasingFunction());
        spin.Duration(std::chrono::seconds{ 20 });
        spin.IterationBehavior(AnimationIterationBehavior::Forever);
        center.StartAnimation(L"RotationAngleInDegrees", spin);
    }

    float3 AnimationWindow::Offset(float degrees, float radius)
    {
        constexpr double pi = 3.14159265358979323846;
        double const radians = degrees * pi / 180.0;
        return {
            static_cast<float>(std::cos(radians) * radius),
            static_cast<float>(std::sin(radians) * radius),
            0.0f
        };
    }

    // A row of tiles that bounce vertically with eased keyframes, staggered for a wave look.
    void AnimationWindow::BuildBouncingTiles()
    {
        auto const ease = m_compositor.CreateCubicBezierEasingFunction(
            { 0.4f, 0.0f },
            { 0.2f, 1.0f });
        std::array<Color, 6> const colors{
            Microsoft::UI::Colors::Coral(),
            Microsoft::UI::Colors::Khaki(),
            Microsoft::UI::Colors::SpringGreen(),
            Microsoft::UI::Colors::SkyBlue(),
            Microsoft::UI::Colors::Orchid(),
            Microsoft::UI::Colors::Salmon()
        };
        for (uint32_t index = 0; index < colors.size(); ++index)
        {
            auto const tile = m_compositor.CreateSpriteVisual();
            tile.Size({ 40.0f, 40.0f });
            tile.Offset({ 120.0f + index * 60.0f, 620.0f, 0.0f });
            tile.Brush(m_compositor.CreateColorBrush(colors[index]));
            tile.Comment(hstring{ std::wstring{ L"Animation.Tile" }.append(to_hstring(index)) });
            m_scene.Children().InsertAtTop(tile);

            auto const bounce = m_compositor.CreateScalarKeyFrameAnimation();
            bounce.InsertKeyFrame(0.0f, 620.0f);
            bounce.InsertKeyFrame(0.5f, 520.0f, ease);
            bounce.InsertKeyFrame(1.0f, 620.0f, ease);
            bounce.Duration(std::chrono::duration_cast<TimeSpan>(std::chrono::duration<double>{ 1.4 }));
            bounce.DelayTime(std::chrono::milliseconds{ index * 120 });
            bounce.IterationBehavior(AnimationIterationBehavior::Forever);
            tile.StartAnimation(L"Offset.Y", bounce);
        }
    }

    // ----- XAML Storyboard animation layer ---------------------------------------------
    // Drives the real UIElements declared in AnimationWindow.xaml using classic Storyboard
    // timelines (DoubleAnimation on CompositeTransform / Opacity, ColorAnimation on the
    // SolidColorBrush fills). All loop forever so the window is lively as soon as it opens.
    void AnimationWindow::BuildXamlStoryboards()
    {
        // Spinning, pulsing, color-cycling orbs.
        StartRotation(Orb0T(), 6);
        StartPulse(Orb0T(), 1.0, 1.35, 1.6);
        StartColorCycle(Orb0Brush(), Color{ 255, 0xEF, 0x53, 0x50 }, Color{ 255, 0xEC, 0x40, 0x7A }, 3);

        StartRotation(Orb1T(), -8);
        StartFloatY(Orb1T(), 60, 2.2);
        StartOpacityBlink(Orb1(), 0.45, 1.0, 1.8);
        StartColorCycle(Orb1Brush(), Color{ 255, 0x42, 0xA5, 0xF5 }, Color{ 255, 0x26, 0xC6, 0xDA }, 3.4);

        // A row of cards that bounce in a staggered wave, wobble, and cycle color.
        StartBounce(Card0T(), -130, 1.3, 0.0);
        StartBounce(Card1T(), -130, 1.3, 0.2);
        StartBounce(Card2T(), -130, 1.3, 0.4);
        StartWobble(Card0T(), 12, 1.5);
        StartWobble(Card1T(), 12, 1.7);
        StartWobble(Card2T(), 12, 1.9);
        StartColorCycle(Card0Brush(), Color{ 255, 0x66, 0xBB, 0x6A }, Color{ 255, 0x9C, 0xCC, 0x65 }, 3.2);
        StartColorCycle(Card1Brush(), Color{ 255, 0xFF, 0xCA, 0x28 }, Color{ 255, 0xFF, 0xA7, 0x26 }, 3.6);
        StartColorCycle(Card2Brush(), Color{ 255, 0xAB, 0x47, 0xBC }, Color{ 255, 0x7E, 0x57, 0xC2 }, 4.0);

        // Twinkling, spinning star.
        StartRotation(Star0T(), 10);
        StartPulse(Star0T(), 0.8, 1.4, 1.0);

        // Gentle title breathing.
        StartOpacityBlink(TitleText(), 0.65, 1.0, 2.6);
    }

    void AnimationWindow::Run(std::initializer_list<Timeline> animations)
    {
        Storyboard const storyboard;
        for (auto const& animation : animations)
        {
            storyboard.Children().Append(animation);
        }
        storyboard.Begin();
    }

    DoubleAnimation AnimationWindow::MakeDouble(
        DependencyObject const& target,
        hstring const& property,
        double from,
        double to,
        double seconds,
        double beginSeconds,
        bool autoReverse,
        EasingFunctionBase const& ease)
    {
        DoubleAnimation const animation;
        animation.From(box_value(from).as<IReference<double>>());
        animation.To(box_value(to).as<IReference<double>>());
        animation.Duration(Duration{
            std::chrono::duration_cast<TimeSpan>(std::chrono::duration<double>{ seconds })
        });
        animation.BeginTime(box_value(
            std::chrono::duration_cast<TimeSpan>(std::chrono::duration<double>{ beginSeconds })
        ).as<IReference<TimeSpan>>());
        animation.AutoReverse(autoReverse);
        animation.RepeatBehavior({ 0.0, {}, RepeatBehaviorType::Forever });
        animation.EnableDependentAnimation(true);
        animation.EasingFunction(ease);
        Storyboard::SetTarget(animation, target);
        Storyboard::SetTargetProperty(animation, property);
        return animation;
    }

    // period > 0 spins clockwise, < 0 counter-clockwise.
    void AnimationWindow::StartRotation(CompositeTransform const& transform, double period)
    {
        double const to = period < 0 ? -360.0 : 360.0;
        Run({ MakeDouble(transform, L"Rotation", 0, to, std::abs(period)) });
    }

    void AnimationWindow::StartPulse(
        CompositeTransform const& transform,
        double min,
        double max,
        double seconds)
    {
        QuadraticEase const ease;
        ease.EasingMode(EasingMode::EaseInOut);
        Run({
            MakeDouble(transform, L"ScaleX", min, max, seconds, 0, true, ease),
            MakeDouble(transform, L"ScaleY", min, max, seconds, 0, true, ease)
        });
    }

    void AnimationWindow::StartFloatY(CompositeTransform const& transform, double delta, double seconds)
    {
        SineEase const ease;
        ease.EasingMode(EasingMode::EaseInOut);
        Run({ MakeDouble(transform, L"TranslateY", 0, delta, seconds, 0, true, ease) });
    }

    void AnimationWindow::StartBounce(
        CompositeTransform const& transform,
        double delta,
        double seconds,
        double begin)
    {
        BounceEase const ease;
        ease.Bounces(2);
        ease.Bounciness(2);
        ease.EasingMode(EasingMode::EaseOut);
        Run({ MakeDouble(transform, L"TranslateY", 0, delta, seconds, begin, true, ease) });
    }

    void AnimationWindow::StartWobble(
        CompositeTransform const& transform,
        double degrees,
        double seconds)
    {
        QuadraticEase const ease;
        ease.EasingMode(EasingMode::EaseInOut);
        Run({ MakeDouble(transform, L"Rotation", -degrees, degrees, seconds, 0, true, ease) });
    }

    void AnimationWindow::StartOpacityBlink(
        UIElement const& element,
        double min,
        double max,
        double seconds)
    {
        SineEase const ease;
        ease.EasingMode(EasingMode::EaseInOut);
        Run({ MakeDouble(element, L"Opacity", max, min, seconds, 0, true, ease) });
    }

    void AnimationWindow::StartColorCycle(
        SolidColorBrush const& brush,
        Color const& from,
        Color const& to,
        double seconds)
    {
        ColorAnimation const animation;
        animation.From(box_value(from).as<IReference<Color>>());
        animation.To(box_value(to).as<IReference<Color>>());
        animation.Duration(Duration{
            std::chrono::duration_cast<TimeSpan>(std::chrono::duration<double>{ seconds })
        });
        animation.AutoReverse(true);
        animation.RepeatBehavior({ 0.0, {}, RepeatBehaviorType::Forever });
        animation.EnableDependentAnimation(true);
        SineEase const ease;
        ease.EasingMode(EasingMode::EaseInOut);
        animation.EasingFunction(ease);
        Storyboard::SetTarget(animation, brush);
        Storyboard::SetTargetProperty(animation, L"Color");
        Run({ animation });
    }
}
