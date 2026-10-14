#pragma once

#include "AnimationWindow.g.h"

namespace winrt::FolderTreeAppCpp::implementation
{
    struct AnimationWindow : AnimationWindowT<AnimationWindow>
    {
        AnimationWindow();

        void RootGrid_Loaded(
            Windows::Foundation::IInspectable const& sender,
            Microsoft::UI::Xaml::RoutedEventArgs const& args);

    private:
        void BuildBackdrop();
        void BuildPulsingRings();
        void BuildOrbit();
        void BuildBouncingTiles();
        void BuildXamlStoryboards();

        static Windows::Foundation::Numerics::float3 Offset(float degrees, float radius);
        static void Run(std::initializer_list<Microsoft::UI::Xaml::Media::Animation::Timeline> animations);
        static Microsoft::UI::Xaml::Media::Animation::DoubleAnimation MakeDouble(
            Microsoft::UI::Xaml::DependencyObject const& target,
            winrt::hstring const& property,
            double from,
            double to,
            double seconds,
            double beginSeconds = 0,
            bool autoReverse = false,
            Microsoft::UI::Xaml::Media::Animation::EasingFunctionBase const& ease = nullptr);
        static void StartRotation(Microsoft::UI::Xaml::Media::CompositeTransform const& transform, double period);
        static void StartPulse(
            Microsoft::UI::Xaml::Media::CompositeTransform const& transform,
            double min,
            double max,
            double seconds);
        static void StartFloatY(
            Microsoft::UI::Xaml::Media::CompositeTransform const& transform,
            double delta,
            double seconds);
        static void StartBounce(
            Microsoft::UI::Xaml::Media::CompositeTransform const& transform,
            double delta,
            double seconds,
            double begin);
        static void StartWobble(
            Microsoft::UI::Xaml::Media::CompositeTransform const& transform,
            double degrees,
            double seconds);
        static void StartOpacityBlink(
            Microsoft::UI::Xaml::UIElement const& element,
            double min,
            double max,
            double seconds);
        static void StartColorCycle(
            Microsoft::UI::Xaml::Media::SolidColorBrush const& brush,
            Windows::UI::Color const& from,
            Windows::UI::Color const& to,
            double seconds);

        Microsoft::UI::Composition::Compositor m_compositor{ nullptr };
        Microsoft::UI::Composition::ContainerVisual m_scene{ nullptr };
    };
}

namespace winrt::FolderTreeAppCpp::factory_implementation
{
    struct AnimationWindow : AnimationWindowT<AnimationWindow, implementation::AnimationWindow>
    {
    };
}
