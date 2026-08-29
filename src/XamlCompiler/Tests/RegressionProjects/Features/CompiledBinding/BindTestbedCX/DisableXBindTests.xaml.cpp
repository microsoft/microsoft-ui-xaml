// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// DisableXBindTests.xaml.cpp
// Implementation of the DisableXBindTests class
//

#include "pch.h"
#include "DisableXBindTests.xaml.h"
#include "DetectLeaksPage.xaml.h"

using namespace BindTestbedCX;

using namespace Platform;
using namespace ::Windows::Foundation;
using namespace ::Windows::Foundation::Collections;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Controls::Primitives;
using namespace Microsoft::UI::Xaml::Data;
using namespace Microsoft::UI::Xaml::Input;
using namespace Microsoft::UI::Xaml::Interop;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Navigation;
using namespace Microsoft::UI::Xaml::Markup;
using namespace ::Windows::UI::Popups;

// The User Control item template is documented at https://go.microsoft.com/fwlink/?LinkId=234236

DisableXBindTests::DisableXBindTests()
{
	InitializeComponent();
    DetectLeaksPage::TrackObject(this, DisableXBindTests::GetType()->FullName);
}

void DisableXBindTests::Click_RegularArgs(Platform::Object^ sender, Microsoft::UI::Xaml::RoutedEventArgs^ e)
{
    MessageDialog^ dlg = ref new MessageDialog("Regular arguments clicked");
    auto t = dlg->ShowAsync();
}

void DisableXBindTests::Click_NoArgs()
{
    MessageDialog^ dlg = ref new MessageDialog("No argument Clicked");
    auto t = dlg->ShowAsync();
}

void DisableXBindTests::On_Loaded(Platform::Object^ sender, Microsoft::UI::Xaml::RoutedEventArgs^ e)
{
   IXamlBindScopeDiagnostics^ bindingsAsDiag = (IXamlBindScopeDiagnostics^)this->Bindings;
    for (int lineNumber = 0; lineNumber < 40; lineNumber++)
    {
        for (int columnNumber = 0; columnNumber < 100; columnNumber++)
        {
            bindingsAsDiag->Disable(lineNumber, columnNumber);
        }
    }
}
