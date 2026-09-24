// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// ListAndTemplateTests.xaml.h
// Declaration of the ListAndTemplateTests class.
//

#pragma once

#include "ListAndTemplateTests.g.h"

namespace BindTestbedCX
{
	/// <summary>
	/// An empty page that can be used on its own or navigated to within a Frame.
	/// </summary>
	public ref class ListAndTemplateTests sealed
	{
	public:
		ListAndTemplateTests();
		property BindTestbedModel::DataModel^ Model;
		property BindTestbedModel::DOModel^ DOModel;
		property BindTestbedCXModel::ModelCX^ ModelCX;

		property ::Windows::Foundation::Collections::IObservableVector<BindTestbedModel::IEmployee^>^ Employees;
		property ::Windows::Foundation::Collections::IObservableVector<::Platform::String^>^ AllFirstNames;
		property ::Windows::Foundation::Collections::IObservableVector<::Platform::String^>^ AllLastNames;

		property Object^ SomeButtonContent
		{
			Object^ get() { return someButton->Content; }
			void set(Object^ value) { someButton->Content = value; }
		}
		
		void UpdateValuesClick(Object^ sender, ::Microsoft::UI::Xaml::RoutedEventArgs^ e);
		void ResetValuesClick(Object^ sender, ::Microsoft::UI::Xaml::RoutedEventArgs^ e);
	};
}
