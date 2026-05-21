// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "XamlTypeInfo.g.h"

#include "App.xaml.h"

#include "App.g.hpp"


::Microsoft::UI::Xaml::Markup::IXamlType^ ::XamlTypeInfo::InfoProvider::XamlTypeInfoProvider::CreateXamlType(::Platform::String^ typeName)
{
    if (typeName == L"Microsoft.UI.Xaml.Application")
    {
        return ref new XamlSystemBaseType(typeName);
    }

    if (typeName == L"Microsoft.UI.Xaml.Controls.Canvas")
    {
        return ref new XamlSystemBaseType(typeName);
    }

    if (typeName == L"Microsoft.UI.Xaml.Controls.Panel")
    {
        return ref new XamlSystemBaseType(typeName);
    }

    if (typeName == L"CustomTypes.App")
    {
        ::XamlTypeInfo::InfoProvider::XamlUserType^ userType = ref new ::XamlTypeInfo::InfoProvider::XamlUserType(this, typeName, GetXamlTypeByName(L"Microsoft.UI.Xaml.Application"));
        userType->KindOfType = ::Windows::UI::Xaml::Interop::TypeKind::Metadata;
        userType->Activator =
            []() -> Platform::Object^ 
            {
                return ref new ::CustomTypes::App(); 
            };
        return userType;
    }

    if (typeName == L"Tests.Native.External.Automation.XamlUIABridge.XamlUIABridgeHostCanvas")
    {
        ::XamlTypeInfo::InfoProvider::XamlUserType^ userType = ref new ::XamlTypeInfo::InfoProvider::XamlUserType(this, typeName, GetXamlTypeByName(L"Microsoft.UI.Xaml.Controls.Canvas"));
        userType->KindOfType = ::Windows::UI::Xaml::Interop::TypeKind::Metadata;
        userType->Activator =
            []() -> Platform::Object^ 
            {
                return ref new ::Tests::Native::External::Automation::XamlUIABridge::XamlUIABridgeHostCanvas(); 
            };
        userType->SetIsBindable();
        return userType;
    }

    return nullptr;
}

::Microsoft::UI::Xaml::Markup::IXamlMember^ ::XamlTypeInfo::InfoProvider::XamlTypeInfoProvider::CreateXamlMember(::Platform::String^ longMemberName)
{
    // No Local Properties
    (void)longMemberName; // Unused parameter
    return nullptr;
}

