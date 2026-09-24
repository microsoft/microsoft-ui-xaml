// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// MainPage.xaml.h
// Declaration of the MainPage class.
//

#pragma once

#include "MainPage.g.h"

namespace EventHandling_968976
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
	};
}
