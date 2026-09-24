// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "MyUserControl1.h"
#include "MyUserControl1.g.cpp"
#include "DetectLeaksPage.h"

namespace winrt::BindTestbed::implementation
{
    wux::DependencyProperty MyUserControl1::property1Property =
        RegisterDependencyProperty(
            L"Property1Property",
            xaml_typename<hstring>(),
            xaml_typename<BindTestbed::MyUserControl1>(),
            nullptr);

    MyUserControl1::MyUserControl1()
    {
        InitializeComponent();
        DetectLeaksPage::TrackObject(*this, xaml_typename<BindTestbed::MyUserControl1>().Name);
    }

    void MyUserControl1::aLazyTextBlock_Tapped(IInspectable const&, wux::Input::TappedRoutedEventArgs const&)
    {}
}