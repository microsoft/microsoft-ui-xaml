// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// BindTests.xaml.cpp
// Implementation of the BindTests class
//

#include "pch.h"
#include "BindTests.xaml.h"

using namespace Conditionals;
using namespace ConditionalControls;
using namespace Conditionals::SubFolder;

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

BindTests::BindTests()
{
    this->Model = ref new ConditionalControls::Model();
	InitializeComponent();
}

String^ BindTests::V2Property::get()
{
    return aButton->V2Property;
}

void BindTests::V2Property::set(String^ value)
{
    if (aButton->V2Property != value)
    {
        aButton->V2Property = value;
        NotifyPropertyChanged("V2Property");
    }
}

String^ BindTests::V3Property::get()
{
    return aButton->V3Property;
}

void BindTests::V3Property::set(String^ value)
{
    if (aButton->V3Property != value)
    {
        aButton->V3Property = value;
        NotifyPropertyChanged("V3Property");
    }
}


void BindTests::Click_V2(Object^ sender, RoutedEventArgs^ e)
{
    IVersionedProperties^ obj = (IVersionedProperties^)sender;
    clickV2results->Text = obj->V2Property;
}

void BindTests::Click_V3(Object^ sender, RoutedEventArgs^ e)
{
    IVersionedProperties^ obj = (IVersionedProperties^)sender;
    clickV2results->Text = obj->V3Property;
}

void BindTests::NotifyPropertyChanged(String^ propertyName)
{
    PropertyChanged(this, ref new PropertyChangedEventArgs(propertyName));
}