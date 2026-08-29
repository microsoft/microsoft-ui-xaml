// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Animal.h"

using namespace BindTestbedCXModel;
using namespace Platform;
using namespace ::Windows::UI::Popups;

Animal::Animal(::Platform::String^ name, ::Platform::String^ color)
{
    this->_name = name;
    this->_color = color;
}
void Animal::Click_RegularArgsOnAnimal(Platform::Object^ sender, Microsoft::UI::Xaml::RoutedEventArgs^ e)
{
    MessageDialog^ dlg = ref new MessageDialog("Regular arguments clicked on Animal");
    auto t = dlg->ShowAsync();
}

void Animal::Click_NoArgsOnAnimal()
{
    MessageDialog^ dlg = ref new MessageDialog("No argument Clicked on Animal");
    auto t = dlg->ShowAsync();
}

void Animal::Click_BaseArgsOnAnimal(Platform::Object^ sender, Microsoft::UI::Xaml::RoutedEventArgs^ e)
{
    MessageDialog^ dlg = ref new MessageDialog("Base argument clicked on Animal");
    auto t = dlg->ShowAsync();
}

void Animal::Click_OverloadedArgsOnAnimal(Platform::Object^ sender, Platform::Object^ e)
{
    MessageDialog^ dlg = ref new MessageDialog("Overloaded argument clicked on Animal");
    auto t = dlg->ShowAsync();
}
