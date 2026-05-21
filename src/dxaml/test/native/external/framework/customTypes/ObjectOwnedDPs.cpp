// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ObjectOwnedDPs.h"

using namespace Tests::Native::External::Framework;
using namespace Microsoft::UI::Xaml;

DependencyProperty^ SomeTypeWithDPsOwnedByObject::s_textProperty = nullptr;
DependencyProperty^ DOWithDPsOwnedByObject::s_textProperty = nullptr;
DependencyProperty^ DOWithDPsOwnedByObject::s_bindingProperty = nullptr;

/*static*/ void SomeTypeWithDPsOwnedByObject::InitDPs()
{
    s_textProperty =
        Microsoft::UI::Xaml::DependencyProperty::RegisterAttached(
            "Text",
            Platform::String::typeid,
            Platform::Object::typeid,   // Note that the owner is Object!
            nullptr);
}

/*static*/ void SomeTypeWithDPsOwnedByObject::ResetDPs()
{
    s_textProperty = nullptr;
}

/*static*/ void DOWithDPsOwnedByObject::InitDPs()
{
    s_textProperty =
        Microsoft::UI::Xaml::DependencyProperty::Register(
            "DoText",
            Platform::String::typeid,
            Platform::Object::typeid,   // Note that the owner is Object!
            nullptr);

    s_bindingProperty =
        Microsoft::UI::Xaml::DependencyProperty::Register(
            "DoBinding",
            Microsoft::UI::Xaml::Data::Binding::typeid,
            Platform::Object::typeid,   // Note that the owner is Object!
            nullptr);
}

/*static*/ void DOWithDPsOwnedByObject::ResetDPs()
{
    s_textProperty = nullptr;
    s_bindingProperty = nullptr;
}