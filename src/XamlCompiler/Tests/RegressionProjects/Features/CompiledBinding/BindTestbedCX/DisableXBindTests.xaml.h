// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// DisableXBindTests.xaml.h
// Declaration of the DisableXBindTests class
//

#pragma once

#include "DisableXBindTests.g.h"

namespace BindTestbedCX
{
	[::Windows::Foundation::Metadata::WebHostHidden]
	public ref class DisableXBindTests sealed
	{
	public:
		DisableXBindTests();
        property BindTestbedModel::DataModel^ Model;
        property BindTestbedModel::DOModel^ DOModel;

        void Click_RegularArgs(Platform::Object^ sender, Microsoft::UI::Xaml::RoutedEventArgs^ e);
        void Click_NoArgs();
        void On_Loaded(Platform::Object^ sender, Microsoft::UI::Xaml::RoutedEventArgs^ e);
	};
}
