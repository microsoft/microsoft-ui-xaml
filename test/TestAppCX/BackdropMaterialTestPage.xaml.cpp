//
// BlankPage.xaml.cpp
// Implementation of the BlankPage class
//

#include "pch.h"
#include "BackdropMaterialTestPage.xaml.h"

using namespace TestAppCX;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::UI::Core;
using namespace Windows::UI::ViewManagement;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;

// The Blank Page item template is documented at https://go.microsoft.com/fwlink/?LinkId=234238

BackdropMaterialTestPage::BackdropMaterialTestPage()
{
	InitializeComponent();
}

void BackdropMaterialTestPage::OnNavigatedTo(NavigationEventArgs^ e)
{
    auto coreTitleBar = CoreApplication::GetCurrentView()->TitleBar;
    auto titleBar = ApplicationView::GetForCurrentView()->TitleBar;

    // Set transparent background for base layer brush to show through
    titleBar->ButtonBackgroundColor = Windows::UI::Colors::Transparent;
    titleBar->ButtonInactiveBackgroundColor = Windows::UI::Colors::Transparent;

    coreTitleBar->ExtendViewIntoTitleBar = true;

    Window::Current->SetTitleBar(AppTitleBar);

    UpdateTitleBar(coreTitleBar);

    m_layoutMetricsChangedToken = coreTitleBar->LayoutMetricsChanged += ref new Windows::Foundation::TypedEventHandler<Windows::ApplicationModel::Core::CoreApplicationViewTitleBar^, Platform::Object^>(this, &TestAppCX::BackdropMaterialTestPage::CoreTitleBar_LayoutMetricsChanged);

    auto coreWindow = CoreWindow::GetForCurrentThread();
    m_activatedToken = coreWindow->Activated += ref new Windows::Foundation::TypedEventHandler<CoreWindow^, WindowActivatedEventArgs^>(this, &BackdropMaterialTestPage::OnActivated);

    TitleBarText->RegisterPropertyChangedCallback(TextBlock::ForegroundProperty,
        ref new DependencyPropertyChangedCallback(this, &BackdropMaterialTestPage::TitleBarForegroundPropertyChanged));
}

void BackdropMaterialTestPage::OnNavigatingFrom(NavigatingCancelEventArgs^ e)
{
    auto coreWindow = CoreWindow::GetForCurrentThread();
    coreWindow->Activated -= m_activatedToken;

    auto coreTitleBar = CoreApplication::GetCurrentView()->TitleBar;
    coreTitleBar->LayoutMetricsChanged -= m_layoutMetricsChangedToken;
}

void BackdropMaterialTestPage::TitleBarForegroundPropertyChanged(DependencyObject^ object, DependencyProperty^ dp)
{
    auto color = safe_cast<SolidColorBrush^>(TitleBarText->Foreground)->Color;

    auto titleBar = ApplicationView::GetForCurrentView()->TitleBar;
    titleBar->ButtonForegroundColor = color;
    titleBar->ButtonHoverForegroundColor = color;
    titleBar->ButtonPressedForegroundColor = color;
    titleBar->ForegroundColor = color;
}

void BackdropMaterialTestPage::CoreTitleBar_LayoutMetricsChanged(CoreApplicationViewTitleBar^ sender, Object^ args)
{
    UpdateTitleBar(sender);
}


void BackdropMaterialTestPage::UpdateTitleBar(CoreApplicationViewTitleBar^ coreTitleBar)
{
    auto height = coreTitleBar->Height;
    AppTitleBar->Height = height;

    LeftPaddingColumn->Width = GridLength(coreTitleBar->SystemOverlayLeftInset);
    RightPaddingColumn->Width = GridLength(coreTitleBar->SystemOverlayRightInset);
}


void TestAppCX::BackdropMaterialTestPage::ToggleTheme(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    auto contentAsFrameworkElement = safe_cast<FrameworkElement^>(Window::Current->Content);
    if (contentAsFrameworkElement->RequestedTheme == ElementTheme::Default)
    {
        // Convert theme from default to either dark or light based on application requestedtheme
        contentAsFrameworkElement->RequestedTheme = (Application::Current->RequestedTheme == ApplicationTheme::Light) ? ElementTheme::Light : ElementTheme::Dark;
    }
    // Invert theme
    contentAsFrameworkElement->RequestedTheme = (contentAsFrameworkElement->RequestedTheme == ElementTheme::Light) ? ElementTheme::Dark : ElementTheme::Light;

    // Reset material to defaults
    Microsoft::UI::Xaml::Controls::BackdropMaterial::SetApplyToRootOrPageBackground(this, false);
    Microsoft::UI::Xaml::Controls::BackdropMaterial::SetApplyToRootOrPageBackground(this, true);

    TintOpacitySlider->Value = Microsoft::UI::Private::Controls::BackdropMaterialTestApi::TintOpacity;
}

void BackdropMaterialTestPage::OnActivated(CoreWindow^ sender, WindowActivatedEventArgs^ args)
{
    auto window = CoreWindow::GetForCurrentThread();
    if (window->ActivationMode == CoreWindowActivationMode::ActivatedInForeground
        || window->ActivationMode == CoreWindowActivationMode::ActivatedNotForeground)
    {
        VisualStateManager::GoToState(this, "Active", true);
    }
    else
    {
        VisualStateManager::GoToState(this, "Inactive", true);
    }
}
