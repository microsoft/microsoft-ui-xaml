// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// NameEventsLoad.xaml.h
// Declaration of the NameEventsLoad class
//

#pragma once

#include "NameEventsLoad.g.h"

namespace Conditionals
{
	/// <summary>
	/// An empty page that can be used on its own or navigated to within a Frame.
	/// </summary>
	[::Windows::Foundation::Metadata::WebHostHidden]
	public ref class NameEventsLoad sealed
	{
	public:
		NameEventsLoad();

    private:
        void Button_Click(Platform::Object^ sender, Microsoft::UI::Xaml::RoutedEventArgs^ e);
        void Button_Click_V1(Platform::Object^ sender, Microsoft::UI::Xaml::RoutedEventArgs^ e);
        void Button_Click_V2(Platform::Object^ sender, Microsoft::UI::Xaml::RoutedEventArgs^ e);
        void Button_Click_V3(Platform::Object^ sender, Microsoft::UI::Xaml::RoutedEventArgs^ e);
        void Button_Click_notV3(Platform::Object^ sender, Microsoft::UI::Xaml::RoutedEventArgs^ e);
    };
}
