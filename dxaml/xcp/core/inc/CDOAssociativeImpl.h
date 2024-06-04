// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <CDOAssociative.h>

namespace AssociativeStorage
{
    template <>
    struct FieldRuntimeInfoArray<CDOFields>
    {
        // These need to be in the same order as the fields of AssociativeStorage::CDOFields
        static constexpr FieldRuntimeInfoArrayType<CDOFields> fields =
        {
            GenerateFieldRuntimeInfo<CDOFields, CDOFields::SetterValueChangedNoficationSubscribers>(),
            GenerateFieldRuntimeInfo<CDOFields, CDOFields::ModifiedValues>(),
            GenerateFieldRuntimeInfo<CDOFields, CDOFields::ThemeResources>(),
            GenerateFieldRuntimeInfo<CDOFields, CDOFields::AutomationAnnotations>(),
            GenerateFieldRuntimeInfo<CDOFields, CDOFields::OverrideResourceKey>(),
        };
    };
}