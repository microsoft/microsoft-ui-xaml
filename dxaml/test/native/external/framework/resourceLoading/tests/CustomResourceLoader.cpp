// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "CustomResourceLoader.h"
#include <stdexcept>

using namespace Microsoft::UI::Xaml::Tests::Framework::ResourceLoading;
using namespace Platform;
using namespace ::Windows::UI::Xaml::Interop;

Platform::String^ CustomResourceLoader::m_defaultResourceId(L"Default");
Platform::String^ CustomResourceLoader::m_defaultResourceIdValue(L"DefaultValue");
Platform::String^ CustomResourceLoader::m_nullId(L"Null");

void CustomResourceLoader::AddResource(
    Platform::String^ resourceId,
    Platform::String^ objectType,
    Platform::String^ propertyName,
    Platform::String^ propertyType,
    Platform::Object^ value)
{
    m_resourceIdToObject[resourceId][objectType][propertyName][propertyType] = value;
}

void CustomResourceLoader::AddResource(
    Platform::String^ resourceId,
    Platform::Object^ value)
{
    m_simpleResourceIdToObject[resourceId] = value;
}

Platform::Object^ CustomResourceLoader::GetDefaultResourceFromResourceId(
    Platform::String^ resourceId,
    Platform::String^ objectType)
{
    if (objectType != TypeName(Platform::String::typeid).Name)
    {
        throw std::invalid_argument("Only a default string is supported");
    }
    return m_defaultResourceIdValue;
}

Platform::Object^ CustomResourceLoader::GetResource(
    Platform::String^ resourceId,
    Platform::String^ objectType,
    Platform::String^ propertyName,
    Platform::String^ propertyType)
{
    if (resourceId == m_nullId)
    {
        return nullptr;
    }

    if (resourceId == m_defaultResourceId)
    {
        return GetDefaultResourceFromResourceId(resourceId, objectType);
    }

    auto simpleIterator = m_simpleResourceIdToObject.find(resourceId);
    if (simpleIterator != m_simpleResourceIdToObject.end())
    {
        return simpleIterator->second;
    }

    return m_resourceIdToObject.at(resourceId).at(objectType).at(propertyName).at(propertyType);
}
