// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Common {

class SurfaceIdModeScopeGuard
{
public:
    explicit SurfaceIdModeScopeGuard(MockDComp::SurfaceIdMode surfaceMode)
    {
        TestServices::Utilities->SetMockDCompSurfaceIdMode(surfaceMode);
    }

    ~SurfaceIdModeScopeGuard()
    {
        TestServices::Utilities->SetMockDCompSurfaceIdMode(MockDComp::SurfaceIdMode::AllocationOrder);
    }

    // Disallow copying/moving
    SurfaceIdModeScopeGuard(const SurfaceIdModeScopeGuard&) = delete;
    SurfaceIdModeScopeGuard(SurfaceIdModeScopeGuard&&) = delete;
    SurfaceIdModeScopeGuard& operator=(const SurfaceIdModeScopeGuard&) = delete;
    SurfaceIdModeScopeGuard& operator=(SurfaceIdModeScopeGuard&&) = delete;
};

} } } } }
