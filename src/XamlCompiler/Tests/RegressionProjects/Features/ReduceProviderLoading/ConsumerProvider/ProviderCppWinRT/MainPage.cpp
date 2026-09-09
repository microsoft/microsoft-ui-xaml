// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// Implementation of the MainPage class.
//

#include "pch.h"
#include "MainPage.h"
#include "MainPage.g.cpp"

namespace winrt::ProviderCppWinRT::implementation
{
    MainPage::MainPage()
    {
        InitializeComponent();
    }

    void MainPage::DoSomething()
    {
        return;
    }

    hstring MainPage::GetTextToShow()
    {
        return L"Hello from Provider";
    }
}
