// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// MainPage.xaml.cpp
// Implementation of the MainPage class.
//

#include "pch.h"
#include "MainPage.xaml.h"
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
using namespace ::Windows::UI::Popups;
using namespace concurrency;

MainPage::MainPage()
{
    this->MainModel = ref new BindTestbedCX::MainModel();
    InitializeComponent();
    DetectLeaksPage::TrackObject(this, MainPage::GetType()->FullName);
}

void MainPage::DetectLeaks_Click(Platform::Object^ sender, Microsoft::UI::Xaml::RoutedEventArgs^ e)
{
    Frame->Navigate(DetectLeaksPage::typeid);
}
