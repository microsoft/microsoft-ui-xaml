// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Framework { namespace Styles {

    ref class TestCustomResourceLoader sealed : Microsoft::UI::Xaml::Resources::CustomXamlResourceLoader
    {
    public:
        TestCustomResourceLoader();
        void AddResource(Platform::String^ resourceId, Platform::String^ objectType, Platform::String^ propertyName, Platform::String^ propertyType, Object^ value);
        void AddResource(Platform::String^ resourceId, Object^ value);

    protected:
        virtual Object^ GetResource(Platform::String^ resourceId, Platform::String^ objectType, Platform::String^ propertyName, Platform::String^ propertyType) override;

    private:
        void EnsureDictionaryGetters(Platform::String^ resourceId, Platform::String^ objectType, Platform::String^ propertyName);
        Object^ GetDefaultResourceFromResourceId(Platform::String^ resourceId, Platform::String^ objectType);

    private:
        std::map<Platform::String^, std::map<Platform::String^, std::map<Platform::String^, std::map<Platform::String^, Object^>>>> resourceIdToObject_;
        std::map<Platform::String^, Object^> simpleResourceIdToObject_;

        Platform::String^ DefaultResourceId;
        Platform::String^ DefaultResourceIdValue;
        Platform::String^ NullId;
    };
} } } } } }