// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// MainPage.xaml.h
// Declaration of the MainPage class.
//

#pragma once

#include "MainPage.g.h"
#include "BasicTests.xaml.h"
#include "EventTests.xaml.h"
#include "FunctionTests.xaml.h"
#include "ListAndTemplateTests.xaml.h"
#include "LoadAndCreateFromStringTests.xaml.h"
#include "TestsPage2.xaml.h"
#include "TwoWayTests.xaml.h"
#include "PhasingTests.xaml.h"
#include "NullableTests.xaml.h"

namespace BindTestbedCX
{
	/// <summary>
	/// An empty page that can be used on its own or navigated to within a Frame.
	/// </summary>
	public ref class MainPage sealed
	{
	public:
		MainPage();
		property MainModel^ MainModel;

	private:
		void DetectLeaks_Click(Platform::Object^ sender, Microsoft::UI::Xaml::RoutedEventArgs^ e);

    };
}