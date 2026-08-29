// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <XamlTypeInfo.g.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Common {

    ref class MetadataProvider sealed : public ::Microsoft::UI::Xaml::Markup::IXamlMetadataProvider
    {
        ::XamlTypeInfo::InfoProvider::XamlTypeInfoProvider^ _xamlCompilerProvider;
        ::Microsoft::UI::Xaml::XamlTypeInfo::XamlControlsXamlMetaDataProvider^ _xamlControlsProvider;
        
    public:
        virtual ::Microsoft::UI::Xaml::Markup::IXamlType^ GetXamlType(::Windows::UI::Xaml::Interop::TypeName type)
        {
            EnsureProviders();
            auto xamlType = _xamlCompilerProvider->GetXamlTypeByType(type);
            
            if (xamlType)
            {
                return xamlType;
            }
            else
            {
                return _xamlControlsProvider->GetXamlType(type);
            }
        }

        virtual ::Microsoft::UI::Xaml::Markup::IXamlType^ GetXamlType(::Platform::String^ fullName)
        {
            EnsureProviders();
            auto xamlType = _xamlCompilerProvider->GetXamlTypeByName(fullName);
            
            if (xamlType)
            {
                return xamlType;
            }
            else
            {
                return _xamlControlsProvider->GetXamlType(fullName);
            }
        }

        virtual ::Platform::Array<Microsoft::UI::Xaml::Markup::XmlnsDefinition>^ GetXmlnsDefinitions()
        {
            EnsureProviders();
            return _xamlControlsProvider->GetXmlnsDefinitions();
        }
        
    private:
        void EnsureProviders()
        {
            if (!_xamlCompilerProvider)
            {
                _xamlCompilerProvider = ref new ::XamlTypeInfo::InfoProvider::XamlTypeInfoProvider();
            }
            
            if (!_xamlControlsProvider)
            {
                _xamlControlsProvider = ref new ::Microsoft::UI::Xaml::XamlTypeInfo::XamlControlsXamlMetaDataProvider();
            }
        }
    };

} } } } }