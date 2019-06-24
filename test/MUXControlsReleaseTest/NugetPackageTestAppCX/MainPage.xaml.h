// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "MainPage.g.h"

namespace NugetPackageTestAppCX
{
    public ref class MainPage sealed
    {
    public:
        MainPage();

    private:
        void CloseAppInvokerButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
        void PageLoaded(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
        void OnAddItemsButtonClick(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);

        Platform::Collections::Vector<Platform::String^>^ mItems;
        void WaitForIdleInvokerButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
    };
}
