// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Common {

class DisableDCompLeakDetectionScopeGuard
{
public:
    explicit DisableDCompLeakDetectionScopeGuard()
    {
        test_infra::TestServices::Utilities->SetDCompDeviceLeakDetectionEnabled(false);
    }

    ~DisableDCompLeakDetectionScopeGuard()
    {
        test_infra::TestServices::Utilities->SetDCompDeviceLeakDetectionEnabled(true);
    }

    // Disallow copying/moving
    DisableDCompLeakDetectionScopeGuard(const DisableDCompLeakDetectionScopeGuard&) = delete;
    DisableDCompLeakDetectionScopeGuard(DisableDCompLeakDetectionScopeGuard&&) = delete;
    DisableDCompLeakDetectionScopeGuard& operator=(const DisableDCompLeakDetectionScopeGuard&) = delete;
    DisableDCompLeakDetectionScopeGuard& operator=(DisableDCompLeakDetectionScopeGuard&&) = delete;
};

} } } } }
