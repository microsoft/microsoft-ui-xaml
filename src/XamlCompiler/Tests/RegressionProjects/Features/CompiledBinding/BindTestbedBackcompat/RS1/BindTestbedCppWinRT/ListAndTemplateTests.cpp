// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ListAndTemplateTests.h"
#include "DetectLeaksPage.h"

namespace winrt::BindTestbed::implementation
{
    ListAndTemplateTests::ListAndTemplateTests()
    {
        InitializeComponent();
        DetectLeaksPage::TrackObject(*this, xaml_typename<BindTestbed::ListAndTemplateTests>().Name);
    }

    void ListAndTemplateTests::UpdateValuesClick(IInspectable const&, wux::RoutedEventArgs const&)
    {
        Model().UpdateValues();
        DOModel().UpdateValues();
        ModelCX().UpdateValues();
        auto temp = Employees().GetAt(0);
        Employees().RemoveAt(0);
        Employees().Append(temp);
    }

    void ListAndTemplateTests::ResetValuesClick(IInspectable const&, wux::RoutedEventArgs const&)
    {
        Model().InitializeValues();
        DOModel().UpdateValues();
        ModelCX().InitializeValues();
    }
}