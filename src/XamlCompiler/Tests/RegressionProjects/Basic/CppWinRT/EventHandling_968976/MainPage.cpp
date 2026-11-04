// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// Implementation of the MainPage class.
//

#include "pch.h"
#include "MainPage.h"
#include "MainPage.g.cpp"

namespace winrt::EventHandling_968976::implementation
{
    MainPage::MainPage()
    {
        InitializeComponent();
    }

    void MainPage::FirstHandler(winrt::array_view<uint32_t const> /* args */)
    {
    }

    void MainPage::SecondHandler(winrt::array_view<uint32_t> /* args */)
    {
    }

    void MainPage::ThirdHandler(winrt::com_array<uint32_t>& /* args */)
    {
    }

    void MainPage::FourthHandler(winrt::hstring& /* args */)
    {
    }

    void MainPage::GuidCharPassHandler(
        winrt::array_view<winrt::guid const> /* guidArgs */,
        winrt::array_view<char16_t const> /* charArgs */)
    {
    }

    void MainPage::GuidCharFillHandler(
        winrt::array_view<winrt::guid> /* guidArgs */,
        winrt::array_view<char16_t> /* charArgs */)
    {
    }

    void MainPage::GuidCharReceiveHandler(
        winrt::com_array<winrt::guid>& /* guidArgs */,
        winrt::com_array<char16_t>& /* charArgs */)
    {
    }
}
