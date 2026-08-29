// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <memory>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Common {

    // Use this in a test class with custom DPs. When using custom types, you should be using 
    // XamlMetadataProviderOverrides, which will implicitly do metadata cleanup, so no need to 
    // also use this helper in that case.
    class MetadataCleanup
    {
    public:
        MetadataCleanup()
        {
        }

        ~MetadataCleanup()
        {
            test_infra::TestServices::Utilities->ResetMetadata();
        }
    };

} } } } }