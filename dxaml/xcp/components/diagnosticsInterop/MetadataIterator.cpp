// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "inc\MetadataIterator.h"
#include "DynamicMetadataStorage.h"
#include "Indexes.g.h"
namespace Diagnostics {

    template<>
    KnownPropertyIndex EnumIterator<KnownPropertyIndex>::Begin()
    {
        return static_cast<KnownPropertyIndex>(1);
    }

    template<>
    KnownPropertyIndex EnumIterator<KnownPropertyIndex>::End()
    {
        DirectUI::DynamicMetadataStorageInstanceWithLock storage;
        return storage->GetNextAvailablePropertyIndex();
    }

    template<>
    KnownTypeIndex EnumIterator<KnownTypeIndex>::Begin()
    {
        return static_cast<KnownTypeIndex>(1);
    }

    template<>
    KnownTypeIndex EnumIterator<KnownTypeIndex>::End()
    {
        DirectUI::DynamicMetadataStorageInstanceWithLock storage;
        return storage->GetNextAvailableTypeIndex();
    }
}

