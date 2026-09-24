// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// NullableTests.xaml.h
// Declaration of the NullableTests class
//

#pragma once

#include "NullableTests.g.h"

namespace BindTestbedCX
{
	/// <summary>
	/// An empty page that can be used on its own or navigated to within a Frame.
	/// </summary>
	[::Windows::Foundation::Metadata::WebHostHidden]
	public ref class NullableTests sealed
	{
	public:
		NullableTests();
        property BindTestbedModel::DataModel^ Model;
        property BindTestbedModel::DOModel^ DOModel;
        void UpdateValuesClick(Object^ sender, ::Microsoft::UI::Xaml::RoutedEventArgs^ e);
        void ResetValuesClick(Object^ sender, ::Microsoft::UI::Xaml::RoutedEventArgs^ e);
	};
}
