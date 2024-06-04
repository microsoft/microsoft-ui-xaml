// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <DynamicMetadataStorage.h>
#include <CStaticLock.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { 
    namespace Metadata {

        struct DynamicMetadataStorageMock : public std::enable_shared_from_this<DynamicMetadataStorageMock>
        {
            friend class MockXamlMetadataProvider;

            bool LastAccessWasLocked = false;

            DirectUI::DynamicMetadataStorage* GetStorage()
            {
                LastAccessWasLocked = DirectUI::CStaticLock::IsOwnedByCurrentThread();
                return &storage;
            }

            void OnReset()
            {
                LastAccessWasLocked = DirectUI::CStaticLock::IsOwnedByCurrentThread();
            }

        protected:
            DirectUI::DynamicMetadataStorage storage;
        };
    }

} } } }