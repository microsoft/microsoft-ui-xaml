// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//
// MainPage.xaml.cpp
// Implementation of the MainPage class.
//

#include "pch.h"
#include "MainPage.xaml.h"

using namespace NugetPackageTestAppCX;

using namespace Platform;
using namespace Platform::Collections;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Automation;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;

// The Blank Page item template is documented at https://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x409

MainPage::MainPage() :
    mItems(ref new Vector<String^>())
{
	InitializeComponent();

    Repeater->ItemsSource = mItems;

    AutomationProperties::SetName(this, L"MainPage");
}

void NugetPackageTestAppCX::MainPage::CloseAppInvokerButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    Application::Current->Exit();
}

void NugetPackageTestAppCX::MainPage::PageLoaded(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    TestContentLoadedCheckBox->IsChecked = true;
}

void NugetPackageTestAppCX::MainPage::OnAddItemsButtonClick(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    mItems->Append(L"Item1");
    mItems->Append(L"Item2");
    mItems->Append(L"Item3");
}
