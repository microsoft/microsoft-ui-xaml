// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Common {

class DisableRenderingScopeGuard
{
public:
    explicit DisableRenderingScopeGuard()
    {
        test_infra::TestServices::WindowHelper->SetIsRenderEnabled(false);
    }

    ~DisableRenderingScopeGuard()
    {
        test_infra::TestServices::WindowHelper->SetIsRenderEnabled(true);
    }

    // Disallow copying/moving
    DisableRenderingScopeGuard(const DisableRenderingScopeGuard&) = delete;
    DisableRenderingScopeGuard(DisableRenderingScopeGuard&&) = delete;
    DisableRenderingScopeGuard& operator=(const DisableRenderingScopeGuard&) = delete;
    DisableRenderingScopeGuard& operator=(DisableRenderingScopeGuard&&) = delete;
};

} } } } }
