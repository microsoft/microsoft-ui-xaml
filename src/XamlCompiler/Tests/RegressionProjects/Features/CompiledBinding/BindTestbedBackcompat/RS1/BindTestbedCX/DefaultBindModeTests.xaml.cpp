// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "DefaultBindModeTests.xaml.h"
#include "DetectLeaksPage.xaml.h"

using namespace BindTestbed;

DefaultBindModeTests::DefaultBindModeTests()
{
    InitializeComponent();
    InitializeValues();
    DetectLeaksPage::TrackObject(this, DefaultBindModeTests::GetType()->FullName);
}

void DefaultBindModeTests::InitializeValues()
{
}

void DefaultBindModeTests::UpdateValuesClick(Platform::Object^ sender, ::Microsoft::UI::Xaml::RoutedEventArgs^ e)
{
    this->Model->UpdateValues();
    this->DOModel->UpdateValues();
}

void DefaultBindModeTests::ResetValuesClick(Platform::Object^ sender, ::Microsoft::UI::Xaml::RoutedEventArgs^ e)
{
    this->Model->InitializeValues();
    this->DOModel->UpdateValues();
    this->InitializeValues();
}