// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "XamlMetadataProvider.h"
#include "XamlType.h"

#ifndef BUILD_WINDOWS
#include "MUXControlsFactory.h"
#endif

std::vector<XamlMetadataProvider::Entry>* XamlMetadataProvider::s_types{ nullptr };

XamlMetadataProvider::XamlMetadataProvider()
{
#ifndef BUILD_WINDOWS
    RegisterTypes();
#endif
}

void XamlMetadataProvider::Initialize()
{
#ifndef BUILD_WINDOWS
    MUXControlsFactory::EnsureInitialized();
#endif
}

bool XamlMetadataProvider::RegisterXamlType(
    _In_ PCWSTR typeName,
    std::function<winrt::IXamlType()> createXamlTypeCallback
    )
{
    if (!s_types)
    {
        s_types = new std::vector<Entry>();
    }

    Entry type{ typeName, createXamlTypeCallback };

    s_types->push_back(type);
    return true;
}

winrt::IXamlType XamlMetadataProvider::GetXamlType(
    _In_ const wstring_view& typeName)
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
