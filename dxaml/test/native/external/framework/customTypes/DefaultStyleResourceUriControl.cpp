// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "DefaultStyleResourceUriControl.h"

using namespace Microsoft::UI::Xaml;
using namespace Tests::Native::External::Framework;

DefaultStyleResourceUriControl::DefaultStyleResourceUriControl()
{
    DefaultStyleKey = DefaultStyleResourceUriControl::typeid->FullName;
    DefaultStyleResourceUri = ref new ::Windows::Foundation::Uri(L"ms-resource://testResourceMap/Files/Resources/native/framework/styles/customgeneric.xaml");
}

DefaultStyleResourceUriControlWithEmptyDefaultStyleKey::DefaultStyleResourceUriControlWithEmptyDefaultStyleKey()
{
    DefaultStyleResourceUri = ref new ::Windows::Foundation::Uri(L"ms-resource://testResourceMap/Files/Resources/native/framework/styles/customgeneric.xaml");
}

DefaultStyleResourceUriControlWithInvalidDefaultStyleKey::DefaultStyleResourceUriControlWithInvalidDefaultStyleKey()
{
    DefaultStyleKey = L"InvalidKey";
    DefaultStyleResourceUri = ref new ::Windows::Foundation::Uri(L"ms-resource://testResourceMap/Files/Resources/native/framework/styles/customgeneric.xaml");
}

InvalidDefaultStyleResourceUriControl::InvalidDefaultStyleResourceUriControl()
{
    DefaultStyleKey = InvalidDefaultStyleResourceUriControl::typeid->FullName;
    DefaultStyleResourceUri = ref new ::Windows::Foundation::Uri(L"ms-resource://invalidMap/Files/Resources/asdf/asdf/asds/sadfasf.xaml");
}
