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
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Controls::Primitives;
using namespace Microsoft::UI::Xaml::Data;
using namespace Microsoft::UI::Xaml::Input;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Navigation;

// The Blank Page item template is documented at https://go.microsoft.com/fwlink/?LinkId=234238

WebView2TestPage::WebView2TestPage()
{
    InitializeComponent();
}

void WebView2TestPage::OnGo(Platform::Object^ sender, Microsoft::UI::Xaml::RoutedEventArgs^ e)
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
