// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// MainPage.xaml.h
// Declaration of the MainPage class.
//

#pragma once

#include "MainPage.g.h"

namespace MetadataTestbedCX
{
    public ref class MainPage sealed
    {
    private:
        ::XamlTypeInfo::InfoProvider::XamlTypeInfoProvider ^ provider{};

    public:
        MainPage();

        property Object^ TestProperty { Object^ get() { return nullptr; } }
        void GetTypeMemberManyTimesClicked(Object^ sender, Microsoft::UI::Xaml::RoutedEventArgs^ e);
        void Test();
        void MainPage::GetTypeMemberTest();
    };
}
