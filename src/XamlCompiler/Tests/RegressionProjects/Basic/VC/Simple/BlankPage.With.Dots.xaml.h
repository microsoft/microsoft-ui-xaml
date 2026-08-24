// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// BlankPage.xaml.h
// Declaration of the BlankPage class
//

#pragma once

#include "BlankPage.With.Dots.g.h"

namespace Simple
{
    public ref class BlankPageBase : Microsoft::UI::Xaml::Controls::Page
    {
    public:
        property Platform::String^ Foo
        {
            Platform::String^ get() { return nullptr; }
        }

        void OnLoaded(Object^ sender, Microsoft::UI::Xaml::RoutedEventArgs^ e)
        {}
    };

	/// <summary>
	/// An empty page that can be used on its own or navigated to within a Frame.
	/// </summary>
	[::Windows::Foundation::Metadata::WebHostHidden]
	public ref class BlankPage sealed
	{
	public:
		BlankPage();
	};
}
