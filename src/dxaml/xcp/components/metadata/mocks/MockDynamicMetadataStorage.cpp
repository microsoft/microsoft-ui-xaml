// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <TypeTableStructs.h>
#include <ThreadLocalStorage.h>
#include <MockDynamicMetadataStorage.h>

using namespace DirectUI;
using namespace Microsoft::UI::Xaml::Tests::Metadata;

DynamicMetadataStorage* DynamicMetadataStorage::GetInstance()
{
    auto mock = TlsProvider<DynamicMetadataStorageMock>::GetWrappedObject();

    if (mock != nullptr)
    {
        return mock->GetStorage();
    }

    // If you hit this assert, you need to add "auto mock = TlsProvider<DynamicMetadataStorageMock>::CreateWrappedObject();" to your unit test.
    ASSERT(false);
    return nullptr;
}

void DynamicMetadataStorage::Reset()
{
    auto mock = TlsProvider<DynamicMetadataStorageMock>::GetWrappedObject();

    if (mock != nullptr)
    {
        return mock->OnReset();
    }
}

void DynamicMetadataStorage::Destroy()
{
}