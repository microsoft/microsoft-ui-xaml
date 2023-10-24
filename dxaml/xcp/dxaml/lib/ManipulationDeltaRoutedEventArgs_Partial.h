// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ManipulationDeltaRoutedEventArgs.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(ManipulationDeltaRoutedEventArgs)
    {
        
    public:
        _Check_return_ HRESULT get_DeltaImpl(_Out_ ixp::ManipulationDelta* pValue);
        _Check_return_ HRESULT get_CumulativeImpl(_Out_ ixp::ManipulationDelta* pValue);
        _Check_return_ HRESULT get_VelocitiesImpl(_Out_ ixp::ManipulationVelocities* pValue);
    };
}
