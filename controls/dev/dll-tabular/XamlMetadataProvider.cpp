// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "XamlMetadataProvider.h"
#include "XamlMetadataProviderGenerated.h"
#include "XamlType.h"
#include "MUXControlsFactory.h"

std::recursive_mutex XamlMetadataProvider::s_typeEntriesMutex{};

/*static */ 
void XamlMetadataProvider::Initialize()
{
    MUXControlsFactory::EnsureInitialized();
}

/*static */ 
void XamlMetadataProvider::ClearTypes()
{
    std::lock_guard<std::recursive_mutex> lock(s_typeEntriesMutex);

    ClearTypeProperties();

    for (auto& entry : c_typeEntries)
    {
        entry.xamlType = nullptr;
    }
}

/* static */
winrt::IXamlType XamlMetadataProvider::LookupXamlType(
    const wstring_view& typeName)
{
    // Only typenames with a matching known namespace prefix, or those without
    // a namespace at all, are going to be found inside our type table.
    bool hasKnownNamespacePrefix = false;
    for (auto& knownNamespacePrefix : c_knownNamespacePrefixes)
    {
        if (typeName.starts_with(knownNamespacePrefix))
        {
            hasKnownNamespacePrefix = true;
            break;
        }
    }
    if (!hasKnownNamespacePrefix && typeName.find(L'.') == std::wstring::npos)
    {
        hasKnownNamespacePrefix = true;
    }

    if (hasKnownNamespacePrefix)
    {
        for (auto& entry : c_typeEntries)
        {
            if (typeName == entry.typeName)
            {
                std::lock_guard<std::recursive_mutex> lock(s_typeEntriesMutex);
                
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
    return LookupXamlType(type.Name);
}

winrt::IXamlType XamlMetadataProvider::GetXamlType(winrt::hstring const& fullName)
{
    return LookupXamlType(fullName);
}

winrt::com_array<winrt::XmlnsDefinition> XamlMetadataProvider::GetXmlnsDefinitions()
{
    return {};
}
