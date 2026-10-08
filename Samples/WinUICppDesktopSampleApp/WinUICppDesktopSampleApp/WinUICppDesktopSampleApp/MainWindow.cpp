// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "MainWindow.h"
#include "MainWindow.g.cpp"
#include "Microsoft.UI.Xaml.Window.h"
#include <winrt/microsoft.ui.xaml.h>
#include <winrt/microsoft.ui.xaml.hosting.h>
#include <winrt/windows.ui.core.h>
#include <winrt/Microsoft.UI.Windowing.h>
#include "strsafe.h"
#include <iostream>
#include <sstream>
#include "ShutdownOrderValidation.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Input;
using namespace Windows::System;
using namespace Microsoft::UI::Xaml::Media;

namespace winrt
{
    namespace MUC = winrt::Microsoft::UI::Composition;
    namespace MUCSB = winrt::Microsoft::UI::Composition::SystemBackdrops;
    namespace MUD = winrt::Microsoft::UI::Dispatching;
    namespace MUX = winrt::Microsoft::UI::Xaml;
    namespace MUXH = winrt::Microsoft::UI::Xaml::Hosting;
}

namespace winrt::WinUICppDesktopSampleApp::implementation
{
    MainWindow::MainWindow()
    {
        InitializeComponent();

        SetBackground(m_currentBackground);

        // sets up window chrome with the provided custom title bar
        ExtendsContentIntoTitleBar(true);
        SetTitleBar(myTitleBar());

        this->Closed([&](IInspectable const&, WindowEventArgs const& args)
        {
            ShutdownOrderValidation::Log(L"Window.Closed raised.");

            if (testShutdownCheckbox().IsChecked().Value())
            {
                EnableShutdownOrderValidation();
            }

            // Prevent window from closing if closingCheckbox is checked.
            const bool shouldBlockClose = this->closingCheckbox().IsChecked().Value();
            args.Handled(shouldBlockClose);

            if (!shouldBlockClose)
            {
                if (m_micaController != nullptr)
                {
                    m_micaController.Close();
                }
                if (m_acrylicController != nullptr)
                {
                    m_acrylicController.Close();
                }
                if (m_dispatcherQueueController != nullptr)
                {
                    // Note in addition to calling ShutdownQueueAsync, DispatcherQueue additionally requires a message loop to run until its ShutdownCompleted event fires. 
                    // We are not doing that here, because this sample does not control the message loop.
                    m_dispatcherQueueController.ShutdownQueueAsync();
                    m_dispatcherQueueController = nullptr;
                }
            }
            
        });

        auto viewModel = WinUICppRuntimeComponent::ViewModel();
        viewModel.Text(L"Text from ViewModel");
        ViewModelBindingTextBlock().DataContext(viewModel);
    }

    MainWindow::~MainWindow()
    {
        ShutdownOrderValidation::Log(L"MainWindow::~MainWindow called.");
    }

    void MainWindow::EnableShutdownOrderValidation()
    {
        ShutdownOrderValidation::ValidateStateOnProcessExit(true);

        auto weak_this = make_weak<class_type>(*this);
        m_dispatcherQueueShutdownStartingToken = DispatcherQueue().ShutdownStarting(
            [weak_this](
                const winrt::MUD::DispatcherQueue& sender,
                const winrt::MUD::DispatcherQueueShutdownStartingEventArgs&)
            {
                ShutdownOrderValidation::Log(L"ShutdownStarting raised.");

                // Interacting with MainWindow (this) and a live Xaml object should be safe at this point.
                // We call get() without checking for null here because we want to crash if it's null.
                auto strong_this = weak_this.get().as<MainWindow>();
                sender.ShutdownStarting(strong_this->m_dispatcherQueueShutdownStartingToken);
                strong_this->m_dispatcherQueueShutdownStartingToken = {};

                sender.TryEnqueue(winrt::MUD::DispatcherQueueHandler([weak_this]() {
                    // It should *still* be safe at this point.
                    auto strong_this = weak_this.get().as<MainWindow>();
                    strong_this->titleBarText().Text(L"Work queued from ShutdownStarting running... ");
                    ShutdownOrderValidation::Log(L"DispatcherQueue work is running.");
                    }));
            });
        
        auto windowsXamlManager = MUXH::WindowsXamlManager::GetForCurrentThread();
        windowsXamlManager.XamlShutdownCompletedOnThread([](
            const MUXH::WindowsXamlManager&,
            const MUXH::XamlShutdownCompletedOnThreadEventArgs&)
            {
                ShutdownOrderValidation::Log(L"XamlShutdownCompletedOnThread raised.");
                // Xaml is no longer safe to interact with here.
            });


        DispatcherQueue().ShutdownCompleted([](auto&&, auto&&) {
            ShutdownOrderValidation::Log(L"ShutdownCompleted raised.");
            ShutdownOrderValidation::SaveLogToTempFile();
            });

    }

    Microsoft::UI::Xaml::Controls::Frame MainWindow::RootFrame()
    {
        return m_rootFrame();
    }
    
