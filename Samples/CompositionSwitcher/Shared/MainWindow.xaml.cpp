#include "pch.h"
#include "MainWindow.xaml.h"

#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

using namespace winrt;
using namespace Microsoft::UI::Composition;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Hosting;

namespace winrt::CompositionSwitcherSample::implementation
{
    void MainWindow::OnRootLoaded(
        Windows::Foundation::IInspectable const&,
        RoutedEventArgs const&)
    {
        engineText().Text(g_engineStatus);

        auto hostVisual = ElementCompositionPreview::GetElementVisual(animationHost());
        auto compositor = hostVisual.Compositor();

        m_sprite = compositor.CreateSpriteVisual();
        m_sprite.Size({ 72.0f, 72.0f });
        m_sprite.Brush(compositor.CreateColorBrush({ 255, 0, 120, 215 }));
        ElementCompositionPreview::SetElementChildVisual(animationHost(), m_sprite);

        auto animation = compositor.CreateVector3KeyFrameAnimation();
        animation.InsertKeyFrame(0.0f, { 24.0f, 74.0f, 0.0f });
        animation.InsertKeyFrame(0.5f, { 464.0f, 74.0f, 0.0f });
        animation.InsertKeyFrame(1.0f, { 24.0f, 74.0f, 0.0f });
        animation.Duration(std::chrono::seconds(3));
        animation.IterationBehavior(AnimationIterationBehavior::Forever);

        m_sprite.StartAnimation(L"Offset", animation);
    }
}
