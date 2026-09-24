// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "DetectLeaksPage.g.h"

namespace winrt::BindTestbed::implementation
{
    struct DetectLeaksPage : DetectLeaksPageT<DetectLeaksPage>
    {
        DetectLeaksPage();

        void BackButton_Click(IInspectable const&, wux::RoutedEventArgs const&);
        void DetectLeakedObjects_Click(IInspectable const&, wux::RoutedEventArgs const&);
        void UpdateValues(IInspectable const&, wux::RoutedEventArgs const&);

        static void TrackObject(IInspectable const&, hstring const&);
    };
}

namespace winrt::BindTestbed::factory_implementation
{
    struct DetectLeaksPage : DetectLeaksPageT<DetectLeaksPage, implementation::DetectLeaksPage>
    {
    };
}
