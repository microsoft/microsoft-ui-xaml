// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// BasicTests.xaml.cpp
// Implementation of the BasicTests class.
//

#include "pch.h"
#include "BasicTests.xaml.h"
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

DependencyProperty^ BasicTests::_DPOnPageProperty = DependencyProperty::Register("DPOnPage", ::Windows::UI::Xaml::Interop::TypeName(::Platform::String::typeid), ::Windows::UI::Xaml::Interop::TypeName(BasicTests::typeid), nullptr);

BasicTests::BasicTests()
{
	InitializeComponent();
	InitializeValues();
	DetectLeaksPage::TrackObject(this, BasicTests::GetType()->FullName);
}

void BasicTests::InitializeValues()
{
	this->DPOnPage = "DP on page";
	this->NonDPOnPage = "Non DP on Page";
}

void BasicTests::UpdateValuesClick(Object^ sender, ::Microsoft::UI::Xaml::RoutedEventArgs^ e)
{
	this->Model->UpdateValues();
	this->DOModel->UpdateValues();
	this->DPOnPage += "-";
}

void BasicTests::ResetValuesClick(Object^ sender, ::Microsoft::UI::Xaml::RoutedEventArgs^ e)
{
	this->Model->InitializeValues();
	this->DOModel->UpdateValues();
	this->InitializeValues();
}

void BasicTests::StopTrackingClick(Object^ sender, ::Microsoft::UI::Xaml::RoutedEventArgs^ e)
{
	this->Bindings->StopTracking();
}

void BasicTests::ReInitializeBindingsClick(Object^ sender, ::Microsoft::UI::Xaml::RoutedEventArgs^ e)
{
	this->Bindings->Initialize();
}