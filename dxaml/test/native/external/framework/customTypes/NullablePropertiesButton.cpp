// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "NullablePropertiesButton.h"

using namespace Tests::Native::External::Framework;
using namespace Microsoft::UI::Xaml;

DependencyProperty^ NullablePropertiesButton::s_nullableDoubleProperty = nullptr;

/*static*/ void NullablePropertiesButton::InitDPs()
{
    s_nullableDoubleProperty =
        Microsoft::UI::Xaml::DependencyProperty::Register(
            "NullableDouble",
            Platform::IBox<double>::typeid,
            NullablePropertiesButton::typeid,
            nullptr);
}

/*static*/ void NullablePropertiesButton::ResetDPs()
{
    s_nullableDoubleProperty = nullptr;
}