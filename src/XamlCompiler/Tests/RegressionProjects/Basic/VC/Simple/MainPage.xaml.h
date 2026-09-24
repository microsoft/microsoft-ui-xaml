// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// MainPage.xaml.h
// Declaration of the MainPage class.
//

#pragma once

#include "MainPage.g.h"
#include "MyGrid.h"

namespace Simple
{
	/// <summary>
	/// An empty page that can be used on its own or navigated to within a Frame.
	/// </summary>
	public ref class MainPage sealed
	{
	public:
		MainPage();
		void FirstHandler(const Platform::Array<unsigned int>^ args) { (void)args; }
		void SecondHandler(Platform::WriteOnlyArray<unsigned int>^ args) { (void)args; }
		void ThirdHandler(Platform::Array<unsigned int>^* args) { (void)args; }
		void FourthHandler(Platform::String^* args) { (void)args; }

	private:
		void ClickHandler(Platform::Object^ sender, Microsoft::UI::Xaml::RoutedEventArgs^ e);
		void TappedHandler(Platform::Object^ sender, Microsoft::UI::Xaml::Input::TappedRoutedEventArgs^ e);
	};

	[::Windows::Foundation::Metadata::WebHostHidden]
	public ref class PropertyBag sealed
	{
	public:
		PropertyBag(void) {}

	public:
		property Platform::String^ StringProp;
		property int IntProp;
		property double DoubleProp;
		property bool BooleanProp;
        property short Int16Prop;
	};
}
