// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// Shutdown.xaml.h
// Declaration of the Shutdown class
//

#pragma once

#include "DetectLeaksPage.g.h"

namespace BindTestbedCX
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    [::Windows::Foundation::Metadata::WebHostHidden]
    public ref class DetectLeaksPage sealed
    {
    public:
        property int NumberOfObjectsLeaked;
        property int NumberOfObjectsTested;
        DetectLeaksPage();
        void BackButton_Click(Platform::Object^ sender, Microsoft::UI::Xaml::RoutedEventArgs^ e);
        static void TrackObject(Object^ object, ::Platform::String^ name);

    private:
        void Button_Click(Platform::Object^ sender, Microsoft::UI::Xaml::RoutedEventArgs^ e);
        void DetectLeakedObjects_Click(Platform::Object^ sender, Microsoft::UI::Xaml::RoutedEventArgs^ e);
        void UpdateValues(Platform::Object^ sender, Microsoft::UI::Xaml::RoutedEventArgs^ e);
    };
}
