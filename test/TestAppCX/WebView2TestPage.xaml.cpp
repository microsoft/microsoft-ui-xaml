//
// BlankPage.xaml.cpp
// Implementation of the BlankPage class
//

#include "pch.h"
#include "WebView2TestPage.xaml.h"
#include "App.xaml.h"

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

WebView2TestPage::WebView2TestPage()
{
	InitializeComponent();
}

void WebView2TestPage::OnGo(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    try
    {
       wv2->Source = ref new Uri(url->Text);
    }
    catch (...)
    {
       // Eat bad Uris
    }
}
