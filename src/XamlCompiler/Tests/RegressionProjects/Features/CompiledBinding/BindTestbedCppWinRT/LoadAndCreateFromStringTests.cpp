// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "LoadAndCreateFromStringTests.h"
#include "LoadAndCreateFromStringTests.g.cpp"
#include "DetectLeaksPage.h"

namespace winrt::BindTestbed::implementation
{
    LoadAndCreateFromStringTests::LoadAndCreateFromStringTests()
    {
        InitializeComponent();
        InitializeValues();
        DetectLeaksPage::TrackObject(*this, xaml_typename<BindTestbed::LoadAndCreateFromStringTests>().Name);
    }

    void LoadAndCreateFromStringTests::InitializeValues()
    {
    }

    void LoadAndCreateFromStringTests::UpdateValuesClick(IInspectable const&, wux::RoutedEventArgs const&)
    {
        Model().UpdateValues();
        DOModel().UpdateValues();
    }

    void LoadAndCreateFromStringTests::ResetValuesClick(IInspectable const&, wux::RoutedEventArgs const&)
    {
        Model().InitializeValues();
        Model().UpdateValues();
        Model().InitializeValues();
    }

    void LoadAndCreateFromStringTests::Button_Click(IInspectable const&, wux::RoutedEventArgs const&)
    {
        FindName(L"AnUnloadedTextBlock");
    }

    void LoadAndCreateFromStringTests::LoadInnerPanel_Click(IInspectable const&, wux::RoutedEventArgs const&)
    {
        FindName(L"InnerPanel");
    }


    void LoadAndCreateFromStringTests::UnloadInnerPanel_Click(IInspectable const&, wux::RoutedEventArgs const&)
    {
        UnloadObject(InnerPanel());
    }


    void LoadAndCreateFromStringTests::LoadOuterPanel_Click(IInspectable const&, wux::RoutedEventArgs const&)
    {
        FindName(L"OuterPanel");
    }


    void LoadAndCreateFromStringTests::UnloadOuterPanel_Click(IInspectable const&, wux::RoutedEventArgs const&)
    {
        UnloadObject(OuterPanel());
    }
}