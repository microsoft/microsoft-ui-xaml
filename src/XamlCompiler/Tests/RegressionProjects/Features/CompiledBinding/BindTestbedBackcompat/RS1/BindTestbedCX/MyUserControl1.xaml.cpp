// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// MyUserControl.xaml.cpp
// Implementation of the MyUserControl class
//

#include "pch.h"
#include "MyUserControl1.xaml.h"
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

// The User Control item template is documented at http://go.microsoft.com/fwlink/?LinkId=234236

DependencyProperty^ MyUserControl1::_Property1Property = DependencyProperty::Register("Property1Property", ::Windows::UI::Xaml::Interop::TypeName(::Platform::String::typeid), ::Windows::UI::Xaml::Interop::TypeName(MyUserControl1::typeid), nullptr);

MyUserControl1::MyUserControl1()
{
    this->InitializeComponent();
    DetectLeaksPage::TrackObject(this, MyUserControl1::GetType()->FullName);
}


void BindTestbed::MyUserControl1::aLazyTextBlock_Tapped(Platform::Object^ sender, Microsoft::UI::Xaml::Input::TappedRoutedEventArgs^ e)
{
}