    // Mica / DesktopAcrylic code
    void MainWindow::SetBackground(BackgroundType type)
    {
        // Reset to default color. If the requested type is supported, we'll update to that.
        m_currentBackground = BackgroundType::DefaultColor;
        btnChangeBackground().Content(box_value(L"Change Background (current: default color)"));
        m_micaController = nullptr;
        m_acrylicController = nullptr;
        m_activatedEventRevoker.revoke();

        if (type == BackgroundType::Mica && winrt::MUCSB::MicaController::IsSupported())
        {
            // MicaController requires a system DispatcherQueue, so create one if not already present.
            if (m_dispatcherQueueController  == nullptr && Windows::System::DispatcherQueue::GetForCurrentThread()== nullptr)
            {
                m_dispatcherQueueController = CreateSystemDispatcherQueueController();
            }

            btnChangeBackground().Content(box_value(L"Change Background (current: Mica)"));
            m_currentBackground = type;

            // Hooking up the policy object
            winrt::MUCSB::SystemBackdropConfiguration configurationSource = winrt::MUCSB::SystemBackdropConfiguration();

            m_activatedEventRevoker = this->Activated(winrt::auto_revoke, [configurationSource](auto&&, const MUX::WindowActivatedEventArgs& args)
            {
                configurationSource.IsInputActive(args.WindowActivationState() != Microsoft::UI::Xaml::WindowActivationState::Deactivated);
            });

            // Initial policy state.
            configurationSource.IsInputActive(true);

            m_micaController = winrt::MUCSB::MicaController();

            // Customize to MicaBaseAlt variant.
            m_micaController.Kind(winrt::MUCSB::MicaKind::BaseAlt);

            // Enable the system backdrop.
            m_micaController.AddSystemBackdropTarget(this->try_as<MUC::ICompositionSupportsSystemBackdrop>());
            m_micaController.SetSystemBackdropConfiguration(configurationSource);
        }
        else if (type == BackgroundType::DesktopAcrylic && winrt::MUCSB::DesktopAcrylicController::IsSupported())
        {
            // DesktopAcrylicController requires a system DispatcherQueue, so create one if not already present.
            if (m_dispatcherQueueController  == nullptr && Windows::System::DispatcherQueue::GetForCurrentThread()== nullptr)
            {
                m_dispatcherQueueController = CreateSystemDispatcherQueueController();
            }

            btnChangeBackground().Content(box_value(L"Change Background (current: Acrylic)"));
            m_currentBackground = type;

            // Hooking up the policy object
            winrt::MUCSB::SystemBackdropConfiguration configurationSource = winrt::MUCSB::SystemBackdropConfiguration();

            m_activatedEventRevoker = this->Activated(winrt::auto_revoke, [configurationSource](auto&&, const MUX::WindowActivatedEventArgs& args)
            {
                configurationSource.IsInputActive(args.WindowActivationState() != Microsoft::UI::Xaml::WindowActivationState::Deactivated);
            });

            // Initial policy state.
            configurationSource.IsInputActive(true);

            m_acrylicController = winrt::MUCSB::DesktopAcrylicController();
            m_acrylicController.LuminosityOpacity(1.0f);
            m_acrylicController.TintOpacity(0.5f);
            winrt::Windows::UI::Color tintColor{ 255, 243, 243, 243 };
            m_acrylicController.TintColor(tintColor);

            // Enable the system backdrop.
            m_acrylicController.AddSystemBackdropTarget(this->try_as<MUC::ICompositionSupportsSystemBackdrop>());
            m_acrylicController.SetSystemBackdropConfiguration(configurationSource);
        }
    }

