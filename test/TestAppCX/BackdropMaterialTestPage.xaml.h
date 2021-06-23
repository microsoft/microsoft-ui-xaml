//
// BlankPage.xaml.h
// Declaration of the BlankPage class
//

#pragma once

#include "BackdropMaterialTestPage.g.h"

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

namespace TestAppCX
{
	/// <summary>
	/// An empty page that can be used on its own or navigated to within a Frame.
	/// </summary>
	[Windows::Foundation::Metadata::WebHostHidden]
	public ref class BackdropMaterialTestPage sealed
	{
	public:
        BackdropMaterialTestPage();
        
        void OnNavigatedTo(NavigationEventArgs^ e) override;
        void OnNavigatingFrom(NavigatingCancelEventArgs^ e) override;

        void OnActivated(CoreWindow^ sender, WindowActivatedEventArgs^ args);

        void CoreTitleBar_LayoutMetricsChanged(CoreApplicationViewTitleBar^ sender, Object^ args);
        void UpdateTitleBar(CoreApplicationViewTitleBar^ coreTitleBar);

        void TitleBarForegroundPropertyChanged(DependencyObject^ object, DependencyProperty^ dp);

        void OnLoaded(Object^ object, RoutedEventArgs^ args);
    private:
        void ToggleTheme(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);

        Windows::Foundation::EventRegistrationToken m_layoutMetricsChangedToken;
        Windows::Foundation::EventRegistrationToken m_activatedToken;
        void NewView(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
        void OnBackRequested(Microsoft::UI::Xaml::Controls::NavigationView^ sender, Microsoft::UI::Xaml::Controls::NavigationViewBackRequestedEventArgs^ args);
        void CloseWindow(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
    };
}
