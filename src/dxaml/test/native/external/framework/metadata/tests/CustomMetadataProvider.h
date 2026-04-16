// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

ref class CustomMetadataProvider sealed : public Microsoft::UI::Xaml::Markup::IXamlMetadataProvider
{
public:
    property bool CalledWithTypeName;
    property bool CalledWithFullName;

    virtual ::Microsoft::UI::Xaml::Markup::IXamlType^ GetXamlType(::Windows::UI::Xaml::Interop::TypeName type)
    {
        CalledWithTypeName = true;
        CalledWithFullName = false;
        return nullptr;
    }

    virtual ::Microsoft::UI::Xaml::Markup::IXamlType^ GetXamlType(::Platform::String^ fullName)
    {
        CalledWithTypeName = false;
        CalledWithFullName = true;
        return nullptr;
    }

    virtual ::Platform::Array<Microsoft::UI::Xaml::Markup::XmlnsDefinition>^ GetXmlnsDefinitions()
    {
        return ref new ::Platform::Array<Microsoft::UI::Xaml::Markup::XmlnsDefinition>(0);
    }
};
