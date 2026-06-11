// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <AssociativeStorage.h>
#include <forward_list>
#include <vector_map.h>
#include <vector_set.h>
#include "resources\inc\OverrideInfo.h"

class CModifiedValue;
class CThemeResource;
class CStyle;

using ModifiedValuesList = std::forward_list<std::shared_ptr<CModifiedValue>>;
using ThemeResourceMap = containers::vector_map<KnownPropertyIndex, xref_ptr<CThemeResource>>;
using SetterValueChangedNoficationSubscribersList = containers::vector_set<xref::weakref_ptr<CStyle>>;

namespace AssociativeStorage
{
    // These need to be ordered by alignof() from largest to smallest.
    // Feel free to rearrange the existing order if necessary to satisfy
    // this requirement.
    enum class CDOFields : uint8_t
    {
        SetterValueChangedNoficationSubscribers,
        ModifiedValues,
        ThemeResources,
        AutomationAnnotations,
        OverrideResourceKey,
        Sentinel
    };

    template <>
    struct FieldInfo<CDOFields, CDOFields::ModifiedValues>
    {
        using StorageType = ModifiedValuesList;
    };

    template <>
    struct FieldInfo<CDOFields, CDOFields::ThemeResources>
    {
        using StorageType = ThemeResourceMap;
    };

    template <>
    struct FieldInfo<CDOFields, CDOFields::AutomationAnnotations>
    {
        using StorageType = xref_ptr<CDependencyObject>;
    };

    template <>
    struct FieldInfo<CDOFields, CDOFields::SetterValueChangedNoficationSubscribers>
    {
        using StorageType = SetterValueChangedNoficationSubscribersList;
    };

    template <>
    struct FieldInfo<CDOFields, CDOFields::OverrideResourceKey>
    {
        using StorageType = Resources::ScopedResources::OverrideInfo;
    };
}