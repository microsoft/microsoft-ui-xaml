// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// EventTests.xaml.cpp
// Implementation of the EventTests class.
//

#include "pch.h"
#include "EventTests.xaml.h"
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

EventTests::EventTests()
{
	this->clickDelegate = ref new RoutedEventHandler(this, &EventTests::Click_RegularArgs);
	InitializeComponent();
	InitializeValues();
	DetectLeaksPage::TrackObject(this, EventTests::GetType()->FullName);
}

void EventTests::InitializeValues()
{
}

void EventTests::UpdateValuesClick(Object^ sender, ::Microsoft::UI::Xaml::RoutedEventArgs^ e)
{
	this->Model->UpdateValues();
	this->DOModel->UpdateValues();
}

void EventTests::ResetValuesClick(Object^ sender, ::Microsoft::UI::Xaml::RoutedEventArgs^ e)
{
	this->Model->InitializeValues();
	this->DOModel->UpdateValues();
}

void EventTests::Click_RegularArgs(Platform::Object^ sender, Microsoft::UI::Xaml::RoutedEventArgs^ e)
{
	MessageDialog^ dlg = ref new MessageDialog("Regular arguments clicked");
	auto t = dlg->ShowAsync();
}

void EventTests::Click_NoArgs()
{
	MessageDialog^ dlg = ref new MessageDialog("No argument Clicked");
	auto t = dlg->ShowAsync();
}

void EventTests::Click_BaseArgs(Platform::Object^ sender, Microsoft::UI::Xaml::RoutedEventArgs^ e)
{
	MessageDialog^ dlg = ref new MessageDialog("Base argument clicked");
	auto t = dlg->ShowAsync();
}

void EventTests::Click_OverloadedArgs()
{
	MessageDialog^ dlg = ref new MessageDialog("Overloaded argument clicked");
	auto t = dlg->ShowAsync();
}

void EventTests::Click_OverloadedArgs(Platform::Object^ sender, Platform::Object^ e)
{
	MessageDialog^ dlg = ref new MessageDialog("Overloaded argument clicked");
	auto t = dlg->ShowAsync();
}