// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "LoadAndCreateFromStringTests.xaml.h"
#include "DetectLeaksPage.xaml.h"

using namespace BindTestbed;

LoadAndCreateFromStringTests::LoadAndCreateFromStringTests()
{
    InitializeComponent();
    InitializeValues();
    DetectLeaksPage::TrackObject(this, LoadAndCreateFromStringTests::GetType()->FullName);
}

void LoadAndCreateFromStringTests::InitializeValues()
{
}

void LoadAndCreateFromStringTests::UpdateValuesClick(Object^ sender, ::Microsoft::UI::Xaml::RoutedEventArgs^ e)
{
    this->Model->UpdateValues();
    this->DOModel->UpdateValues();
}

void LoadAndCreateFromStringTests::ResetValuesClick(Object^ sender, ::Microsoft::UI::Xaml::RoutedEventArgs^ e)
{
    this->Model->InitializeValues();
    this->DOModel->UpdateValues();
    this->InitializeValues();
}

/*
void LoadAndCreateFromStringTests::Button_Click(Platform::Object^ sender, Microsoft::UI::Xaml::RoutedEventArgs^ e)
{
    this->FindName("AnUnloadedTextBlock");
}

void LoadAndCreateFromStringTests::LoadInnerPanel_Click(Platform::Object^ sender, Microsoft::UI::Xaml::RoutedEventArgs^ e)
{
    this->FindName(L"InnerPanel");
}


void LoadAndCreateFromStringTests::UnloadInnerPanel_Click(Platform::Object^ sender, Microsoft::UI::Xaml::RoutedEventArgs^ e)
{
    this->UnloadObject(this->InnerPanel);
}


void LoadAndCreateFromStringTests::LoadOuterPanel_Click(Platform::Object^ sender, Microsoft::UI::Xaml::RoutedEventArgs^ e)
{
    this->FindName(L"OuterPanel");
}


void LoadAndCreateFromStringTests::UnloadOuterPanel_Click(Platform::Object^ sender, Microsoft::UI::Xaml::RoutedEventArgs^ e)
{
    this->UnloadObject(this->OuterPanel);
}
*/