// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "TestCustomResourceLoader.h"

using namespace Platform;
using namespace ::Windows::Foundation;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Data;
using namespace Microsoft::UI::Xaml::Media;

using namespace test_infra;
using namespace std;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Framework { namespace Styles {

TestCustomResourceLoader::TestCustomResourceLoader()
{
    DefaultResourceId = L"Default";
    DefaultResourceIdValue = L"DefaultValue";
    NullId = L"Null";
}

void TestCustomResourceLoader::EnsureDictionaryGetters(String^ resourceId, String^ objectType, String^ propertyName)
{
    if (resourceIdToObject_.find(resourceId) == resourceIdToObject_.end())
    {
        resourceIdToObject_.emplace(resourceId, std::map<String^, std::map<String^, std::map<String^, Object^>>>());
    }

    std::map<String^, std::map<String^, std::map<String^, Object^>>> map = resourceIdToObject_[resourceId];
    if (map.find(objectType) == map.end())
    {
        map.emplace(objectType, std::map<String^, std::map<String^, Object^>>());
    }

    std::map<String^, std::map<String^, Object^>> map2 = map[objectType];
    if (map2.find(propertyName) == map2.end())
    {
        map2.emplace(propertyName, std::map<String^, Object^>());
    }
}

Object^ TestCustomResourceLoader::GetDefaultResourceFromResourceId(String^ resourceId, String^ objectType)
{
    String^ typeOfString = L"Windows.Foundation.String";
    if (objectType != typeOfString)
    {
        throw ref new COMException(E_FAIL, L"Only a default string is supported;  " + resourceId + L" of type " + typeOfString + L" was not found.");
    }

    return DefaultResourceIdValue;
}

void TestCustomResourceLoader::AddResource(String^ resourceId, String^ objectType, String^ propertyName, String^ propertyType, Object^ value)
{
    EnsureDictionaryGetters(resourceId, objectType, propertyName);
    resourceIdToObject_[resourceId][objectType][propertyName].emplace(propertyType, value);
}

void TestCustomResourceLoader::AddResource(String^ resourceId, Object^ value)
{
    simpleResourceIdToObject_.emplace(resourceId, value);
}

Object^ TestCustomResourceLoader::GetResource(String^ resourceId, String^ objectType, String^ propertyName, String^ propertyType)
{
    if (resourceId == NullId)
    {
        return nullptr;
    }

    if (resourceId == DefaultResourceId)
    {
        return GetDefaultResourceFromResourceId(resourceId, objectType);
    }

    try
    {
        if (simpleResourceIdToObject_.find(resourceId) != simpleResourceIdToObject_.end())
        {
            return simpleResourceIdToObject_[resourceId];
        }
        else
        {
            return resourceIdToObject_[resourceId][objectType][propertyName][propertyType];
        }
    }
    catch (...)
    {
        throw ref new COMException(E_FAIL, L"Object not found custom resource loader (" + resourceId + L", " + objectType + L", " + propertyName + L", " + propertyType);
    }
}

} } } } } }
