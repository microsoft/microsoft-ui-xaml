// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Private { namespace Infrastructure {

    struct HdrOutputOverrideHelper
    {
        HdrOutputOverrideHelper()
        {
            test_infra::TestServices::WindowHelper->SetHdrOutputOverride(true);
        }

        ~HdrOutputOverrideHelper()
        {
            test_infra::TestServices::WindowHelper->SetHdrOutputOverride(false);
        }
    };

}}
