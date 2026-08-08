// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "FunctionTests.xaml.h"
#include "DetectLeaksPage.xaml.h"

using namespace BindTestbed;
using namespace Platform;

FunctionTests::FunctionTests()
{
    InitializeComponent();
    InitializeValues();
    DetectLeaksPage::TrackObject(this, FunctionTests::GetType()->FullName);
}

void FunctionTests::InitializeValues()
{
}

void FunctionTests::UpdateValuesClick(Object^ sender, ::Microsoft::UI::Xaml::RoutedEventArgs^ e)
{
    this->Model->UpdateValues();
    this->DOModel->UpdateValues();
}

void FunctionTests::ResetValuesClick(Object^ sender, ::Microsoft::UI::Xaml::RoutedEventArgs^ e)
{
    this->Model->InitializeValues();
    this->DOModel->UpdateValues();
    this->InitializeValues();
}

void FunctionTests::StopTrackingClick(Object^ sender, ::Microsoft::UI::Xaml::RoutedEventArgs^ e)
{
    this->Bindings->StopTracking();
}

void FunctionTests::ReInitializeBindingsClick(Object^ sender, ::Microsoft::UI::Xaml::RoutedEventArgs^ e)
{
    this->Bindings->Initialize();
}