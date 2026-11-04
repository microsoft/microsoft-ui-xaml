#pragma once

#include "MainWindow.g.h"

namespace winrt::FolderTreeAppCpp::implementation
{
    struct MainWindow : MainWindowT<MainWindow>
    {
        MainWindow();

        Windows::Foundation::Collections::IObservableVector<FolderTreeAppCpp::FolderNode> RootNodes() const;

        void FolderTreeView_Expanding(
            Microsoft::UI::Xaml::Controls::TreeView const& sender,
            Microsoft::UI::Xaml::Controls::TreeViewExpandingEventArgs const& args);
        void FolderTreeView_SelectionChanged(
            Microsoft::UI::Xaml::Controls::TreeView const& sender,
            Microsoft::UI::Xaml::Controls::TreeViewSelectionChangedEventArgs const& args);
        void OpenAnimations_Click(
            Windows::Foundation::IInspectable const& sender,
            Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void RootGrid_Loaded(
            Windows::Foundation::IInspectable const& sender,
            Microsoft::UI::Xaml::RoutedEventArgs const& args);

    private:
        void LoadRootFolder();
        void BuildShapeVisual(Microsoft::UI::Composition::Compositor const& compositor);
        void BuildContainerVisual(Microsoft::UI::Composition::Compositor const& compositor);

        Windows::Foundation::Collections::IObservableVector<FolderTreeAppCpp::FolderNode> m_rootNodes{
            winrt::single_threaded_observable_vector<FolderTreeAppCpp::FolderNode>()
        };
        FolderTreeAppCpp::AnimationWindow m_animationWindow{ nullptr };
        Microsoft::UI::Xaml::Window::Closed_revoker m_animationWindowClosedRevoker{};
    };
}

namespace winrt::FolderTreeAppCpp::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}
