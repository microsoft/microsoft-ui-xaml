// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "StarCppWinRT.g.h"

namespace winrt::BindTestbedCppWinRTModel::implementation
{
    struct StarCppWinRT : StarCppWinRTT<StarCppWinRT>
    {
        StarCppWinRT() = default;

        static winrt::Microsoft::UI::Xaml::DependencyProperty CoordsProperty();
        static winrt::Microsoft::UI::Xaml::DependencyProperty TestStringProperty();
        winrt::BindTestbedCppWinRTModel::Coordinates Coords();
        void Coords(winrt::BindTestbedCppWinRTModel::Coordinates const& value);
        hstring TestString();
        void TestString(hstring const& value);
    };
}
namespace winrt::BindTestbedCppWinRTModel::factory_implementation
{
    struct StarCppWinRT : StarCppWinRTT<StarCppWinRT, implementation::StarCppWinRT>
    {
    };
}