    void MainWindow::ChangeBackgroundButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        BackgroundType newType;
        switch (m_currentBackground)
        {
            case BackgroundType::Mica:           newType = BackgroundType::DesktopAcrylic; break;
            case BackgroundType::DesktopAcrylic: newType = BackgroundType::DefaultColor; break;
            default:
            case BackgroundType::DefaultColor:   newType = BackgroundType::Mica; break;
        }
        SetBackground(newType);
    }

    Windows::System::DispatcherQueueController MainWindow::CreateSystemDispatcherQueueController()
    {
        DispatcherQueueOptions options
        {
            sizeof(DispatcherQueueOptions),
            DQTYPE_THREAD_CURRENT,
            DQTAT_COM_STA
        };

        ::ABI::Windows::System::IDispatcherQueueController* ptr{};
        winrt::check_hresult(CreateDispatcherQueueController(options, &ptr));
        return { ptr, take_ownership_from_abi };
    }

    // MUX Controls Tests
    void MainWindow::Button_Click(IInspectable const&, RoutedEventArgs const&)
    {
        textBlockMUXControls().Text(L"Button.Click");
    }

    void MainWindow::ToggleButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        if (toggleButton().IsChecked().Value())
        {
            textBlockMUXControls().Text(L"ToggleButton.Click - IsChecked=True");
        }
        else
        {
            textBlockMUXControls().Text(L"ToggleButton.Click - IsChecked=False");
        }
        checkBox().IsChecked(toggleButton().IsChecked());
    }

    void MainWindow::ToggleButton_Checked(IInspectable const&, RoutedEventArgs const&)
    {
        textBlockMUXControls().Text(L"ToggleButton.Checked");
        checkBox().IsChecked(true);
    }

    void MainWindow::ToggleButton_Unchecked(IInspectable const&, RoutedEventArgs const&)
    {
        textBlockMUXControls().Text(L"ToggleButton.Unchecked");
        checkBox().IsChecked(false);
    }

    void MainWindow::ButtonWithFlyout_Click(IInspectable const&, RoutedEventArgs const&)
    {
        textBlockMUXControls().Text(L"Button.Click - buttonWithFlyout");
    }

    void MainWindow::ButtonFlyout_Opened(IInspectable const&, IInspectable const&)
    {
        textBlockMUXControls().Text(L"Flyout.Opened - buttonWithFlyout");
    }

    void MainWindow::ButtonFlyout_Closed(IInspectable const&, IInspectable const&)
    {
        textBlockMUXControls().Text(L"Flyout.Closed - buttonWithFlyout");
    }
    
    void MainWindow::ButtonWithContextFlyout_RightTapped(IInspectable const&, RightTappedRoutedEventArgs const&)
    {
        textBlockMUXControls().Text(L"Button.RightTapped - buttonWithContextFlyout");
    }

    void MainWindow::ButtonContextFlyout_Opened(IInspectable const&, IInspectable const&)
    {
        textBlockMUXControls().Text(L"Flyout.Opened - buttonWithContextFlyout");
    }

    void MainWindow::ButtonContextFlyout_Closed(IInspectable const&, IInspectable const&)
    {
        textBlockMUXControls().Text(L"Flyout.Closed - buttonWithContextFlyout");
    }
    
    void MainWindow::ShareMenuFlyoutItem_Click(IInspectable const&, RoutedEventArgs const&)
    {
        textBlockMUXControls().Text(L"MenuFlyoutItem.Click - buttonWithContextFlyout");
    }

    void MainWindow::SplitButton_Click(IInspectable const&, SplitButtonClickEventArgs const&)
    {
        textBlockMUXCControls().Text(L"SplitButton.Click");
    }

    void MainWindow::LoadWebView2Button_Click(IInspectable const&, RoutedEventArgs const&)
    {
        myWebView2().Source(winrt::Windows::Foundation::Uri(L"http://www.bing.com"));
    }

    void MainWindow::SplitButtonFlyout_Opened(IInspectable const&, IInspectable const&)
    {
        textBlockMUXCControls().Text(L"SplitButtonFlyout.Opened");
    }

    void MainWindow::SplitButtonFlyout_Closed(IInspectable const&, IInspectable const&)
    {
        textBlockMUXCControls().Text(L"SplitButtonFlyout.Closed");
    }

    void MainWindow::DropDownButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        textBlockMUXCControls().Text(L"DropDownButton.Click");
    }

    void MainWindow::ButtonResetBounds_Click(IInspectable const&, RoutedEventArgs const&)
    {
        textBlockBoundsX().Text(L"");
        textBlockBoundsY().Text(L"");
        textBlockBoundsWidth().Text(L"");
        textBlockBoundsHeight().Text(L"");
    }

    void MainWindow::ButtonGetBounds_Click(IInspectable const&, RoutedEventArgs const&)
    {
        try
        {
            ::Windows::Foundation::Rect bounds = this->Bounds();
            wchar_t hexString[24];

            check_hresult(StringCchPrintfW(&hexString[0], 24, L"%f", bounds.X));
            textBlockBoundsX().Text(hexString);
            check_hresult(StringCchPrintfW(&hexString[0], 24, L"%f", bounds.Y));
            textBlockBoundsY().Text(hexString);
            check_hresult(StringCchPrintfW(&hexString[0], 24, L"%f", bounds.Width));
            textBlockBoundsWidth().Text(hexString);
            check_hresult(StringCchPrintfW(&hexString[0], 24, L"%f", bounds.Height));
            textBlockBoundsHeight().Text(hexString);
        }
        catch (hresult_error e)
        {
            textBoxError().Text(e.message());
        }
    }

    void MainWindow::ButtonResetVisible_Click(IInspectable const&, RoutedEventArgs const&)
    {
        textBlockVisible().Text(L"");
    }
    
    void MainWindow::ButtonGetVisible_Click(IInspectable const&, RoutedEventArgs const&)
    {
        try
        {
            if (this->Visible())
            {
                textBlockVisible().Text(L"True");
            }
            else
            {
                textBlockVisible().Text(L"False");
            }
        }
        catch (hresult_error e)
        {
            textBoxError().Text(e.message());
        }
    }

    void MainWindow::ButtonResetContent_Click(IInspectable const&, RoutedEventArgs const&)
    {
        textBlockContent().Text(L"");
    }

    void MainWindow::ButtonGetContent_Click(IInspectable const&, RoutedEventArgs const&)
    {
        try
        {
            UIElement content = this->Content();
            if (content)
            {
                FrameworkElement contentAsFE = content.as<FrameworkElement>();

                if (contentAsFE)
                {
                    textBlockContent().Text(contentAsFE.Name());
                }
            }
        }
        catch (hresult_error e)
        {
            textBoxError().Text(e.message());
        }
    }

    void MainWindow::ButtonResetTitle_Click(IInspectable const&, RoutedEventArgs const&)
    {
        textBoxTitle().Text(L"");
    }

    void MainWindow::ButtonGetTitle_Click(IInspectable const&, RoutedEventArgs const&)
    {
        try
        {
            textBoxTitle().Text(this->Title());
        }
        catch (hresult_error e)
        {
            textBoxError().Text(e.message());
        }
    }

    void MainWindow::ButtonSetTitle_Click(IInspectable const&, RoutedEventArgs const&)
    {
        try
        {
            this->Title(textBoxTitle().Text());
        }
        catch (hresult_error e)
        {
            textBoxError().Text(e.message());
        }
    }

    void MainWindow::ButtonResetHandle_Click(IInspectable const&, RoutedEventArgs const&)
    {
        textBlockHandle().Text(L"");
    }

    void MainWindow::ButtonGetHandle_Click(IInspectable const&, RoutedEventArgs const&)
    {
        try
        {            
            HWND hwnd = GetMainWindowHwnd();

            textBlockHandle().Text(FormatString(L"%1!u!", reinterpret_cast<uint64_t>(hwnd)));
        }
        catch (hresult_error e)
        {
            textBoxError().Text(e.message());
        }
    }
    
    void MainWindow::ButtonSetSize_Click(IInspectable const&, RoutedEventArgs const&)
    {
        try
        {
            HWND hwnd = GetMainWindowHwnd();

            hstring strWidth = textBoxWidth().Text();
            hstring strHeight = textBoxHeight().Text();

            std::wstring_view widthView = strWidth;
            std::wstring_view heightView = strHeight;

            double rasterizationScale = stackPanelRoot().XamlRoot().RasterizationScale();
            
            int width = static_cast<int>(rasterizationScale * _wtoi(widthView.data()));
            int height = static_cast<int>(rasterizationScale * _wtoi(heightView.data()));

            SetWindowPos(
                hwnd,
                0                         /*hWndInsertAfter*/,
                0                         /*X*/,
                0                         /*Y*/,
                width                     /*cx*/,
                height                    /*cy*/,
                SWP_NOZORDER | SWP_NOMOVE /*ignore hWndInsertAfter param and only change size and not position*/
            );
        }
        catch (hresult_error e)
        {
            textBoxError().Text(e.message());
        }
    }

    void MainWindow::ButtonSetWindowFree_Click(IInspectable const&, RoutedEventArgs const&)
    {
        m_bFreeWillWindows = true;
        buttonSetWindowFree().IsEnabled(!m_bFreeWillWindows);
    }
   
    void MainWindow::ButtonCreateWindow_Click(IInspectable const&, RoutedEventArgs const&)
    {
        try
        {
            bool isMainWindowSelected = false;
            LogWindowEvent(L"ButtonCreateWindow_Click - entry");

            auto weakThis = make_weak<class_type>(*this);
            // ------------------------ Runtime Window ----------------------------------------------
            if (radioButtonRuntimeWindow().IsChecked().Value() == true)
            {
                m_window = Window();
                m_window.Title(L"WinUI Desktop - Runtime Window");
            }
            // ------------------------ Custom Window ----------------------------------------------
            else if (radioButtonBlankCustomWindow().IsChecked().Value() == true)
            {
                auto window = BlankCustomWindow();
                window.Title(L"WinUI Desktop - Custom Window");
                window.Activate();
                return;
            }
            // ------------------------ Markup Window ----------------------------------------------
            else if (radioButtonMarkupWindow().IsChecked().Value() == true)
            {
                auto window = MarkupWindow();

                //grab the main window activate button from the child window and add a click handler in this context so we can print logs and errors
                m_mainWindowActivateButton = window.GetMainWindowActivateButton();
                m_mainWindowActivateButtonClickToken = m_mainWindowActivateButton.Click([weakThis](IInspectable const& sender, RoutedEventArgs const& args)
                    {
                        if (auto mainWindow = weakThis.get())
                        {
                            get_self<MainWindow>(mainWindow)->ButtonActivateMainWindow_Click(sender, args);
                        }
                    });

                m_window = window;
            }
            // ------------------------ Child Window --------------------------------------------------            
            else if (radioButtonChildWindow().IsChecked().Value() == true)
            {
                // Creating parent window
                auto window = Window();
                window.Title(L"WinUI Desktop - Child Window Test");
                window.Activate();

                // Creating child window
                auto childWindow = MarkupWindow();
                childWindow.Activate();

                HWND hWnd{ 0 };
                hWnd = (HWND)window.AppWindow().Id().Value;
          
                HWND hWnd2{ 0 };
                hWnd2 = (HWND)childWindow.AppWindow().Id().Value;
         
                // Setting parent child relationship
                ::SetParent(hWnd2, hWnd);
                m_window = window;

            }
            // ------------------------ Current (Main) Window ----------------------------------------------
            else
            {
                m_window = Window::Current();
                isMainWindowSelected = true;
            }

            this->Closed([weakThis](IInspectable const& sender, WindowEventArgs const& args)
                {
                    if (auto mainWindow = weakThis.get())
                    {
                        get_self<MainWindow>(mainWindow)->MainWindow_Closed(sender, args);
                    }
                });

            this->Activated([weakThis](IInspectable const& sender, WindowActivatedEventArgs const& args)
                {
                    if (auto mainWindow = weakThis.get())
                    {
                        get_self<MainWindow>(mainWindow)->Window_Activated(L"Main", sender, args);
                    }
                });

            if (isMainWindowSelected)
            {
                this->SizeChanged([weakThis](IInspectable const& sender, WindowSizeChangedEventArgs const& args)
                    {
                        if (auto mainWindow = weakThis.get())
                        {
                            get_self<MainWindow>(mainWindow)->Window_SizeChanged(sender, args);
                        }
                    });

                radioButtonMarkupWindow().XamlRoot().Changed([&](IInspectable const&, XamlRootChangedEventArgs const&)
                    {
                        LogWindowEvent(L"XamlRoot_Changed event");
                    });

                this->VisibilityChanged([weakThis](IInspectable const& sender, WindowVisibilityChangedEventArgs const& args)
                    {
                        if (auto mainWindow = weakThis.get())
                        {
                            get_self<MainWindow>(mainWindow)->Window_VisibilityChanged(sender, args);
                        }
                    });
            }
            else
            {
                m_windowActivatedToken = m_window.Activated([weakThis](IInspectable const& sender, WindowActivatedEventArgs const& args)
                    {
                        if (auto mainWindow = weakThis.get())
                        {
                            get_self<MainWindow>(mainWindow)->Window_Activated(L"Secondary", sender, args);
                        }
                    });

                m_windowClosedToken = m_window.Closed([weakThis](IInspectable const& sender, WindowEventArgs const& args)
                    {
                        if (auto mainWindow = weakThis.get())
                        {
                            get_self<MainWindow>(mainWindow)->Window_Closed(sender, args);
                        }
                    });

                m_windowSizeChangedToken = m_window.SizeChanged([weakThis](IInspectable const& sender, WindowSizeChangedEventArgs const& args)
                    {
                        if (auto mainWindow = weakThis.get())
                        {
                            get_self<MainWindow>(mainWindow)->Window_SizeChanged(sender, args);
                        }
                    });

                m_windowVisibilityChangedToken = m_window.VisibilityChanged([weakThis](IInspectable const& sender, WindowVisibilityChangedEventArgs const& args)
                    {
                        if (auto mainWindow = weakThis.get())
                        {
                            get_self<MainWindow>(mainWindow)->Window_VisibilityChanged(sender, args);
                        }
                    });
            }

            // ------------------------ Runtime Window only ----------------------------------------------
            if (radioButtonRuntimeWindow().IsChecked().Value() == true)
            {
                LogWindowEvent(L"ButtonCreateWindow_Click - setting Window.Content");
                TextBlock textBlock = TextBlock();
                textBlock.Name(L"textBlockRuntimeWindow");
                textBlock.Text(L"Runtime Window");
                textBlock.FontSize(20);
                textBlock.Margin({ 2, 2, 2, 2 });
                m_window.Content(textBlock);
            }
            // --------------------------------------------------------------------------------------------

            RefreshEnabledButtons();

            LogWindowEvent(L"ButtonCreateWindow_Click - exit");
        }
        catch (hresult_error e)
        {
            textBoxError().Text(e.message());
        }
    }

    void MainWindow::ButtonActivateMainWindow_Click(IInspectable const& sender, RoutedEventArgs const& args)
    {
        UNREFERENCED_PARAMETER(sender);
        UNREFERENCED_PARAMETER(args);

        try
        {
            LogWindowEvent(L"ButtonActivateMainWindow_Click - entry");

            Activate();
            RefreshEnabledButtons();

            LogWindowEvent(L"ButtonActivateMainWindow_Click - exit");
        }
        catch (hresult_error e)
        {
            textBoxError().Text(e.message());
        }
    }

    void MainWindow::ButtonActivateWindow_Click(IInspectable const&, RoutedEventArgs const&)
    {
        try
        {
            LogWindowEvent(L"ButtonActivateWindow_Click - entry");

            m_window.Activate();
            m_isWindowCloseable = true;
            RefreshEnabledButtons();

            LogWindowEvent(L"ButtonActivateWindow_Click - exit");
        }
        catch (hresult_error e)
        {
            textBoxError().Text(e.message());
        }
    }

    void MainWindow::ButtonCloseWindow_Click(IInspectable const&, RoutedEventArgs const&)
    {
        try
        {
            LogWindowEvent(L"ButtonCloseWindow_Click - entry");

            m_window.Close();
            m_isWindowCloseable = false;
            RefreshEnabledButtons();

            LogWindowEvent(L"ButtonCloseWindow_Click - exit");
        }
        catch (hresult_error e)
        {
            textBoxError().Text(e.message());
        }
    }

    void MainWindow::ButtonDiscardWindow_Click(IInspectable const&, RoutedEventArgs const&)
    {
        try
        {
            LogWindowEvent(L"ButtonDiscardWindow_Click - entry");

            DiscardWindow();
            RefreshEnabledButtons();

            LogWindowEvent(L"ButtonDiscardWindow_Click - exit");
        }
        catch (hresult_error e)
        {
            textBoxError().Text(e.message());
        }
    }

    void MainWindow::DiscardWindow()
    {
        m_window.Activated(m_windowActivatedToken);
        m_window.Closed(m_windowClosedToken);
        m_window.SizeChanged(m_windowSizeChangedToken);
        m_window.VisibilityChanged(m_windowVisibilityChangedToken);
        m_window = nullptr;
    }

    void MainWindow::ButtonExitApplication_Click(IInspectable const&, RoutedEventArgs const&)
    {
        try
        {
            Application::Current().Exit();
        }
        catch (hresult_error e)
        {
            textBoxError3().Text(e.message());
        }
    }

    void MainWindow::ButtonClearEvents_Click(IInspectable const&, RoutedEventArgs const&)
    {
        textBoxLastEvent().Text(L"");
        comboBoxEvents().Items().Clear();
    }

    void MainWindow::ButtonClearEvents2_Click(IInspectable const&, RoutedEventArgs const&)
    {
        textBoxLastEvent2().Text(L"");
        comboBoxEvents2().Items().Clear();
    }

    void MainWindow::ButtonClearError_Click(IInspectable const&, RoutedEventArgs const&)
    {
        textBoxError().Text(L"");
    }

    void MainWindow::ButtonClearError2_Click(IInspectable const&, RoutedEventArgs const&)
    {
        textBoxError2().Text(L"");
    }

    void MainWindow::ButtonClearError3_Click(IInspectable const&, RoutedEventArgs const&)
    {
        textBoxError3().Text(L"");
    }

    void MainWindow::LogWindowEvent(hstring strEvent)
    {
        textBoxLastEvent().Text(strEvent);
        comboBoxEvents().Items().Append(box_value(strEvent));
        comboBoxEvents().SelectedIndex(comboBoxEvents().Items().Size() - 1);
    }

    void MainWindow::LogWindowEvent2(hstring strEvent)
    {
        textBoxLastEvent2().Text(strEvent);
        comboBoxEvents2().Items().Append(box_value(strEvent));
        comboBoxEvents2().SelectedIndex(comboBoxEvents2().Items().Size() - 1);
    }

    void MainWindow::RefreshEnabledButtons()
    {
        if (!buttonGetBounds()) return;

        bool isWindowCreated = m_window != nullptr;

        buttonGetBounds().IsEnabled(isWindowCreated);
        buttonGetVisible().IsEnabled(isWindowCreated);
        buttonGetContent().IsEnabled(isWindowCreated);
        buttonGetTitle().IsEnabled(isWindowCreated);
        buttonSetTitle().IsEnabled(isWindowCreated);
        buttonCreateWindow().IsEnabled(!isWindowCreated);
        buttonActivateWindow().IsEnabled(isWindowCreated);
        buttonCloseWindow().IsEnabled(isWindowCreated && m_isWindowCloseable); // Remove " && m_isWindowCloseable" once closing a never-activated window no longer causes a crash.
        buttonDiscardWindow().IsEnabled(isWindowCreated && !m_isWindowCloseable);
    }

    void MainWindow::MainWindow_Closed(IInspectable const&, WindowEventArgs const&)
    {
        if (m_window && !m_bFreeWillWindows)
        {
            if (m_isWindowCloseable)
            {
                m_window.Close();
            }
            DiscardWindow();
        }
    }

    void MainWindow::Window_Activated(hstring windowString, IInspectable const&, WindowActivatedEventArgs const& args)
    {
        LogWindowEvent(FormatString(L"Window_Activated - Window=%1!s!, Handled=%2!s!, WindowActivationState=%3!d!", windowString.data(), args.Handled() ? L"True" : L"False", args.WindowActivationState()));
    }

    void MainWindow::Window_Closed(IInspectable const&, WindowEventArgs const& args)
    {
        LogWindowEvent(FormatString(L"Window_Closed - Handled=%1!s!", args.Handled() ? L"True" : L"False"));
        
        if (m_mainWindowActivateButtonClickToken)
        {
            m_mainWindowActivateButton.Click(m_mainWindowActivateButtonClickToken);
            m_mainWindowActivateButton = nullptr;
        }

        m_isWindowCloseable = false;
        RefreshEnabledButtons();
    }

    void MainWindow::Window_SizeChanged(IInspectable const&, WindowSizeChangedEventArgs const& args)
    {
        LogWindowEvent(FormatString(L"Window_SizeChanged - Handled=%1!s!", args.Handled() ? L"True" : L"False"));
    }

    void MainWindow::Window_VisibilityChanged(IInspectable const&, WindowVisibilityChangedEventArgs const& args)
    {
        LogWindowEvent(FormatString(L"Window_VisibilityChanged - Handled=%1!s!, Visible=%2!s!", args.Handled() ? L"True" : L"False", args.Visible() ? L"True" : L"False"));
    }

    void MainWindow::RectangleManipulated_ManipulationStarting(IInspectable const&, ManipulationStartingRoutedEventArgs const& args)
    {
        LogWindowEvent2(FormatString(L"RectangleManipulated_ManipulationStarting - Handled=%1!s!, Mode=%2!d!",
            args.Handled() ? L"True" : L"False",
            args.Mode()));
        if (args.Pivot())
        {
            LogWindowEvent2(FormatString(L" - Pivot.Center=%1!i!,%2!i!, Pivot.Radius=%3!i!",
                static_cast<int32_t>(args.Pivot().Center().X),
                static_cast<int32_t>(args.Pivot().Center().Y),
                static_cast<int32_t>(args.Pivot().Radius())));
        }
        else
        {
            LogWindowEvent2(L" - Pivot=null");
        }

        rectangleManipulatedPremanipulationLeft = Canvas::GetLeft(rectangleManipulated());
        rectangleManipulatedPremanipulationTop = Canvas::GetTop(rectangleManipulated());
    }

    void MainWindow::RectangleManipulated_ManipulationStarted(IInspectable const&, ManipulationStartedRoutedEventArgs const& args)
    {
        LogWindowEvent2(FormatString(L"RectangleManipulated_ManipulationStarted - Handled=%1!s!, PointerDeviceType=%2!d!",
            args.Handled() ? L"True" : L"False",
            args.PointerDeviceType()));
        LogWindowEvent2(FormatString(L" - Cumulative.Translation=%1!i!,%2!i!, Position=%3!i!,%4!i!", 
            static_cast<int32_t>(args.Cumulative().Translation.X),
            static_cast<int32_t>(args.Cumulative().Translation.Y),
            static_cast<int32_t>(args.Position().X),
            static_cast<int32_t>(args.Position().Y)));

        Canvas::SetLeft(rectangleManipulated(), rectangleManipulatedPremanipulationLeft + args.Cumulative().Translation.X);
        Canvas::SetTop(rectangleManipulated(), rectangleManipulatedPremanipulationTop + args.Cumulative().Translation.Y);
    }

    void MainWindow::RectangleManipulated_ManipulationDelta(IInspectable const&, ManipulationDeltaRoutedEventArgs const& args)
    {
        LogWindowEvent2(FormatString(L"RectangleManipulated_ManipulationDelta - Handled=%1!s!, PointerDeviceType=%2!d!, IsInertial=%3!s!",
            args.Handled() ? L"True" : L"False",
            args.PointerDeviceType(),
            args.IsInertial() ? L"True" : L"False"));
        LogWindowEvent2(FormatString(L" - Cumulative.Translation=%1!i!,%2!i!, Delta.Translation=%3!i!,%4!i!",
            static_cast<int32_t>(args.Cumulative().Translation.X),
            static_cast<int32_t>(args.Cumulative().Translation.Y),
            static_cast<int32_t>(args.Delta().Translation.X),
            static_cast<int32_t>(args.Delta().Translation.Y)));
        LogWindowEvent2(FormatString(L" - Velocities.Linear=%1!i!,%2!i!, Position=%3!i!,%4!i!",
            static_cast<int32_t>(args.Velocities().Linear.X),
            static_cast<int32_t>(args.Velocities().Linear.Y),
            static_cast<int32_t>(args.Position().X),
            static_cast<int32_t>(args.Position().Y)));
        
        Canvas::SetLeft(rectangleManipulated(), rectangleManipulatedPremanipulationLeft + args.Cumulative().Translation.X);
        Canvas::SetTop(rectangleManipulated(), rectangleManipulatedPremanipulationTop + args.Cumulative().Translation.Y);
    }

    void MainWindow::RectangleManipulated_ManipulationInertiaStarting(IInspectable const&, ManipulationInertiaStartingRoutedEventArgs const& args)
    {
        LogWindowEvent2(FormatString(L"RectangleManipulated_ManipulationInertiaStarting - Handled=%1!s!, PointerDeviceType=%2!d!",
            args.Handled() ? L"True" : L"False",
            args.PointerDeviceType()));

        LogWindowEvent2(FormatString(L" - Cumulative.Translation=%1!i!,%2!i!, Delta.Translation=%3!i!,%4!i!",
            static_cast<int32_t>(args.Cumulative().Translation.X),
            static_cast<int32_t>(args.Cumulative().Translation.Y),
            static_cast<int32_t>(args.Delta().Translation.X),
            static_cast<int32_t>(args.Delta().Translation.Y)));
        LogWindowEvent2(FormatString(L" - Velocities.Linear=%1!i!,%2!i!, TranslationBehavior.DesiredDeceleration=%3!i!, TranslationBehavior.DesiredDisplacement=%4!i!",
            static_cast<int32_t>(args.Velocities().Linear.X),
            static_cast<int32_t>(args.Velocities().Linear.Y),
            static_cast<int32_t>(args.TranslationBehavior().DesiredDeceleration()),
            static_cast<int32_t>(args.TranslationBehavior().DesiredDisplacement())));

        Canvas::SetLeft(rectangleManipulated(), rectangleManipulatedPremanipulationLeft + args.Cumulative().Translation.X);
        Canvas::SetTop(rectangleManipulated(), rectangleManipulatedPremanipulationTop + args.Cumulative().Translation.Y);
    }
    
    void MainWindow::RectangleManipulated_ManipulationCompleted(IInspectable const&, ManipulationCompletedRoutedEventArgs const& args)
    {
        LogWindowEvent2(FormatString(L"RectangleManipulated_ManipulationCompleted - Handled=%1!s!, PointerDeviceType=%2!d!, IsInertial=%3!s!",
            args.Handled() ? L"True" : L"False",
            args.PointerDeviceType(),
            args.IsInertial() ? L"True" : L"False"));
            LogWindowEvent2(FormatString(L" - Cumulative.Translation=%1!i!,%2!i!",
                static_cast<int32_t>(args.Cumulative().Translation.X),
                static_cast<int32_t>(args.Cumulative().Translation.Y)));
            LogWindowEvent2(FormatString(L" - Velocities.Linear=%1!i!,%2!i!, Position=%3!i!,%4!i!",
                static_cast<int32_t>(args.Velocities().Linear.X),
                static_cast<int32_t>(args.Velocities().Linear.Y),
                static_cast<int32_t>(args.Position().X),
                static_cast<int32_t>(args.Position().Y)));

            Canvas::SetLeft(rectangleManipulated(), rectangleManipulatedPremanipulationLeft + args.Cumulative().Translation.X);
            Canvas::SetTop(rectangleManipulated(), rectangleManipulatedPremanipulationTop + args.Cumulative().Translation.Y);
    }

    HWND MainWindow::GetMainWindowHwnd()
    {
        winrt::impl::com_ref<IWindowNative> windowNative = try_as<IWindowNative>();

        if (!windowNative)
        {
            textBoxError().Text(L"QI for IWindowNative unsuccessful");
            return {};
        }

        HWND hwnd{};
        HRESULT hr = windowNative->get_WindowHandle(&hwnd);

        if (FAILED(hr))
        {
            textBoxError().Text(FormatString(L"IWindowNative::get_WindowHandle failed - hr=%1!ld!", hr));
            return {};
        }

        return hwnd;
    }

    hstring MainWindow::FormatString(std::wstring_view formatString, ...)
    {
        va_list args;
        va_start(args, formatString);

        LPVOID formattedString = nullptr;

        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_STRING,
            formatString.data(),
            0,
            0,
            (LPTSTR)&formattedString,
            0,
            &args);

        va_end(args);

        hstring result((LPTSTR)formattedString);
        LocalFree(formattedString);

        return result;
    }
    
    void MainWindow::titlebarChange_Click(IInspectable const&, RoutedEventArgs const&)
    {
        ExtendsContentIntoTitleBar(!ExtendsContentIntoTitleBar());
        if (ExtendsContentIntoTitleBar())
        {
            SetTitleBar(myTitleBar());
            myTitleBar().Visibility(Visibility::Visible);
            myTitleBar().Margin(Thickness{0});
            titlebarChange().Content(box_value(L"Reset titlebar"));
        }
        else
        {
            myTitleBar().Height(32);
            myTitleBar().Visibility(Visibility::Collapsed);
            titlebarChange().Content(box_value(L"Change to Custom titlebar"));
        }
    }
    
    void MainWindow::titlebarSizeChange_Click(IInspectable const&, RoutedEventArgs const&)
    {
            myTitleBar().Height(500);
    }

    void MainWindow::titlebarNullptr_Click(IInspectable const&, RoutedEventArgs const&)
    {
        SetTitleBar(nullptr);
    }

    void MainWindow::contentDialogWithTitlebar_Click(IInspectable const&, RoutedEventArgs const&)
    {
        contentDialogWindowChromeTest().ShowAsync();

    }

    void MainWindow::extremeMarginTitlebar_Click(IInspectable const&, RoutedEventArgs const&)
    {
        myTitleBar().Margin(Thickness{ 100, 100, 100, 100 });
    }
}


