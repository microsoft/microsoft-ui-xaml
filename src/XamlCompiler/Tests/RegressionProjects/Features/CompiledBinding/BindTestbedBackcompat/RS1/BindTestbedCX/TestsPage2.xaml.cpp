// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// TestsPage2.xaml.cpp
// Implementation of the TestsPage2 class.
//

#include "pch.h"
#include "TestsPage2.xaml.h"
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

DependencyProperty^ TestsPage2::_DPOnPageProperty = DependencyProperty::Register("DPOnPage", ::Windows::UI::Xaml::Interop::TypeName(::Platform::String::typeid), ::Windows::UI::Xaml::Interop::TypeName(TestsPage2::typeid), nullptr);

TestsPage2::TestsPage2()
{
    auto v = ref new Platform::Collections::Vector<String^>();
    InitializeComponent();
    InitializeValues();
    v->Append(L"A");
    v->Append(L"A");
    v->Append(L"A");

    cb->ItemsSource = v;
    DetectLeaksPage::TrackObject(this, TestsPage2::GetType()->FullName);
}

void TestsPage2::InitializeValues()
{
    this->DPOnPage = "DP on page";
    IntPropNoINPC = 42;
    this->ImageUriString = "http://static-hp-wus.s-msn.com/sc/homepage/i/65/e8a77758e8644573ba5d41ada16e8c.jpg";
}

void TestsPage2::UpdateValuesClick(Object^ sender, ::Microsoft::UI::Xaml::RoutedEventArgs^ e)
{
    this->Model->UpdateValues();
    this->DOModel->UpdateValues();

    IntPropNoINPC += 3;
    this->DPOnPage += "-";

    if (this->Model->IntPropWithINPC % 5 == 0)
    {
        Grid::SetColumn(BisqueRectangle, 1);
    }
}
void TestsPage2::ResetValuesClick(Object^ sender, ::Microsoft::UI::Xaml::RoutedEventArgs^ e)
{
    this->Model->InitializeValues();
    this->DOModel->UpdateValues();
    this->InitializeValues();
}

void TestsPage2::UndeferElementClick(Object^ sender, ::Microsoft::UI::Xaml::RoutedEventArgs^ e)
{
    FindName("deferedTextBlock");
}