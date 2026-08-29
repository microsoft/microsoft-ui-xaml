// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Private { namespace Infrastructure {

    class ICoordinateTransformer
    {
    public:
        virtual POINT ComputeScreenCoordinates(_In_ xaml::IFrameworkElement* elementInIsland, wf::Point point) = 0;
    };

} }