void winrt::WinUICppDesktopSampleApp::implementation::MainWindow::TextBox_TextChanged(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::Controls::TextChangedEventArgs const&)
{
    auto lval = winrt::to_string(left().Text());
    if (!strcmp(lval.c_str(), "") || !strcmp(lval.c_str(), "-"))
    {
        lval = "0";
    }

    auto tval = winrt::to_string(top().Text());
    if (!strcmp(tval.c_str(), "") || !strcmp(tval.c_str(), "-"))
    {
        tval = "0";
    }

    auto rval = winrt::to_string(right().Text());
    if (!strcmp(rval.c_str(), "") || !strcmp(rval.c_str(), "-"))
    {
        rval = "0";
    }

    auto bval = winrt::to_string(bottom().Text());
    if (!strcmp(bval.c_str(), "") || !strcmp(bval.c_str(), "-"))
    {
        bval = "0";
    }

    try {
        double lval_double = std::stod(lval);
        double tval_double = std::stod(tval);
        double rval_double = std::stod(rval);
        double bval_double = std::stod(bval);
        myTitleBar().Margin(Thickness{ lval_double, tval_double, rval_double, bval_double });
    }
    catch (const std::invalid_argument& ia) {
        std::cerr << "Invalid argument: " << ia.what() << '\n';
    }

}


void winrt::WinUICppDesktopSampleApp::implementation::MainWindow::toggleAppTheme()
{
    UIElement content = Content();
    if (content)
    {
        FrameworkElement contentAsFE = content.as<FrameworkElement>();

        if (contentAsFE)
        {
            if (!isExplicitlySetDarkTheme)
            {
                contentAsFE.RequestedTheme(ElementTheme::Dark);
            }
            else
            {
                contentAsFE.RequestedTheme(ElementTheme::Light);

            }
            isExplicitlySetDarkTheme = !isExplicitlySetDarkTheme;
        }
    }
}


void winrt::WinUICppDesktopSampleApp::implementation::MainWindow::themeChangeBtn_Click(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
{
    toggleAppTheme();
}


void winrt::WinUICppDesktopSampleApp::implementation::MainWindow::AppWindowObj_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
{
    UNREFERENCED_PARAMETER(sender);
    UNREFERENCED_PARAMETER(e);

    winrt::Microsoft::UI::Windowing::AppWindow appWindow = this->AppWindow();
    bool ans = appWindow.IsVisible();
    textBlockMUXControls().Text(L"AppWindow api : " + winrt::to_hstring(ans));
}
