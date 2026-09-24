// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// TwoWayTests.xaml.cpp
// Implementation of the TwoWayTests class.
//

#include "pch.h"
#include "TwoWayTests.xaml.h"
#include "MainPage.xaml.h"
#include "DetectLeaksPage.xaml.h"

using namespace BindTestbed;

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

DependencyProperty^ TwoWayTests::_DPOnPageProperty = DependencyProperty::Register("DPOnPage", ::Windows::UI::Xaml::Interop::TypeName(::Platform::String::typeid), ::Windows::UI::Xaml::Interop::TypeName(TwoWayTests::typeid), nullptr);

TwoWayTests::TwoWayTests()
{
    InitializeComponent();
    DetectLeaksPage::TrackObject(this, TwoWayTests::GetType()->FullName);
}

void TwoWayTests::UpdateValuesClick(Object^ sender, ::Microsoft::UI::Xaml::RoutedEventArgs^ e)
{
    this->Model->UpdateValues();
    this->DOModel->UpdateValues();
}
void TwoWayTests::ResetValuesClick(Object^ sender, ::Microsoft::UI::Xaml::RoutedEventArgs^ e)
{
    this->Model->InitializeValues();
    this->DOModel->UpdateValues();
}
