// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#pragma push_macro("GetCurrentTime")
#undef GetCurrentTime

#include "MainWindow.g.h"
#include <winrt/microsoft.ui.composition.systembackdrops.h>
#include <winrt/windows.system.h>
#include <dispatcherqueue.h>

#pragma pop_macro("GetCurrentTime")
namespace winrt::WinUICppDesktopSampleApp::implementation
{
    enum class BackgroundType
    {
        Mica,
        DesktopAcrylic,
        DefaultColor
    };

    struct MainWindow : MainWindowT<MainWindow>
    {
        MainWindow();
        ~MainWindow();

        Microsoft::UI::Xaml::Controls::Frame RootFrame();

        void Button_Click(IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void ToggleButton_Click(IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void ToggleButton_Checked(IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void ToggleButton_Unchecked(IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void ButtonWithFlyout_Click(IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void ButtonFlyout_Opened(IInspectable const& sender, IInspectable const& args);
        void ButtonFlyout_Closed(IInspectable const& sender, IInspectable const& args);
        void ButtonWithContextFlyout_RightTapped(IInspectable const& sender, Microsoft::UI::Xaml::Input::RightTappedRoutedEventArgs const& args);
        void LoadWebView2Button_Click(winrt::Windows::Foundation::IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void ButtonContextFlyout_Opened(IInspectable const& sender, IInspectable const& args);
        void ButtonContextFlyout_Closed(IInspectable const& sender, IInspectable const& args);
        void ShareMenuFlyoutItem_Click(IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void SplitButton_Click(IInspectable const& sender, Microsoft::UI::Xaml::Controls::SplitButtonClickEventArgs const& args);
        void SplitButtonFlyout_Opened(IInspectable const& sender, IInspectable const& args);
        void SplitButtonFlyout_Closed(IInspectable const& sender, IInspectable const& args);
        void DropDownButton_Click(IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void ButtonResetBounds_Click(IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void ButtonGetBounds_Click(IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void ButtonResetVisible_Click(IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void ButtonGetVisible_Click(IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void ButtonResetContent_Click(IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void ButtonGetContent_Click(IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void ButtonResetTitle_Click(IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void ButtonGetTitle_Click(IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void ButtonSetTitle_Click(IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void ButtonResetHandle_Click(IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void ButtonGetHandle_Click(IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void ButtonSetSize_Click(IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void ButtonCreateWindow_Click(IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void ButtonSetWindowFree_Click(IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void ButtonActivateWindow_Click(IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void ButtonCloseWindow_Click(IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void ButtonDiscardWindow_Click(IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void ButtonExitApplication_Click(IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void ButtonClearEvents_Click(IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void ButtonClearEvents2_Click(IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void ButtonClearError_Click(IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void ButtonClearError2_Click(IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void ButtonClearError3_Click(IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void RectangleManipulated_ManipulationStarting(IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::ManipulationStartingRoutedEventArgs const& args);
        void RectangleManipulated_ManipulationStarted(IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::ManipulationStartedRoutedEventArgs const& args);
        void RectangleManipulated_ManipulationDelta(IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::ManipulationDeltaRoutedEventArgs const& args);
        void RectangleManipulated_ManipulationInertiaStarting(IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::ManipulationInertiaStartingRoutedEventArgs const& args);
        void RectangleManipulated_ManipulationCompleted(IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::ManipulationCompletedRoutedEventArgs const& args);
        void titlebarChange_Click(IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void titlebarSizeChange_Click(IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void titlebarNullptr_Click(IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void contentDialogWithTitlebar_Click(IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void ChangeBackgroundButton_Click(IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);

        void extremeMarginTitlebar_Click(IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);

        // This callback function is attached to the child MarkupWindow manually, and not associated with MainWindow markup
        void ButtonActivateMainWindow_Click(IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);

    private:
        void EnableShutdownOrderValidation();
        void DiscardWindow();
        void LogWindowEvent(hstring strEvent);
        void LogWindowEvent2(hstring strEvent);
        void RefreshEnabledButtons();
        void MainWindow_Closed(IInspectable const& sender, Microsoft::UI::Xaml::WindowEventArgs const& args);
        void Window_Activated(hstring windowString, IInspectable const& sender, Microsoft::UI::Xaml::WindowActivatedEventArgs const& args);
        void Window_Closed(IInspectable const& sender, Microsoft::UI::Xaml::WindowEventArgs const& args);
        void Window_SizeChanged(IInspectable const& sender, Microsoft::UI::Xaml::WindowSizeChangedEventArgs const& args);
        void Window_VisibilityChanged(IInspectable const& sender, Microsoft::UI::Xaml::WindowVisibilityChangedEventArgs const& args);
        HWND GetMainWindowHwnd();
        hstring FormatString(std::wstring_view formatString, ...);
        void SetBackground(BackgroundType type);
        static Windows::System::DispatcherQueueController CreateSystemDispatcherQueueController();

    private:
        winrt::Microsoft::UI::Xaml::Window m_window{ nullptr };
        winrt::Microsoft::UI::Xaml::Controls::Button m_mainWindowActivateButton{ nullptr };
        bool m_isWindowCloseable{ false };
        bool m_bFreeWillWindows{ false };
        bool isExplicitlySetDarkTheme{ true };  // used for toggling theme. we start with dark theme always

        winrt::Microsoft::UI::Composition::SystemBackdrops::MicaController m_micaController{ nullptr };
        winrt::Microsoft::UI::Composition::SystemBackdrops::DesktopAcrylicController m_acrylicController{ nullptr };
        winrt::Microsoft::UI::Xaml::Window::Activated_revoker m_activatedEventRevoker{};
        BackgroundType m_currentBackground{ BackgroundType::Mica };
        Windows::System::DispatcherQueueController m_dispatcherQueueController{ nullptr };
        
        double rectangleManipulatedPremanipulationLeft{};
        double rectangleManipulatedPremanipulationTop{};
        event_token m_mainWindowActivateButtonClickToken{};
        event_token m_windowActivatedToken{};
        event_token m_windowClosedToken{};
        event_token m_windowSizeChangedToken{};
        event_token m_windowVisibilityChangedToken{};
        event_token m_dispatcherQueueShutdownStartingToken{};
    public:
        void TextBox_TextChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Controls::TextChangedEventArgs const& e);
        void themeChangeBtn_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void AppWindowObj_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void toggleAppTheme();
};
}

namespace winrt::WinUICppDesktopSampleApp::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}
