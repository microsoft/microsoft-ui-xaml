// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "MainPage.h"
#include "MainPage.g.cpp"
#include "DetectLeaksPage.h"

namespace winrt::BindTestbed::implementation
{
    MainPage::MainPage()
    {
        _mainModel = make<BindTestbed::implementation::MainModel>();
        auto canAccessModel = MainModel().Model().StringPropNoINPC().size();
        canAccessModel;
        InitializeComponent();
        DetectLeaksPage::TrackObject(*this, xaml_typename<BindTestbed::MainPage>().Name);
    }

    void MainPage::DetectLeaks_Click(IInspectable const&, wux::RoutedEventArgs const&)
    {
        Frame().Navigate(xaml_typename<BindTestbed::DetectLeaksPage>());
    }
}