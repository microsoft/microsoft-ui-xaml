// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

using namespace Concurrency;

#include "BasicTests.g.h"


namespace BindTestbedCX
{
	/// <summary>
	/// An empty page that can be used on its own or navigated to within a Frame.
	/// </summary>
	[::Windows::Foundation::Metadata::WebHostHidden]
	public ref class BasicTests sealed
	{
	public:
		BasicTests();
		property int MyInt;
		property BindTestbedModel::DataModel^ Model;
		property BindTestbedModel::DOModel^ DOModel;

		property ::Windows::Foundation::Collections::IObservableVector<BindTestbedModel::IEmployee^>^ Employees;
		property ::Windows::Foundation::Collections::IObservableVector<::Platform::String^>^ AllFirstNames;
		property ::Windows::Foundation::Collections::IObservableVector<::Platform::String^>^ AllLastNames;


		property ::Platform::String^ DPOnPage
		{
			::Platform::String^ get()
			{
				return (::Platform::String^)GetValue(DPOnPageProperty);
			}
			void set(::Platform::String^ value)
			{
				SetValue(DPOnPageProperty, value);
			}
		}

		property ::Platform::String^ NonDPOnPage
		{
			::Platform::String^ get()
			{
				return _nonDPonPage;
			}

			void set(::Platform::String^ value)
			{
				this->_nonDPonPage = value;
			}
		}

		void InitializeValues();
		void UpdateValuesClick(Object^ sender, ::Microsoft::UI::Xaml::RoutedEventArgs^ e);
		void ResetValuesClick(Object^ sender, ::Microsoft::UI::Xaml::RoutedEventArgs^ e);
		void StopTrackingClick(Object^ sender, ::Microsoft::UI::Xaml::RoutedEventArgs^ e);
		void ReInitializeBindingsClick(Object^ sender, ::Microsoft::UI::Xaml::RoutedEventArgs^ e);


	private:
		static ::Microsoft::UI::Xaml::DependencyProperty^ _DPOnPageProperty;
		::Platform::String^ _nonDPonPage;

	public:
		static property ::Microsoft::UI::Xaml::DependencyProperty^ DPOnPageProperty
		{
			::Microsoft::UI::Xaml::DependencyProperty^ get()
			{
				return _DPOnPageProperty;
			}
			void set(::Microsoft::UI::Xaml::DependencyProperty^ value)
			{
				_DPOnPageProperty = value;
			}
		}
	};
}
