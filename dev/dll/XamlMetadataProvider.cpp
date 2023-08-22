// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "XamlMetadataProvider.h"
#include "XamlType.h"
#include "MUXControlsFactory.h"

std::vector<XamlMetadataProvider::Entry>* XamlMetadataProvider::s_types{ nullptr };

XamlMetadataProvider::XamlMetadataProvider()
{
    RegisterTypes();
}

void XamlMetadataProvider::Initialize()
{
    MUXControlsFactory::EnsureInitialized();
}

bool XamlMetadataProvider::RegisterXamlType(
    PCWSTR typeName,
    std::function<winrt::IXamlType()> createXamlTypeCallback
    )
{
    if (!s_types)
    {
#pragma warning(push)
#pragma warning(disable : 26409) // Disable r.11, see comment in header file
        s_types = new std::vector<Entry>();
#pragma warning(pop)
    }

    Entry type{ typeName, createXamlTypeCallback };

    s_types->push_back(type);
    return true;
}

winrt::IXamlType XamlMetadataProvider::GetXamlType(
    const wstring_view& typeName)
{
    if (s_types)
    {
        for (auto& entry : *s_types)
        {
            if (typeName == entry.typeName)
            {
                if (!entry.xamlType)
                {
                    entry.xamlType = entry.createXamlTypeCallback();
                }
                return entry.xamlType;
            }
        }
    }

    return nullptr;
}

winrt::IXamlType XamlMetadataProvider::GetXamlType(winrt::TypeName const& type)
{
    return GetXamlType(type.Name);
}

winrt::IXamlType XamlMetadataProvider::GetXamlTypeByFullName(winrt::hstring const& fullName)
{
    return GetXamlType(fullName);
}

winrt::com_array<winrt::XmlnsDefinition> XamlMetadataProvider::GetXmlnsDefinitions()
{
    return {};
}
