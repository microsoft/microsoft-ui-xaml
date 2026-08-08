// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// EventTests.xaml.h
// Declaration of the EventTests class.
//

#pragma once

#include "EventTests.g.h"
#include "MainPage.xaml.h"

using namespace Microsoft::UI::Xaml::Automation::Peers;
using namespace Microsoft::UI::Xaml::Automation::Provider;
using namespace Microsoft::UI::Xaml::Controls;

namespace BindTestbed
{
	/// <summary>
	/// An empty page that can be used on its own or navigated to within a Frame.
	/// </summary>
	public ref class EventTests sealed
	{
	public:
		EventTests();
		property BindTestbedModel::DataModel^ Model;
		property BindTestbedModel::DOModel^ DOModel;

		property Microsoft::UI::Xaml::RoutedEventHandler^ clickDelegate;

		void InitializeValues();
		void UpdateValuesClick(Object^ sender, ::Microsoft::UI::Xaml::RoutedEventArgs^ e);
		void ResetValuesClick(Object^ sender, ::Microsoft::UI::Xaml::RoutedEventArgs^ e);

		void Click_RegularArgs(Platform::Object^ sender, Microsoft::UI::Xaml::RoutedEventArgs^ e);
		void Click_NoArgs();
		void Click_BaseArgs(Platform::Object^ sender, Microsoft::UI::Xaml::RoutedEventArgs^ e);
		void Click_OverloadedArgs();
		void Click_OverloadedArgs(Platform::Object^ sender, Platform::Object^ e);
	};
}
