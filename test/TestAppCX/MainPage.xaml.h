// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//
// MainPage.xaml.h
// Declaration of the MainPage class.
//

#pragma once

#include "MainPage.g.h"

using namespace Windows::UI::Xaml::Navigation;

namespace TestAppCX
{
    public ref class MainPage sealed
    {
    public:
        MainPage();

        void OnNavigatedTo(NavigationEventArgs^ e) override;

    private:
        void GoToLeakTestControlPage(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
        void GoToMenuBarTestPage(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
        void GoToCornerRadiusTestPage(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
        void GoToTreeViewTestPage(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
        void GoToBackdropMaterialTestPage(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
    };
}
