// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// DefaultBindModeTests.xaml.h
// Declaration of the DefaultBindModeTests class
//

#pragma once

#include "DefaultBindModeTests.g.h"

namespace BindTestbed
{
	[::Windows::Foundation::Metadata::WebHostHidden]
	public ref class DefaultBindModeTests sealed
	{
	public:
		DefaultBindModeTests();
        property BindTestbedModel::DataModel^ Model;
        property BindTestbedModel::DOModel^ DOModel;

        void InitializeValues();
        void UpdateValuesClick(Object^ sender, ::Microsoft::UI::Xaml::RoutedEventArgs^ e);
        void ResetValuesClick(Object^ sender, ::Microsoft::UI::Xaml::RoutedEventArgs^ e);
	};
}
