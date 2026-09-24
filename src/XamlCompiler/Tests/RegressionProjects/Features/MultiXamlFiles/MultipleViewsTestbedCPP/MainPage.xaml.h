// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// MainPage.xaml.h
// Declaration of the MainPage class.
//

#pragma once

#include "MainPage.g.h"

namespace MultipleViewsTestbedCPP
{
	/// <summary>
	/// An empty page that can be used on its own or navigated to within a Frame.
	/// </summary>
	public ref class MainPage sealed
	{
	public:
		MainPage();

	private:
		Platform::String^ filename = nullptr;

		void Current_SizeChanged(Platform::Object^ sender, Microsoft::UI::Xaml::WindowSizeChangedEventArgs^ e);
		void LoadCorrectXamlFile();

		void MyControl_Click(Platform::Object^ sender, Microsoft::UI::Xaml::RoutedEventArgs^ e);
		void MyControl_Checked(Platform::Object^ sender, Microsoft::UI::Xaml::RoutedEventArgs^ e);
	};
}
