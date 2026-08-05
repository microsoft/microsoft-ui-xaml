// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// PhasingTests.xaml.h
// Declaration of the PhasingTests class.
//

#pragma once

#include "MainPage.xaml.h"
#include "PhasingTests.g.h"

using namespace Microsoft::UI::Xaml::Controls;
using namespace ::Windows::Foundation;
using namespace ::Windows::Foundation::Collections;
using namespace Microsoft::UI::Xaml;
using namespace Platform::Collections;

namespace BindTestbed
{
	public ref class ExtraInfo sealed
	{
	public:
		property Platform::String^ Caption;

		ExtraInfo(Platform::String^ caption);
	};

	public ref class MyInfo sealed
	{
	public:
		property Platform::String^ ImageUrl;
		property Platform::String^ Caption;
		property ::Platform::String^ Prop1;
		property ::Platform::String^ Prop2;
		property ::Platform::String^ Prop3;

		MyInfo(int index, Platform::String^ imageUrl, Platform::String^ caption);
	};

	[::Windows::Foundation::Metadata::WebHostHidden]
	public ref class MyItem sealed : Microsoft::UI::Xaml::Data::INotifyPropertyChanged, public Microsoft::UI::Xaml::DependencyObject
	{
	public:
		property Platform::String^ Title;
		property Platform::String^ Subtitle;
		property Platform::String^ Description
		{
			Platform::String^ get()
			{
				return _description;
			}
			void set(Platform::String^ value)
			{
				if (value != _description)
				{
					_description = value;
					NotifyPropertyChanged("Description");
				}
			}
		};
		property int Index;
		property MyInfo^ Info;
		property ExtraInfo^ OtherInfo;



		property ::Platform::String^ DPOnMyItem
		{
			::Platform::String^ get()
			{
				return (::Platform::String^)GetValue(DPOnMyItemProperty);
			}
			void set(::Platform::String^ value)
			{
				SetValue(DPOnMyItemProperty, value);
			}
		}
		static property ::Microsoft::UI::Xaml::DependencyProperty^ DPOnMyItemProperty
		{
			::Microsoft::UI::Xaml::DependencyProperty^ get()
			{
				return _DPOnMyItemProperty;
			}
			void set(::Microsoft::UI::Xaml::DependencyProperty^ value)
			{
				_DPOnMyItemProperty = value;
			}
		}
		MyItem(int index, Platform::String^ title, Platform::String^ subtitle, Platform::String^ description, MyInfo^ info, ExtraInfo^ otherInfo, Platform::String^ dp);

		// Fired when properties change
		virtual event Microsoft::UI::Xaml::Data::PropertyChangedEventHandler^ PropertyChanged;

	private:
		static ::Microsoft::UI::Xaml::DependencyProperty^ _DPOnMyItemProperty;
		Platform::String^ _description;
		void NotifyPropertyChanged(Platform::String^ propertyName);
	};

	/// <summary>
	/// An empty page that can be used on its own or navigated to within a Frame.
	/// </summary>
	public ref class PhasingTests sealed
	{
	public:
		PhasingTests();

		void InitializeValues();
		void Reset_Click(Object^ sender, ::Microsoft::UI::Xaml::RoutedEventArgs^ e);
		void Reload_Click(Object^ sender, ::Microsoft::UI::Xaml::RoutedEventArgs^ e);
		void StackPanel_PointerReleased(Object^ sender, ::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs^ e);

		void PhasingTests::MyGridView_ContainerContentChanging(Microsoft::UI::Xaml::Controls::ListViewBase^ sender, Microsoft::UI::Xaml::Controls::ContainerContentChangingEventArgs^ args);

	private:
		bool Initialized = false;
		int itemsCount = 10;
		void wait(int msTime);
		Platform::Collections::Vector<MyItem^>^ myItems;

		::Windows::Foundation::EventRegistrationToken INPC_token;

		void SlowPhasing_UnChecked(Object^ sender, ::Microsoft::UI::Xaml::RoutedEventArgs^ e);
		void SlowPhasing_Checked(Object^ sender, ::Microsoft::UI::Xaml::RoutedEventArgs^ e);
		void PhasedTemplate_UnChecked(Object^ sender, ::Microsoft::UI::Xaml::RoutedEventArgs^ e);
		void PhasedTemplate_Checked(Object^ sender, ::Microsoft::UI::Xaml::RoutedEventArgs^ e);
	};
}