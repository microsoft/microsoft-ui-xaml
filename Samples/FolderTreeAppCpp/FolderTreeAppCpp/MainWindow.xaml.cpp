#include "pch.h"
#include "MainWindow.xaml.h"
#include "AnimationWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

#include <winrt/Windows.Foundation.Numerics.h>
#include <string>

using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Foundation::Numerics;
using namespace Windows::Storage;
using namespace Windows::UI;
using namespace Microsoft::UI;
using namespace Microsoft::UI::Composition;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Hosting;

namespace winrt::FolderTreeAppCpp::implementation
{
    MainWindow::MainWindow()
    {
        InitializeComponent();
        LoadRootFolder();
    }

    IObservableVector<FolderTreeAppCpp::FolderNode> MainWindow::RootNodes() const
    {
        return m_rootNodes;
    }

    void MainWindow::LoadRootFolder()
    {
        auto const rootPath = UserDataPaths::GetDefault().Profile();
        auto const rootNode = FolderTreeAppCpp::FolderNode{ rootPath };
        rootNode.LoadChildren();
        m_rootNodes.Append(rootNode);
        StatusText().Text(hstring{ std::wstring{ L"Browsing: " }.append(rootPath) });
    }

    void MainWindow::FolderTreeView_Expanding(
        TreeView const&,
        TreeViewExpandingEventArgs const& args)
    {
        if (auto const folder = args.Item().try_as<FolderTreeAppCpp::FolderNode>())
        {
            if (folder.HasUnrealizedChildren())
            {
                folder.LoadChildren();
            }
        }
    }

    void MainWindow::FolderTreeView_SelectionChanged(
        TreeView const& sender,
        TreeViewSelectionChangedEventArgs const&)
    {
        if (auto const folder = sender.SelectedItem().try_as<FolderTreeAppCpp::FolderNode>())
        {
            StatusText().Text(hstring{ std::wstring{ L"Selected: " }.append(folder.Name()) });
        }
    }

    void MainWindow::OpenAnimations_Click(IInspectable const&, RoutedEventArgs const&)
    {
        if (!m_animationWindow)
        {
            m_animationWindow = make<AnimationWindow>();
            m_animationWindowClosedRevoker = m_animationWindow.Closed(
                auto_revoke,
                [this](IInspectable const&, WindowEventArgs const&)
                {
                    m_animationWindowClosedRevoker.revoke();
                    m_animationWindowClosedRevoker = {};
                    m_animationWindow = nullptr;
                });
        }

        m_animationWindow.Activate();
    }

    // Build a couple of composition visuals by hand and host them in named Borders, so the
    // composition tree shows more than the SpriteVisuals XAML normally emits: a ShapeVisual
    // (with a CompositionSpriteShape) and a ContainerVisual holding several SpriteVisuals.
    void MainWindow::RootGrid_Loaded(IInspectable const&, RoutedEventArgs const&)
    {
        auto const compositor = ElementCompositionPreview::GetElementVisual(RootGrid()).Compositor();

        BuildShapeVisual(compositor);
        BuildContainerVisual(compositor);
    }

    // A ShapeVisual containing a stroked, filled ellipse — plus a forever rotation animation
    // so it is easy to spot in a live capture.
    void MainWindow::BuildShapeVisual(Microsoft::UI::Composition::Compositor const& compositor)
    {
        auto const shapeVisual = compositor.CreateShapeVisual();
        shapeVisual.Size({ 140.0f, 140.0f });
        shapeVisual.CenterPoint({ 70.0f, 70.0f, 0.0f });
        shapeVisual.Comment(L"FolderTreeApp.Showcase.ShapeVisual");

        auto const ellipseGeometry = compositor.CreateEllipseGeometry();
        ellipseGeometry.Radius({ 55.0f, 45.0f });
        ellipseGeometry.Center({ 70.0f, 70.0f });

        auto const ellipseShape = compositor.CreateSpriteShape(ellipseGeometry);
        ellipseShape.FillBrush(compositor.CreateColorBrush(Microsoft::UI::Colors::MediumPurple()));
        ellipseShape.StrokeBrush(compositor.CreateColorBrush(Microsoft::UI::Colors::Purple()));
        ellipseShape.StrokeThickness(4.0f);
        shapeVisual.Shapes().Append(ellipseShape);

        ElementCompositionPreview::SetElementChildVisual(ShapeVisualHost(), shapeVisual);

        auto const rotate = compositor.CreateScalarKeyFrameAnimation();
        rotate.InsertKeyFrame(1.0f, 360.0f, compositor.CreateLinearEasingFunction());
        rotate.Duration(std::chrono::seconds{ 6 });
        rotate.IterationBehavior(AnimationIterationBehavior::Forever);
        shapeVisual.StartAnimation(L"RotationAngleInDegrees", rotate);
    }

    // A ContainerVisual parenting three colored SpriteVisuals.
    void MainWindow::BuildContainerVisual(Microsoft::UI::Composition::Compositor const& compositor)
    {
        auto const container = compositor.CreateContainerVisual();
        container.Size({ 200.0f, 80.0f });
        container.Comment(L"FolderTreeApp.Showcase.ContainerVisual");

        std::array<Color, 3> const colors{
            Microsoft::UI::Colors::Tomato(),
            Microsoft::UI::Colors::Gold(),
            Microsoft::UI::Colors::MediumSeaGreen()
        };

        for (uint32_t index = 0; index < colors.size(); ++index)
        {
            auto const sprite = compositor.CreateSpriteVisual();
            sprite.Size({ 56.0f, 56.0f });
            sprite.Offset({ 12.0f + index * 64.0f, 12.0f, 0.0f });
            sprite.Brush(compositor.CreateColorBrush(colors[index]));
            sprite.Comment(hstring{
                std::wstring{ L"FolderTreeApp.Showcase.Sprite" }.append(to_hstring(index))
            });
            container.Children().InsertAtTop(sprite);
        }

        ElementCompositionPreview::SetElementChildVisual(ContainerVisualHost(), container);
    }
}
