// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestsPage2.h"
#include "DetectLeaksPage.h"

namespace winrt::BindTestbed::implementation
{
    wux::DependencyProperty TestsPage2::dpOnPageProperty =
        RegisterDependencyProperty(
            L"DPOnPage",
            xaml_typename<hstring>(),
            xaml_typename<BindTestbed::TestsPage2>(),
            nullptr);

    TestsPage2::TestsPage2()
    {
        auto v = single_threaded_observable_vector<IInspectable>();
        InitializeComponent();
        InitializeValues();
        v.Append(box_value(L"A"));
        v.Append(box_value(L"B"));
        v.Append(box_value(L"C"));
        cb().ItemsSource(v);
        DetectLeaksPage::TrackObject(*this, xaml_typename<BindTestbed::TestsPage2>().Name);
    }

    void TestsPage2::InitializeValues()
    {
        DPOnPage(L"DP on page");
        IntPropNoINPC(42);
        ImageUriString(L"http://static-hp-wus.s-msn.com/sc/homepage/i/65/e8a77758e8644573ba5d41ada16e8c.jpg");
    }

    void TestsPage2::UpdateValuesClick(IInspectable const&, wux::RoutedEventArgs const&)
    {
        Model().UpdateValues();
        DOModel().UpdateValues();

        IntPropNoINPC(IntPropNoINPC() + 3);
        std::wstring s = DPOnPage().c_str();
        DPOnPage(s.append(L"-").c_str());

        if (Model().IntPropWithINPC() % 5 == 0)
        {
            wuxc::Grid::SetColumn(BisqueRectangle(), 1);
        }
    }

    void TestsPage2::ResetValuesClick(IInspectable const&, wux::RoutedEventArgs const&)
    {
        Model().InitializeValues();
        DOModel().UpdateValues();
        InitializeValues();
    }

    void TestsPage2::UndeferElementClick(IInspectable const&, wux::RoutedEventArgs const&)
    {
        FindName(L"deferedTextBlock");
    }
}