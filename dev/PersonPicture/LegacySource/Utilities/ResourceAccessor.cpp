// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"

#include "ResourceAccessor.h"
using namespace Platform;
using namespace Microsoft::People::Controls;
using namespace Windows::ApplicationModel::Resources;

const StringReference ResourceAccessor::c_resourceLoc = L"Microsoft.People.Controls/Resources";

String^ ResourceAccessor::s_singularSuffix = "Singular";
String^ ResourceAccessor::s_plural1Suffix = "Plural1";
String^ ResourceAccessor::s_plural2Suffix = "Plural2";
String^ ResourceAccessor::s_plural3Suffix = "Plural3";
String^ ResourceAccessor::s_plural4Suffix = "Plural4";
String^ ResourceAccessor::s_plural5Suffix = "Plural5";
String^ ResourceAccessor::s_plural6Suffix = "Plural6";
String^ ResourceAccessor::s_plural7Suffix = "Plural7";
IResourceLoader^ ResourceAccessor::s_resourceLoader = nullptr;

String^ ResourceAccessor::GetLocalizedStringResource(String^ resourceName)
{
#ifdef _PEOPLE_PLATFORM_
    return SharedUtilities::GetApplicationResourceLoader()->GetString(resourceName);
#else
    if (s_resourceLoader == nullptr)
    {
        s_resourceLoader = ResourceLoader::GetForViewIndependentUse(c_resourceLoc);
    }

    return s_resourceLoader->GetString(resourceName);
#endif
}

String^ ResourceAccessor::GetLocalizedPluralStringResource(String^ resourceNamePrefix, uint32 numericValue)
{
#ifdef _PEOPLE_PLATFORM_
    String^ pluralResourceName = ResourceAccessor::_GetPluralResourceName(resourceNamePrefix, numericValue);
    return SharedUtilities::GetApplicationResourceLoader()->GetString(pluralResourceName);
#else
    if (s_resourceLoader == nullptr)
    {
        s_resourceLoader = ResourceLoader::GetForViewIndependentUse(c_resourceLoc);
    }

    String^ pluralResourceName = ResourceAccessor::_GetPluralResourceName(resourceNamePrefix, numericValue);
    return ResourceAccessor::GetLocalizedStringResource(pluralResourceName);
#endif
}

String^ ResourceAccessor::_GetPluralResourceName(String^ resourceNamePrefix, unsigned int numericValue)
{
    uint32 valueMod10 = numericValue % 10;
    String^ pluralSuffix;

    if (numericValue == 1)
    {
        // Singular
        pluralSuffix = ResourceAccessor::s_singularSuffix;
    }
    else if (numericValue == 2)
    {
        // 2
        pluralSuffix = ResourceAccessor::s_plural7Suffix;
    }
    else if (numericValue == 3 || numericValue == 4)
    {
        // 3, 4
        pluralSuffix = ResourceAccessor::s_plural2Suffix;
    }
    else if (numericValue >= 5 && numericValue <= 10)
    {
        // 5-10
        pluralSuffix = ResourceAccessor::s_plural5Suffix;
    }
    else if (numericValue >= 11 && numericValue <= 19)
    {
        // 11-19
        pluralSuffix = ResourceAccessor::s_plural6Suffix;
    }
    else if (valueMod10 == 1)
    {
        // 21, 31, 41, etc.
        pluralSuffix = ResourceAccessor::s_plural1Suffix;
    }
    else if (valueMod10 >= 2 && valueMod10 <= 4)
    {
        // 22-24, 32-34, 42-44, etc.
        pluralSuffix = ResourceAccessor::s_plural3Suffix;
    }
    else
    {
        // Everything else... 0, 20, 25-30, 35-40, etc.
        pluralSuffix = ResourceAccessor::s_plural4Suffix;
    }

    return String::Concat(resourceNamePrefix, pluralSuffix);
}