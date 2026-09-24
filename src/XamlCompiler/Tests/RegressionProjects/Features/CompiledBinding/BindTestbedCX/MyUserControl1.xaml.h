// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// MyUserControl.xaml.h
// Declaration of the MyUserControl class
//

#pragma once

#include "MyUserControl1.g.h"

namespace BindTestbedCX
{
	[::Windows::Foundation::Metadata::WebHostHidden]
	public ref class MyUserControl1 sealed
	{
	private:
		static ::Microsoft::UI::Xaml::DependencyProperty^ _Property1Property;

	public:
		MyUserControl1();
		
		static property ::Microsoft::UI::Xaml::DependencyProperty^ Property1Property
		{
			::Microsoft::UI::Xaml::DependencyProperty^ get()
			{
				return _Property1Property;
			}
			void set(::Microsoft::UI::Xaml::DependencyProperty^ value)
			{
				_Property1Property = value;
			}
		}

		property ::Platform::String^ Property1
		{
			::Platform::String^ get()
			{
				return (::Platform::String^)GetValue(Property1Property);
			}
			void set(::Platform::String^ value)
			{
				SetValue(Property1Property, value);
			}
		}
    private:
        void aLazyTextBlock_Tapped(Platform::Object^ sender, Microsoft::UI::Xaml::Input::TappedRoutedEventArgs^ e);
    };
}
