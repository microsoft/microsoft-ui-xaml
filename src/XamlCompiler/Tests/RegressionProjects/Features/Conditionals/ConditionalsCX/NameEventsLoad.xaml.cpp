// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// NameEventsLoad.xaml.cpp
// Implementation of the NameEventsLoad class
//

#include "pch.h"
#include "NameEventsLoad.xaml.h"

using namespace Conditionals;
using namespace ConditionalControls;

using namespace Platform;
using namespace ::Windows::Foundation;
using namespace ::Windows::Foundation::Collections;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Controls::Primitives;
using namespace Microsoft::UI::Xaml::Data;
using namespace Microsoft::UI::Xaml::Input;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Navigation;

// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=234238

NameEventsLoad::NameEventsLoad()
{
	InitializeComponent();
}


void Conditionals::NameEventsLoad::Button_Click(Platform::Object^ sender, Microsoft::UI::Xaml::RoutedEventArgs^ e)
{
    buttonResult->Text = "always";
}

void Conditionals::NameEventsLoad::Button_Click_V1(Platform::Object^ sender, Microsoft::UI::Xaml::RoutedEventArgs^ e)
{
    IVersionedProperties^ obj = (IVersionedProperties^) sender;
    buttonResult->Text = obj->V1Property;
}

void Conditionals::NameEventsLoad::Button_Click_V2(Platform::Object^ sender, Microsoft::UI::Xaml::RoutedEventArgs^ e)
{
    IVersionedProperties^ obj = (IVersionedProperties^)sender;
    buttonResult->Text = obj->V2Property;
}

void Conditionals::NameEventsLoad::Button_Click_V3(Platform::Object^ sender, Microsoft::UI::Xaml::RoutedEventArgs^ e)
{
    IVersionedProperties^ obj = (IVersionedProperties^)sender;
    buttonResult->Text = obj->V3Property;
}

void Conditionals::NameEventsLoad::Button_Click_notV3(Platform::Object^ sender, Microsoft::UI::Xaml::RoutedEventArgs^ e)
{
    IVersionedProperties^ obj = (IVersionedProperties^)sender;
    buttonResult->Text = "notV3here's" + obj->V2Property;
}
