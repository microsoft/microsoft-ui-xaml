// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <map>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Framework { namespace ResourceLoading {

        // Implementing the ICustomXamlResourceLoader interface, allowing us to provide custom
        // resources from code that are visible to the parser
        ref class CustomResourceLoader sealed : public Microsoft::UI::Xaml::Resources::CustomXamlResourceLoader
        {
        private:
            typedef std::map<Platform::String^, Platform::Object^> PropertyTypeMap;
            typedef std::map<Platform::String^, PropertyTypeMap> PropertyNameMap;
            typedef std::map<Platform::String^, PropertyNameMap> ObjectTypeMap;
            typedef std::map<Platform::String^, ObjectTypeMap> ResourceIdMap;

            ResourceIdMap m_resourceIdToObject;
            std::map<Platform::String^, Platform::Object^> m_simpleResourceIdToObject;

            static Platform::String^ m_defaultResourceId;
            static Platform::String^ m_defaultResourceIdValue;
            static Platform::String^ m_nullId;

        public:
            CustomResourceLoader()
            {
                Microsoft::UI::Xaml::Resources::CustomXamlResourceLoader::Current = this;
            }

            void AddResource(
                Platform::String^ resourceId,
                Platform::String^ objectType,
                Platform::String^ propertyName,
                Platform::String^ propertyType,
                Platform::Object^ value);

            void AddResource(
                Platform::String^ resourceId,
                Platform::Object^ value);

            Platform::Object^ GetDefaultResourceFromResourceId(
                Platform::String^ resourceId,
                Platform::String^ objectType);

            Platform::Object^ GetResource(
                Platform::String^ resourceId,
                Platform::String^ objectType,
                Platform::String^ propertyName,
                Platform::String^ propertyType) override;
        };
} } } } } }