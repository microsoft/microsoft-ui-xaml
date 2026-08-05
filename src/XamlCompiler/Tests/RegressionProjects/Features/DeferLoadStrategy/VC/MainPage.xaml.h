// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// MainPage.xaml.h
// Declaration of the MainPage class.
//

#pragma once

#include "MainPage.g.h"

namespace VC
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public ref class MainPage sealed
    {
    public:
        MainPage();
        property int CurrentYearAsInteger { int get() { return 2015;  } }

    private:
        void Button_Click(Platform::Object^ sender, Microsoft::UI::Xaml::RoutedEventArgs^ e);
        void deferred_Click(Platform::Object^ sender, Microsoft::UI::Xaml::RoutedEventArgs^ e);
    };
}
